#include <stdio.h>
#include <string>
#include <vector>
#include <string.h>
#include <filesystem>
#include <algorithm>
#include <queue>

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
#define SEC_PER_CLUSTER 4
#define CLUSTER_SIZE (SECTOR_SIZE * SEC_PER_CLUSTER)
#define RESERVED_SECTOR_COUNT 32
#define NUM_FATS 2
#define ROOT_CLUSTER 2
#define BACKUP_BOOT_SEC 6
#define FS_INFO_SEC 1
#define FIRST_USABLE_DATA_CLUSTER 2
#define MIN_TOTAL_SIZE 64 * 1024 * 1024

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
    u16 fat_size_16 = 0x3d;
    u16 sectors_per_track = 0x3f;
    u16 num_of_heads = 0xff;
    u32 num_hidden_sectors = 0x3f;
    u32 total_sectors_32;
} __attribute__((packed));

struct bootsector_fat32 {
    u32 num_fat_sectors;
    u16 ext_flags = 0;
    u16 fs_version = 0;
    u32 root_cluster = ROOT_CLUSTER;
    u16 fs_info = FS_INFO_SEC;
    u16 bk_boot_sec = BACKUP_BOOT_SEC;
    u8 reserved[12] = {};
    u8 drive_num = 0x80;
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

#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

struct directory_entry {
    char name[8];
    char ext[3];
    u8 attrib; // ATTR_ARCHIVE must be set when the file is created
    u8 nt_res = 0;
    u8 crt_time_tenth = 0; // The crt times don't need to be supported.
    u16 crt_time = 0;
    u16 crt_date = 0;
    u16 last_access_date; // same value as write_date
    u16 first_cluster_high;
    u16 write_time;
    u16 write_date;
    u16 first_cluster_low;
    u32 file_size_in_bytes;
} __attribute__((packed));

struct tree_directory_entry;

struct tree_file_entry {
    std::string name;
    std::string ext;
    tree_directory_entry *parent;
    u32 size;
    std::string host_path;
};

struct tree_directory_entry {
    std::string name;
    tree_directory_entry *parent;
    std::vector<tree_directory_entry *> directories;
    std::vector<tree_file_entry *> files;
    directory_entry *dir_entry;
    u32 dir_entry_size;
};

struct sector {
    u8 bytes[SECTOR_SIZE];
} __attribute__((packed));

struct cluster {
    u8 bytes[CLUSTER_SIZE];
} __attribute__((packed));

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

u32 read_entire_file_cluster_aligned(std::string path, u8* &file_buffer, u32 &aligned_file_size) {
    FILE *file = fopen(path.c_str(), "rb");
    fseek(file, 0, SEEK_END);
    u32 file_size = (u32) ftell(file);
    rewind(file);

    aligned_file_size = align_to(file_size, CLUSTER_SIZE);
    file_buffer = new u8[aligned_file_size];
    memset(file_buffer, 0, aligned_file_size);

    fread(file_buffer, 1, file_size, file);
    fclose(file);

    return file_size;
}

void write_file_to_path(std::string path, const u8 *buffer, u32 size) {
    FILE *file = fopen(path.c_str(), "wb");
    fwrite(buffer, size, 1, file);
    fclose(file);
}

u32 get_cluster_byte_offset_in_fat(u32 cluster_index) {
    return RESERVED_SECTOR_COUNT * SECTOR_SIZE + cluster_index * 4;
}

#define FAT_EOF 0x0fffffff

void *allocate_single_cluster(u32 *fat, cluster *clusters, std::vector<u32>& free_cluster_list, u32 &cluster_index) {
    cluster_index = free_cluster_list.back();
    free_cluster_list.pop_back();
    
    fat[cluster_index] = FAT_EOF;
    return (void *)(clusters + cluster_index);
}

void *allocate_given_single_cluster(u32 *fat, cluster *clusters, std::vector<u32>& free_cluster_list, u32 cluster_index) {
    free_cluster_list.erase(std::find(free_cluster_list.begin(), free_cluster_list.end(), cluster_index));
    
    fat[cluster_index] = FAT_EOF;
    return (void *)(clusters + cluster_index);
}

u32 write_clusters_from_file(u32 *fat, cluster *clusters, std::vector<u32>& free_cluster_list, 
    const tree_file_entry *file, u32 *file_size, u32 *first_cluster) {
    u8 *file_buffer;
    u32 aligned_file_size;
    *file_size = read_entire_file_cluster_aligned(file->host_path, file_buffer, aligned_file_size);
    u32 num_clusters = aligned_file_size / CLUSTER_SIZE;

    u32 next_cluster_index = FAT_EOF; // initialize with EOF
    for (int i = num_clusters - 1; i >= 0; i--) {
        u32 cluster_index = free_cluster_list.back();
        free_cluster_list.pop_back();

        fat[cluster_index] = next_cluster_index;
        memcpy(clusters + cluster_index, file_buffer + i * CLUSTER_SIZE, CLUSTER_SIZE);

        next_cluster_index = cluster_index;
    }

    *first_cluster = next_cluster_index;
    delete[] file_buffer;

    return num_clusters;
}

bool get_name_and_ext(std::string full_name, std::string& file_name, std::string& file_ext) {    
    auto found_dot = full_name.find_last_of(".");
    file_name = full_name.substr(0, found_dot);

    if (found_dot == -1) {
        file_name = full_name;
        file_ext = "   ";
    } else {
        file_name = full_name.substr(0, found_dot);
        file_ext = full_name.substr(found_dot + 1);
    }

    return file_name.size() <= sizeof(tree_file_entry::name) && file_ext.size() <= sizeof(tree_file_entry::ext);
}

std::vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;

