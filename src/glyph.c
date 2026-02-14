
#include <glyph.h>
#include <glyph_utils.h>
#include <utils.h> 
#include <containers/darray.h>
#include <context.h>
#include <sys_utils.h>

#include <stb_image.h>

#include <string.h>

const float CHAR_SIZE = 58.601f;
const uint32_t CHAR_OFFSET_X = 58;
const uint32_t CHAR_OFFSET_Y = 58;

const uint32_t SYM_PER_LINE = 26;
const uint32_t SYM_TOTAL = 100;

const uint8_t DETECTION_THRESHOLD = 120;

typedef struct Vec2i {
    int32_t x;
    int32_t y;
} Vec2i; 

typedef struct ImageData {
    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t* data;
} ImageData;

static void detect_glyph_data(ImageData* img, Vec2i src_offset, Glyph* glyph, uint8_t c) {
    int32_t min_x = INT32_MAX;
    int32_t max_x = -1;

    const int32_t cell_px = (int32_t)(CHAR_SIZE + 0.5f);

    for (int32_t y = 0; y < cell_px; y++) {
        for (int32_t x = 0; x < cell_px; x++) {
            int32_t index = ((src_offset.y + y) * img->width + (src_offset.x + x)) * img->channels;
            uint8_t* pixel = &img->data[index];

            uint8_t luminance = pixel_luminance(pixel, img->channels);
            if (luminance < DETECTION_THRESHOLD) {
                min_x = MIN(min_x, x);
                max_x = MAX(max_x, x);
            }
        }
    }

    // Set to defaults if nothing detected
    if (min_x == INT32_MAX || max_x == -1) {
        glyph->width = cell_px * 0.65f;
        glyph->offset_x = 0;
        glyph->offset_y = 0;
    }
    else {
        glyph->width = (max_x - min_x) + 1;
        float glyph_center_x = min_x + glyph->width * 0.5f;
        glyph->offset_x = glyph_center_x - (cell_px * 0.5f);
        glyph->offset_y = 0;
    }

    glyph->width += 1; // TODO: Remove Constant PADDING
}

static void auto_detect_image(ImageData* img, CharacterSet* set) {
    for (int32_t i = 0; i < SYM_TOTAL; i++) {
        Vec2i focused_char = {0};
        focused_char.x = (i % SYM_PER_LINE) * CHAR_SIZE + CHAR_OFFSET_X;
        focused_char.y = (i / SYM_PER_LINE) * CHAR_SIZE + CHAR_OFFSET_Y;
        uint8_t c = tile_index_to_char(i);
        detect_glyph_data(img, focused_char, &set->font_widths[c], c);
    }
    set->image_width = img->width;
    set->image_height = img->height;
    set->image_channels = img->channels;
    set->image_data = img->data;
}

int32_t generate_glyphs(CharacterSet *sets, char **fonts) {
    for (int32_t i = 0; i < darrayLength(fonts); i++) {
        int32_t img_width, img_height, img_channels;
        uint8_t* font_data = stbi_load(fonts[i], &img_width, &img_height, &img_channels, 0);
        if (!font_data) {
            fprintf(stderr, "failed to load font template: %s\n", fonts[i]);
            return 1;
        }

        ImageData img = {
            .width = img_width,
            .height = img_height,
            .channels = img_channels,
            .data = font_data,
        };

        auto_detect_image(&img, &sets[i]);
    }

    return 0;
}

int32_t get_character_sets(struct Context *ctx, const char* font_dir) {
    if (!font_dir) {
        fprintf(stderr, "Must specify 1 font template directory\n");
        return 1;
    }

    char** fonts = darrayCreate(char*);
    if (read_files_in_dir(font_dir, &fonts) || darrayLength(fonts) <= 0) {
        fprintf(stderr, "Failed getting font templates\n");
        return 1;
    }

    ctx->sets_count = darrayLength(fonts);
    ctx->sets = calloc(ctx->sets_count, sizeof(CharacterSet));
    generate_glyphs(ctx->sets, fonts);
    darrayDestroy(fonts);

    return 0;
}

void destroy_character_sets(CharacterSet *sets, int32_t sets_count) {
    for (int32_t i = 0; i < sets_count; i++) {
        free(sets[i].image_data);
    }
    free(sets);
}

