#include <stdio.h>
#include <string>
#include <vector>
#include <string.h>
#include <filesystem>

// For more information see:
// https://www.cs.fsu.edu/~cop4610t/assignments/project3/spec/fatspec.pdf

typedef __INT8_TYPE__ s8;
typedef __UINT8_TYPE__ u8;
typedef __INT16_TYPE__ s16;
typedef __UINT16_TYPE__ u16;
typedef __INT32_TYPE__ s32;
typedef __UINT32_TYPE__ u32;
typedef __INT64_TYPE__ s64;
typedef __UINT64_TYPE__ u64;

#define SECTOR_SIZE 512
#define SEC_PER_CLUSTER 1
#define CLUSTER_SIZE (SECTOR_SIZE * SEC_PER_CLUSTER)
#define RESERVED_SECTOR_COUNT 32
#define NUM_FATS 2
#define FS_INFO_SEC 1

struct bootsector {
    u8 jmp_boot[3] = {};
    char oem_name[8] = {'M', 'S', 'W', 'I', 'N', '4', '.', '1'};
    u16 bytes_per_sec = SECTOR_SIZE;
    u8 sec_per_cluster = SEC_PER_CLUSTER;
    u16 reserved_sec_count = RESERVED_SECTOR_COUNT;
    u8 num_fats = NUM_FATS;
    u16 root_dir_secs = 0;
    u16 total_sec_count_16 = 0;
    u8 media = 0xf8;
    u16 fat_size_16 = 0;
    u16 sectors_per_track = 0;
    u16 num_of_heads = 0;
    u32 num_hidden_sectors = 0;
    u32 total_sectors_32;
} __attribute__((packed));

struct bootsector_fat32 {
    u32 fat_num_sec_32;
    u16 ext_flags = (1 << 7);
    u16 fs_version = 0;
    u32 root_cluster = 2;
    u16 fs_info = FS_INFO_SEC;
    u16 backup_boot_sector = 6;
    u8 reserved[12] = {};
    u8 drive_num = 0;
    u8 reserved_1 = 0;
    u8 extended_boot_sig = 0x29;
    u32 vol_id = 0xabcdefff;
    char vol_label[11] = {'N', 'O', ' ', 'N', 'A', 'M', 'E', ' ', ' ', ' '};
    char fil_sys_type[8] = {'F', 'A', 'T', '3', '2', ' ', ' ', ' '};
} __attribute__((packed));

struct fs_info {
    u32 lead_sig = 0x41615252;
    u8 reserved[480] = {};
    u32 struc_sig = 0x61417272;
    u32 free_count = 0xffffffff;
    u32 next_free = 0xffffffff;
    u8 reserved_2[12] = {};
    u32 trail_sig = 0xaa550000;
};

struct directory_entry {
    char name[8];
    char ext[3];
    u8 attrib = 0;
    u8 nt_res = 0;
    u8 crt_time_tenth = 0; // The crt times must not be supported.
    u16 crt_time = 0;
    u16 crt_date = 0;
    u16 last_access_date; // same value as write_date
    u16 first_cluster_high;
    u16 write_time;
    u16 write_date;
    u16 first_cluster_low;
    u32 file_size_in_bytes;
} __attribute__((packed));

struct file {
    std::string host_path;
    u32 size;
    char name[8];
    char ext[3];
};

int align_to_divided(int value, int align_to) {
    return (value + align_to - 1) / align_to;
}

int align_to(int value, int align_to) {
    return align_to_divided(value, align_to) * align_to;
}

u32 get_file_size(std::string path) {
    std::filesystem::path p{path};
    return (u32) std::filesystem::file_size(p);
}

u32 read_entire_file_cluster_aligned(std::string path, u8* &file_buffer) {
    FILE *file = fopen(path.c_str(), "rb");
    fseek(file, 0, SEEK_END);
    u32 file_size = (u32) ftell(file);
    fseek(file, 0, SEEK_SET);

    u32 file_buffer_size = align_to(file_size, CLUSTER_SIZE);
    file_buffer = new u8[file_buffer_size];
    memset(file_buffer, 0, file_buffer_size);
    fread(file_buffer, file_size, 1, file);
    fclose(file);

    return file_size;
}

void write_file_to_path(std::string path, const u8 *buffer, u32 size) {
    FILE *file = fopen(path.c_str(), "wb");
    fwrite(buffer, 1, size, file);
    fclose(file);
}

u32 get_cluster_byte_offset_in_fat(u32 cluster_index) {
    return RESERVED_SECTOR_COUNT * SECTOR_SIZE + cluster_index * 4;
}

u32 get_cluster_byte_offset_in_data(u32 cluster_index, int first_data_sector_offset) {
    return first_data_sector_offset + cluster_index * CLUSTER_SIZE;
}

