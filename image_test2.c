
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int32_t arial_widths[128] = {
    ['A'] = 667, ['B'] = 667, ['C'] = 722, ['D'] = 722, ['E'] = 667,
    ['F'] = 611, ['G'] = 778, ['H'] = 722, ['I'] = 278, ['J'] = 500,
    ['K'] = 667, ['L'] = 556, ['M'] = 833, ['N'] = 722, ['O'] = 778,
    ['P'] = 667, ['Q'] = 778, ['R'] = 722, ['S'] = 667, ['T'] = 611,
    ['U'] = 722, ['V'] = 667, ['W'] = 944, ['X'] = 667, ['Y'] = 667,
    ['Z'] = 611,
    
    ['a'] = 556, ['b'] = 556, ['c'] = 500, ['d'] = 556, ['e'] = 556,
    ['f'] = 278, ['g'] = 556, ['h'] = 556, ['i'] = 222, ['j'] = 222,
    ['k'] = 500, ['l'] = 222, ['m'] = 833, ['n'] = 556, ['o'] = 556,
    ['p'] = 556, ['q'] = 556, ['r'] = 333, ['s'] = 500, ['t'] = 278,
    ['u'] = 556, ['v'] = 500, ['w'] = 722, ['x'] = 500, ['y'] = 500,
    ['z'] = 500,
    
    ['0'] = 556, ['1'] = 556, ['2'] = 556, ['3'] = 556, ['4'] = 556,
    ['5'] = 556, ['6'] = 556, ['7'] = 556, ['8'] = 556, ['9'] = 556,
    
    [' '] = 278,  
    ['!'] = 278, ['"'] = 355, ['#'] = 556, ['$'] = 556, ['%'] = 889,
    ['&'] = 667, ['\''] = 191, ['('] = 333, [')'] = 333, ['*'] = 389,
    ['+'] = 584, [','] = 278, ['-'] = 333, ['.'] = 278, ['/'] = 278,
    [':'] = 278, [';'] = 278, ['<'] = 584, ['='] = 584, ['>'] = 584,
    ['?'] = 556, ['@'] = 1015,
    ['['] = 278, ['\\'] = 278, [']'] = 278, ['^'] = 469, ['_'] = 556,
    ['`'] = 333, ['{'] = 334, ['|'] = 260, ['}'] = 334, ['~'] = 584
};

int main(void) {
    int32_t width, height, channels;
    uint8_t* data = stbi_load("test.jpeg", &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "failed to load image\n");
        return 1;
    }
    printf("Channels: %d\n", channels);

    int32_t offset_x = 57;
    int32_t offset_y = 57;
    int32_t char_size = 60;
    int32_t char_size_squared = char_size * char_size;
    int32_t img_size = char_size_squared * channels;
    int32_t tiles = 20;

    uint8_t* crop_img = malloc(img_size * tiles);

    for (int32_t i = 0; i < tiles; i++) {
        for (int32_t y = 0; y < char_size; y++) {
            for (int32_t x = 0; x < char_size; x++) {
                for (int32_t c = 0; c < channels; c++) {
                    int32_t tile_offset = i * char_size * channels;
                    int32_t pixel_offset = (y * char_size * tiles + x) * channels;
                    int32_t src_offset = ((offset_y + y) * width + (offset_x + x + i * char_size)) * channels;
                    uint8_t src_val = data[src_offset + c];
                    crop_img[tile_offset + pixel_offset + c] = src_val < 120 ? 0 : 255;
                }
            }
        }
    }

    stbi_write_jpg("crop.jpg", char_size * tiles, char_size, channels, crop_img, 90);

    free(crop_img);
    stbi_image_free(data);

    return 0;
}

