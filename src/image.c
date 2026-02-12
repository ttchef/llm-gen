
#include "utils.h"
#include <image.h>
#include <glyph.h>
#include <glyph_utils.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static void skip_unicode(char* out, size_t* size, const char* input) {
    size_t i = 0;
    size_t out_index = 0;

    while (input[i] != '\0') {
        unsigned char c = (unsigned char)input[i];
        if (c < 0x80) {
            // ASCII (1 byte)
            if (out && out_index < *size - 1) {
                if (c == '\\' && input[i + 1] == 'u' && 
                input[i + 2] && input[i + 3] && input[i + 4] && input[i + 5]) {

                    char s[5];
                    s[0] = input[i + 2];
                    s[1] = input[i + 3];
                    s[2] = input[i + 4];
                    s[3] = input[i + 5];
                    s[4] = '\0';
                    uint32_t hex_val = strtoul(s, NULL, 16);
                    if (hex_val < 128) { // TODO: change later to extended
                        out[out_index] = (char)hex_val;
                        out_index++;
                    }
                    i += 6;
                    continue;
                }
                out[out_index] = c;
            }
            out_index++;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            // german ä ö etc..
            if (input[i + 1]) {
                uint8_t c1 = (uint8_t)input[i];
                uint8_t c2 = (uint8_t)input[i + 1];

                uint32_t codepoint = ((c1 & 0x1F) << 6) | (c2 & 0x3F);

                if (codepoint < 256 && out && out_index < *size - 1) {
                    out[out_index] = (uint8_t)codepoint;
                }
                out_index++;
                i += 2;
            }
            else {
                i += 2;
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte UTF-8 (emoji)
            i += 4;
        } else {
            // Invalid byte, skip
            i += 1;
        }
    }
    if (out) out[out_index] = '\0'; 
    *size = out_index + 1;
}

static int32_t count_rows(const char* text, size_t len, CharacterSet max_set, int32_t pixels_horizontal) {
    int32_t current_x = 0;
    int32_t rows = 1;

    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n' || (text[i] == '\\' && text[i + 1] == 'n')) {
            if (text[i] == '\\') i++;
            rows++;
            current_x = 0;
            continue;
        }

        int32_t pixels_char = max_set.font_widths[(uint8_t)text[i]].width;

        if (current_x + pixels_char >= pixels_horizontal) {
            rows++;
            current_x = pixels_char;
        }
        else {
            current_x += pixels_char;
        }
    }

    return rows;
}

/* Returns character set with the hightest width */
static CharacterSet get_max_char_set(CharacterSet* sets, size_t sets_count) {
    CharacterSet res = {0};

    for (int32_t i = 0; i < (int32_t)sets_count; i++) {
        for (int32_t j = 0; j < SYM_TOTAL; j++) {
            uint8_t c = tile_index_to_char(j);
            if (res.font_widths[c].width < sets[i].font_widths[c].width) {
                res.font_widths[c].width = sets[i].font_widths[c].width;
                res.font_widths[c].offset_x = sets[i].font_widths[c].offset_x;
                res.font_widths[c].offset_y = sets[i].font_widths[c].offset_y;
            }
        }
    }

    return res;
}

static void draw_background(Images* images, int32_t current_image, enum PageBackgroundType bg_type) {
    // Horizontal
    for (int32_t y = 0; y < images->height; y += CHAR_SIZE) {
        for (int32_t x = 0; x < images->width; x++) {
            for (int32_t c = 0; c < images->channels; c++) {
                images->images_data[current_image][(y * images->width + x) * images->channels + c] = 211;
            }
        }
    }

    if (bg_type != PAGE_BG_TYPE_SQUARES) return;
    for (int32_t y = 0; y < images->height; y++) {
        for (int32_t x = 0; x < images->width; x += CHAR_SIZE) {
            for (int32_t c = 0; c < images->channels; c++) {
                images->images_data[current_image][(y * images->width + x) * images->channels + c] = 211;
            }
        }
    }
}

Images generate_font_image(Page page, char* text, CharacterSet* sets, size_t sets_count) {
    size_t asci_text_len;
    skip_unicode(NULL, &asci_text_len, text);
    uint8_t asci_text[asci_text_len];
    skip_unicode(asci_text, &asci_text_len, text);

    CharacterSet max_set = get_max_char_set(sets, sets_count);
    int32_t usable_pixels_x = page.dim.width - page.padding.x * 2;
    int32_t usable_pixels_y = page.dim.height - page.padding.y * 2;

    int32_t required_rows = count_rows(asci_text, asci_text_len, max_set, usable_pixels_x);
    int32_t total_height = required_rows * CHAR_SIZE + page.padding.y * 2;
    int32_t num_needed_pages = (int32_t)ceilf((float)total_height / (float)page.dim.height);

    Images images = {
        .width = page.dim.width,
        .height = page.dim.height,
        .channels = 4,
        .images_count = num_needed_pages,
        .images_data = malloc(sizeof(uint8_t*) * num_needed_pages),
    };

    for (int32_t i = 0; i < num_needed_pages; i++) {
        images.images_data[i] = malloc(images.width * images.height * images.channels);
        memset(images.images_data[i], 255, images.width * images.height * images.channels);
        if (!images.images_data[i]) {
            fprintf(stderr, "failed to allocate rendering image: %d\n", i);
            return images;
        }
        
        draw_background(&images, i, page.bg_type);
    }

    return images;
}

void destroy_images(Images *images) {
    for (int32_t i = 0; i < images->images_count; i++) {
        if (images->images_data[i]) free(images->images_data[i]);
    }
    if (images->images_data) free(images->images_data);
}