u32 write_clusters_to_file(u8 *output_buffer, int first_data_sector_offset, std::vector<u32>& free_cluster_indices, u8 *file_buffer, u32 file_size, u32 *first_cluster) {
    u32 num_clusters = align_to_divided(file_size, CLUSTER_SIZE);

    u32 next_cluster_index = 0x0FFFFFFF; // initialize with EOF
    for (int i = num_clusters - 1; i >= 0; i--) {
        u32 cluster_index = free_cluster_indices.back();
        free_cluster_indices.pop_back();

        u32 fat_offset = get_cluster_byte_offset_in_fat(cluster_index);
        u32 cluster_offset = get_cluster_byte_offset_in_data(cluster_index, first_data_sector_offset);

        reinterpret_cast<u32 *>(output_buffer + fat_offset)[0] = next_cluster_index;
        memcpy(output_buffer + cluster_offset, file_buffer + i * CLUSTER_SIZE, CLUSTER_SIZE);
        
        next_cluster_index = cluster_index;
    }

    *first_cluster = next_cluster_index;

    return num_clusters;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("The first argument has to be the output path, followed by at least one file to add to the fat32 filesystem.\n");
        return -1;
    }

    std::string output_path = argv[1];

    std::vector<file> files;
    u32 total_file_size = 0;
    for (int i = 2; i < argc; i++) {
        const std::string &host_path = argv[i];
        const std::string file_path = host_path.substr(host_path.find_last_of("/") + 1);

        auto found_dot = file_path.find_last_of(".");
        std::string file_name = file_path.substr(0, found_dot);
        std::string file_ext;

        if (found_dot == -1) {
            file_name = file_path;
            file_ext = "";
        } else {
            file_name = file_path.substr(0, found_dot);
            file_ext = file_path.substr(found_dot + 1);
        }
        
        u32 size = get_file_size(host_path);
        file f = {
            .host_path = host_path,
            .size = size,
            .name = {},
            .ext = {}
        };

        memcpy(f.name, file_name.c_str(), sizeof(f.name));
        memcpy(f.ext, file_ext.c_str(), sizeof(f.ext));

        files.push_back(f);
        
        total_file_size += size;
    }

    // Note: 1 sector per cluster is enough for up to 260MB devices.

    // Compute the amount of data sectors required for the files
    // and calculate a couple of important values and offsets.
    int num_data_clusters = align_to_divided(total_file_size, CLUSTER_SIZE);
    int data_size = 64 * 1024 * 1024; // num_data_clusters * CLUSTER_SIZE;
    int fat_num_sec_32 = align_to_divided(num_data_clusters * 4, SECTOR_SIZE); // todo: root dir secs should be 0 but where does the root dir go then? into the reserved sectors?
    int first_data_sector = (RESERVED_SECTOR_COUNT + (NUM_FATS * fat_num_sec_32));
    int first_data_sector_offset = first_data_sector * SECTOR_SIZE;
    int total_size = first_data_sector_offset + data_size;
    int total_sectors = total_size / SECTOR_SIZE;

    printf("Fat32 filesystem has %d sectors per fat, %d data clusters and the first data sector is %d!\n", fat_num_sec_32, num_data_clusters, first_data_sector);

    u8 *output_buffer = new u8[total_size];
    memset(output_buffer, 0, total_size);

    std::vector<u32> free_cluster_indices;
    for (int i = 0; i < num_data_clusters; i++) {
        free_cluster_indices.push_back(i);
    }

    bootsector boot = {
        .total_sectors_32 = (u32)total_sectors
    };
    *reinterpret_cast<bootsector*>(output_buffer + 0) = boot;

    bootsector_fat32 boot_fat32 = {
        .fat_num_sec_32 = (u32) fat_num_sec_32
    };
    *reinterpret_cast<bootsector_fat32*>(output_buffer + sizeof(bootsector)) = boot_fat32;

    // Note: This is only good for SECTOR_SIZE == 512.
    *reinterpret_cast<u32*>(output_buffer + 510) = 0xaa55;

    fs_info fs = {};
    *reinterpret_cast<fs_info*>(output_buffer + FS_INFO_SEC * SECTOR_SIZE) = fs;

    directory_entry *root_dirs = reinterpret_cast<directory_entry *>(boot_fat32.root_cluster * CLUSTER_SIZE);
    for (int i = 0; i < files.size(); i++) {
        file &f = files[i];

        u8 *file_buffer;
        u32 file_size = read_entire_file_cluster_aligned(f.host_path, file_buffer);

        u32 first_cluster;
        u32 num_cluster = write_clusters_to_file(output_buffer, first_data_sector_offset, free_cluster_indices, file_buffer, file_size, &first_cluster);

        std::string file_name = std::string(f.name, sizeof(f.name));
        std::string file_ext = std::string(f.ext, sizeof(f.ext));
        printf("Wrote file %s with size %d to cluster %d-%d and path %s.%s!\n", 
            f.host_path.c_str(), file_size, first_cluster, first_cluster + num_cluster - 1, file_name.c_str(), file_ext.c_str());

        directory_entry dir = {
            .last_access_date = 0, // TODO: Find out real value.
            .first_cluster_high = (u16)(first_cluster >> 16),
            .write_time = 0, // TODO: Find out real value.
            .write_date = 0, // TODO: Find out real value.
            .first_cluster_low = (u16)(first_cluster & 0xffffffff),
            .file_size_in_bytes = file_size
        };

        memcpy(dir.name, f.name, sizeof(dir.name));
        memcpy(dir.ext, f.ext, sizeof(dir.ext));

        delete[] file_buffer;
    }

    write_file_to_path(output_path, output_buffer, total_size);

    printf("Successfully wrote %d bytes (%d sectors, %d clusters) to %s!\n", total_size, total_size / SECTOR_SIZE, total_size / CLUSTER_SIZE, output_path.c_str());
}