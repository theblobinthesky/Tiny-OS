from PIL import ImageFont, ImageDraw, Image
import numpy as np

ASCII_START = 33
ASCII_END = 126
FONT_PATH = "pixel-comic-sans-undertale-sans-font.ttf"
GLYPH_SIZE_PX = 16
CHARACTERS = [chr(c) for c in range(ASCII_START, ASCII_END + 1)]
HEADER_FILE_PATH = "../../src/kernel/pixel_font.h"

font = ImageFont.truetype(FONT_PATH, GLYPH_SIZE_PX)
ascent, descent = font.getmetrics()
fontArray = None

for char in CHARACTERS:
    bbox = font.getbbox(char)
    # maybe try this: font.getmask()
    
    image = Image.new(mode='L', size=(GLYPH_SIZE_PX, GLYPH_SIZE_PX))
    draw = ImageDraw.Draw(image)
    # draw.fontmode = "1" # disable anti-aliasing
    draw.text((GLYPH_SIZE_PX // 2, 0), char, fill=(255), font=font, anchor="ma")
    arr = np.asarray(image)[np.newaxis, :, :]

    if fontArray is None: fontArray = arr
    else: fontArray = np.concatenate([fontArray, arr], axis=0)

# import matplotlib.pyplot as plt
# plt.imshow(fontArray.reshape((-1, GLYPH_SIZE_PX)))
# plt.show()

arrayCode = "#pragma once\n"
arrayCode += "// I understand this is not an ideal way to deploy a font.\n"
arrayCode += "// But truetype font rendering is kind of overkill for this tiny kernel.\n"
arrayCode += "// Maybe it's not idk. Userspace code can and will do whatever it wants though.\n"
arrayCode += "// So maybe the kernel will just use stb_truetype.h aswell...\n"
arrayCode += "\n"
arrayCode += f"#define FONT_GLYPH_SIZE_PX {GLYPH_SIZE_PX}\n"
arrayCode += f"#define FONT_GLYPH_PTR(c) (font + {GLYPH_SIZE_PX * GLYPH_SIZE_PX * 1} * ((c) - (char){ASCII_START}))\n"
arrayCode += "\n"
arrayCode += "char font[] = {\n"

for i in range(fontArray.shape[0]):
    for y in range(fontArray.shape[1]):
        arrayCode += "    "
        for x in range(fontArray.shape[2]):
            arrayCode += str(fontArray[i, y, x]) + ", "  
        arrayCode += "\n"
    arrayCode += "\n"

arrayCode += "};"

with open(HEADER_FILE_PATH, "w") as headerFile:
    headerFile.write(arrayCode)