    do
    {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) pos = str.length();
        std::string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + delim.length();
    }
    while (pos < str.length() && prev < str.length());
    
    return tokens;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("The first argument has to be the output path, followed by at least one file to add to the fat32 filesystem.\n");
        return -1;
    }

    std::string output_path = argv[1];

    tree_directory_entry *tree_root = new tree_directory_entry();
    u32 total_file_size = 0;
    for (int i = 2; i < argc; i++) {
        const std::string &both_paths = argv[i];
        int seperatorIndex = both_paths.find_last_of(",");

        if (seperatorIndex == -1) {
            printf("ERROR: Seperator is missing!\n");
            return 1;
        }

        std::string host_path = both_paths.substr(0, seperatorIndex);
        std::string dst_path = both_paths.substr(seperatorIndex + 1);

        if (dst_path[0] != '/') {
            printf("Destination path doesn't start with /.\n");
            return 1;
        }

        std::vector<std::string> parts = split(dst_path, "/");
        tree_directory_entry *parent_entry = tree_root;
        tree_file_entry *file_entry = new tree_file_entry();

        for (int j = 0; j < parts.size(); j++) {
            const std::string &part = parts[j];

            if (j == parts.size() - 1) {
                if (!get_name_and_ext(part, file_entry->name, file_entry->ext)) {
                    printf("Destination file name too long.\n");
                    return 1;
                }

                file_entry->parent = parent_entry;
                file_entry->size = get_file_size(host_path);
                file_entry->host_path = host_path;
                parent_entry->files.push_back(file_entry);
            } else {
                if (part.size() > sizeof(tree_directory_entry::name)) {
                    printf("Destination directory name too long.\n");
                    return 1;
                }

                int dir_idx = -1;
                for (int j = 0; j < parent_entry->directories.size(); j++) {
                    if (part == parent_entry->directories[j]->name) {
                        dir_idx = j;
                        break;
                    }
                }

                if (dir_idx == -1) {
                    tree_directory_entry *dir_entry = new tree_directory_entry();
                    dir_entry->name = part;
                    dir_entry->parent = parent_entry;
                    parent_entry->directories.push_back(dir_entry);

                    parent_entry = parent_entry->directories[parent_entry->directories.size() - 1];
                } else {
                    parent_entry = parent_entry->directories[dir_idx];
                }                
            }
        }

        total_file_size += align_to(file_entry->size, CLUSTER_SIZE);
    }
    
    std::vector<tree_directory_entry *> flattened_dirs;
    std::vector<tree_file_entry *> flattened_files;
    std::queue<tree_directory_entry *> dirs_queue;
    dirs_queue.push(tree_root);

    while (dirs_queue.size() > 0) {
        tree_directory_entry *tree_dir = dirs_queue.front();
        dirs_queue.pop();

        flattened_dirs.push_back(tree_dir);
            
        for (tree_file_entry *file_entry: tree_dir->files) {
            flattened_files.push_back(file_entry);
        }

        for (tree_directory_entry *child_dir: tree_dir->directories) {
            dirs_queue.push(child_dir);
        }
    }


    int num_data_clusters_for_dirs = flattened_dirs.size();
    int num_data_clusters = align_to_divided(total_file_size + num_data_clusters_for_dirs * CLUSTER_SIZE, CLUSTER_SIZE);
    int num_data_sectors = SEC_PER_CLUSTER * num_data_clusters; // TODO: Hack
    int num_fat_sectors = align_to_divided(num_data_sectors * 4, SECTOR_SIZE);
    
    int first_data_sector = RESERVED_SECTOR_COUNT + NUM_FATS * num_fat_sectors;
    int total_sectors = first_data_sector + num_data_sectors;

    // To be recognized as a fat32 image, respect the minimum size.
    if (total_sectors * SECTOR_SIZE < MIN_TOTAL_SIZE) {
        total_sectors = align_to_divided(MIN_TOTAL_SIZE, SECTOR_SIZE); 
    }

    static_assert(sizeof(sector) == SECTOR_SIZE);
    static_assert(sizeof(cluster) == CLUSTER_SIZE);

    sector *output_sectors = new sector[total_sectors];
    memset(output_sectors, 0, total_sectors * SECTOR_SIZE);

    sector *boot_sector = &output_sectors[0];

    *reinterpret_cast<bootsector*>(boot_sector->bytes + 0) = {
        .total_sectors_32 = (u32)total_sectors
    };

    static_assert(sizeof(bootsector) == 36);
    *reinterpret_cast<bootsector_fat32*>(boot_sector->bytes + sizeof(bootsector)) = {
        .num_fat_sectors = (u32) num_fat_sectors,
    };

    // Note: This is only good for SECTOR_SIZE == 512.
    *reinterpret_cast<u32*>(boot_sector->bytes + 510) = 0xaa55;

    // A backup boot sector is required for fat32 volumes.
    memcpy(output_sectors[BACKUP_BOOT_SEC].bytes, boot_sector, sizeof(sector));

    *reinterpret_cast<fs_info *>(output_sectors[FS_INFO_SEC].bytes) = {};

    std::vector<u32> free_cluster_list;
    // Clusters 0 and 1 are not available.
    for (int i = 0; i < num_data_clusters; i++) {
        free_cluster_list.push_back(i + FIRST_USABLE_DATA_CLUSTER);
    }

    u32 *fat1 = reinterpret_cast<u32 *>(output_sectors[RESERVED_SECTOR_COUNT].bytes);
    u32 *fat2 = reinterpret_cast<u32 *>(output_sectors[RESERVED_SECTOR_COUNT + num_fat_sectors].bytes);
    cluster *data_clusters = reinterpret_cast<cluster *>(output_sectors[first_data_sector].bytes) - FIRST_USABLE_DATA_CLUSTER;
    directory_entry *root_dir = reinterpret_cast<directory_entry *>(data_clusters + ROOT_CLUSTER);

    // Required in both fats:
    fat1[0] = 0xfffffff8;
    fat1[1] = FAT_EOF;

    // Write all directories:
    for (int i = 0; i < flattened_dirs.size(); i++) {
        tree_directory_entry *tree_dir_entry = flattened_dirs[i];
        u32 first_cluster;

        if (i == 0) {
            first_cluster = ROOT_CLUSTER;
            tree_dir_entry->dir_entry = (directory_entry *)allocate_given_single_cluster(fat1, data_clusters, free_cluster_list, ROOT_CLUSTER);
        } else {
            tree_dir_entry->dir_entry = (directory_entry *)allocate_single_cluster(fat1, data_clusters, free_cluster_list, first_cluster);
        }

        if (tree_dir_entry->parent != nullptr) {            
            tree_dir_entry->dir_entry[tree_dir_entry->dir_entry_size++] = {
                .name = { '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
                .attrib = ATTR_DIRECTORY,
                .first_cluster_high = (u16)(first_cluster >> 16),
                .first_cluster_low = (u16)first_cluster
            };

            tree_dir_entry->dir_entry[tree_dir_entry->dir_entry_size++] = {
                .name = { '.', '.', ' ', ' ', ' ', ' ', ' ', ' ' },
                .attrib = ATTR_DIRECTORY,
                .first_cluster_high = (tree_dir_entry->parent) ? tree_dir_entry->parent->dir_entry->first_cluster_high : (u16)(ROOT_CLUSTER >> 16),
                .first_cluster_low = (tree_dir_entry->parent) ? tree_dir_entry->parent->dir_entry->first_cluster_low : (u16)ROOT_CLUSTER
            };

            directory_entry dir_entry = {
                .attrib = ATTR_DIRECTORY,
                .last_access_date = 0, // TODO: Find out real value.
                .first_cluster_high = (u16)(first_cluster >> 16),
                .write_time = 0, // TODO: Find out real value.
                .write_date = 0, // TODO: Find out real value.
                .first_cluster_low = (u16)first_cluster,
                .file_size_in_bytes = 0
            };
            memcpy(dir_entry.name, tree_dir_entry->name.c_str(), sizeof(dir_entry.name));

            tree_dir_entry->dir_entry[tree_dir_entry->dir_entry_size++] = dir_entry;
            tree_dir_entry->parent->directories.push_back(tree_dir_entry);
        }

        printf("Wrote directory to cluster %d-%d and path %s!\n", first_cluster, first_cluster, tree_dir_entry->name.c_str());
    }

    // Write all files:
    for (int i = 0; i < flattened_files.size(); i++) {
        const tree_file_entry *tree_file_entry = flattened_files[i];

        u32 file_size, first_cluster;
        u32 num_cluster = write_clusters_from_file(fat1, data_clusters, free_cluster_list, tree_file_entry, &file_size, &first_cluster);

        directory_entry file_entry = {
            .attrib = ATTR_ARCHIVE,
            .last_access_date = 0, // TODO: Find out real value.
            .first_cluster_high = (u16)(first_cluster >> 16),
            .write_time = 0, // TODO: Find out real value.
            .write_date = 0, // TODO: Find out real value.
            .first_cluster_low = (u16)first_cluster,
            .file_size_in_bytes = file_size
        };
        memcpy(file_entry.name, tree_file_entry->name.c_str(), sizeof(file_entry.name));
        memcpy(file_entry.ext, tree_file_entry->ext.c_str(), sizeof(file_entry.ext));

        tree_file_entry->parent->dir_entry[tree_file_entry->parent->dir_entry_size++] = file_entry;
        
        printf("Wrote file %s with size %d to cluster %d-%d and path %s.%s!\n", 
            tree_file_entry->host_path.c_str(), file_size, first_cluster, first_cluster + num_cluster - 1, 
            tree_file_entry->name.c_str(), tree_file_entry->ext.c_str());
    }

    memcpy(fat2, fat1, num_fat_sectors * SECTOR_SIZE);

    u32 total_size = total_sectors * sizeof(sector);
    write_file_to_path(output_path, (u8 *)output_sectors, total_sectors * sizeof(sector));
    printf("Successfully wrote %d bytes (%d sectors) to %s!\n", total_size, total_sectors, output_path.c_str());
}