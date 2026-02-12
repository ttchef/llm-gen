
#include "ocr.h"
#include "utils.h"
#include <image.h>
#include <glyph.h>
#include <glyph_utils.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct DrawContext {
    uint32_t current_x;
    uint32_t current_y;
    uint32_t usable_pixels_x;
    uint32_t channels;
} DrawContext;

#define BBP 4

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
        printf("Pixel Width: %d\n", pixels_char);
        printf("Char: %c\n", text[i]);

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
    CharacterSet res;
    memset(&res, 0, sizeof(CharacterSet));

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

static void draw_char(uint8_t c, DrawContext* ctx, CharacterSet* set, Page* page, uint8_t* output_data) {
    int32_t tile_index = char_to_tile_index(c);
    const uint32_t tile_offset_x = CHAR_OFFSET_X + (tile_index % SYM_PER_LINE) * CHAR_SIZE;
    const uint32_t tile_offset_y = CHAR_OFFSET_Y + (tile_index / SYM_PER_LINE) * CHAR_SIZE;

    const uint32_t char_width = set->font_widths[c].width;
    const int32_t char_offset_x = set->font_widths[c].offset_x;
    const int32_t char_offset_y = set->font_widths[c].offset_y;

    const uint32_t template_center_offset_x = CHAR_SIZE * 0.5f - char_width * 0.5f;

    if (ctx->current_x + char_width >= ctx->usable_pixels_x) {
        ctx->current_y += CHAR_SIZE;
        ctx->current_x = 0;
    }

    if (ctx->current_x == 0) ctx->current_x = page->padding.x;
    if (ctx->current_y == 0) ctx->current_y = page->padding.y;

    for (uint32_t y = 0; y < CHAR_SIZE; y++) {
        for (uint32_t x = 0; x < char_width; x++) {
            uint32_t dst_x = ctx->current_x + x;
            uint32_t dst_y = ctx->current_y + y;
            uint32_t dst_index = (dst_y * page->dim.width + dst_x) * ctx->channels;

            uint32_t src_x = tile_offset_x + (CHAR_SIZE * 0.5f) - (char_width * 0.5f) + char_offset_x + x;
            uint32_t src_y = tile_offset_y + y;

            if (src_x < 0 || src_x >= set->image_width ||
                src_y < 0 || src_y >= set->image_height) {
                continue;
            }
            if (dst_x < 0 || dst_x >= page->dim.width ||
                dst_y < 0 || dst_y >= page->dim.height) {
                continue;
            }

            uint32_t src_index = (src_y * set->image_width + src_x) * set->image_channels;

            uint8_t* pixel = &set->image_data[src_index];

            uint8_t luminance = pixel_luminance(pixel, set->image_channels);

            if (luminance < 120) {
                for (int32_t c = 0; c < ctx->channels; c++) {
                    output_data[dst_index + c] = luminance;
                }
            }
        }
    }

    ctx->current_x += char_width;
}

Images generate_font_image(Page page, char* text, CharacterSet* sets, size_t sets_count) {
    size_t asci_text_len;
    skip_unicode(NULL, &asci_text_len, text);
    uint8_t asci_text[asci_text_len];
    skip_unicode(asci_text, &asci_text_len, text);

    printf("\n\n\nAsci String: %s", asci_text);

    CharacterSet max_set = sets[0]; // get_max_char_set(sets, sets_count);
    int32_t usable_pixels_x = page.dim.width - page.padding.x * 2;
    int32_t usable_pixels_y = page.dim.height - page.padding.y * 2;

    int32_t required_rows = count_rows(asci_text, asci_text_len, max_set, usable_pixels_x);
    int32_t total_height = required_rows * CHAR_SIZE + page.padding.y * 2;
    int32_t num_needed_pages = (int32_t)ceilf((float)total_height / (float)page.dim.height);
    printf("Required Rows: %d\n", required_rows);
    printf("Total Height: %d\n", total_height);
    printf("Num Pages: %d\n", num_needed_pages);

    Images images = {
        .width = page.dim.width,
        .height = page.dim.height,
        .channels = BPP,
        .images_count = num_needed_pages,
        .images_data = malloc(sizeof(uint8_t*) * num_needed_pages),
    };

    const int32_t chars_per_page = (int32_t)ceilf((float)asci_text_len / (float)num_needed_pages);
    int32_t text_index = 0;

    DrawContext ctx = {
        .usable_pixels_x = usable_pixels_x,
        .channels = (uint32_t)images.channels,
    };

    for (int32_t i = 0; i < num_needed_pages; i++) {
        images.images_data[i] = malloc(images.width * images.height * images.channels);
        memset(images.images_data[i], 255, images.width * images.height * images.channels);
        if (!images.images_data[i]) {
            fprintf(stderr, "failed to allocate rendering image: %d\n", i);
            return images;
        }
        
        draw_background(&images, i, page.bg_type);

        ctx.current_x = 0;
        ctx.current_y = 0;

        int32_t j = 0;
        for (; j < chars_per_page; j++) {
            if (text_index + j >= asci_text_len) break;
            draw_char(asci_text[text_index + j], &ctx, &sets[0], &page, images.images_data[i]);
        }
        text_index += j; // TODO: maybe + 1
    }

    return images;
}

void destroy_images(Images *images) {
    for (int32_t i = 0; i < images->images_count; i++) {
        if (images->images_data[i]) free(images->images_data[i]);
    }
    if (images->images_data) free(images->images_data);
}

