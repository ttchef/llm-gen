
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "utils.h"
#include "ui.h"

// global because i dont care rn
int32_t window_width = 1200;
int32_t window_height = 800;

const float ui_width = 0.3f;
const float texture_width = 1.0f - ui_width;

const uint32_t char_offset_x = 58;
const uint32_t char_offset_y = 58;
const float char_size = 58.601f;
const uint32_t sym_per_line = 26;

const uint32_t sym_total = 100;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct Vector2I {
    int32_t x;
    int32_t y;
} Vector2I;

typedef struct RawImageData {
    int32_t width;
    int32_t height;
    int32_t channels;
    uint8_t* data;
} RawImageData;

typedef struct GylphData {
    float width;
    float offset_x;
    float offset_y;
} GlyphData;

static void detect_gylph_data(RawImageData* image_data, float char_size, Vector2I src_offset,
                                GlyphData* glyph, uint8_t threshold, int32_t auto_detection_padding) {
    int32_t min_x = INT32_MAX;
    int32_t max_x = -1;

    const int32_t cell_px = (int32_t)(char_size + 0.5f);

    for (int32_t y = 0; y < cell_px; y++) {
        for (int32_t x = 0; x < cell_px; x++) {
            int32_t index = ((src_offset.y + y) * image_data->width + (src_offset.x + x)) * image_data->channels;
            uint8_t* pixel = &image_data->data[index];

            uint8_t luminance = pixel_luminance(pixel, image_data->channels);
            if (luminance < threshold) {
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
    }

    glyph->width += auto_detection_padding;
}

static void auto_detect_images(RawImageData* image_data, float char_size, int32_t char_offset_x,
                               int32_t char_offset_y, GlyphData* glyphs, size_t count, uint8_t threshold, int32_t auto_detection_padding) {
    for (int32_t i = 0; i < count; i++) {
        Vector2I focused_char = {0};
        focused_char.x = (i % sym_per_line) * char_size + char_offset_x;
        focused_char.y = (i / sym_per_line) * char_size + char_offset_y;
        detect_gylph_data(image_data, char_size, focused_char, &glyphs[i], threshold, auto_detection_padding);
    }
}

static void export(GlyphData* bounding_boxes, size_t count) {
    FILE* file = fopen("array.h", "wb");
    if (!file) {
        fprintf(stderr, "failed to write file\n");
        exit(1);
    }

    fprintf(file, "\nstruct Glyph {\n\tuint32_t width;\n\tint32_t offset;\n};\n\n");
    fprintf(file, "const struct Glyph font_widths[256] = {\n");

    for (int32_t i = 0; i < count; i++) {
        uint8_t ascii = tile_index_to_char(i);
        fprintf(file, "\t[%u] = { %d, %d, %d },\n", ascii, (int32_t)bounding_boxes[i].width,
                                                         (int32_t)bounding_boxes[i].offset_x,
                                                         (int32_t)bounding_boxes[i].offset_y);
    }
    fprintf(file, "};\n\n");

    fclose(file);
}

static void import(const char* filepath, GlyphData* bounding_boxes, size_t count) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "failed to open file: %s\n", filepath);
        return;
    }

    const int32_t max_line_length = 128;
    char line[max_line_length];
    bool in_array = false;

    const char* matching_string = "const struct Glyph font_widths";
    size_t matching_len = strlen(matching_string);

    int32_t bound_index = 0;
    int32_t line_index = 0;

    while (fgets(line, max_line_length, file)) {
        line_index++;
        if (!in_array && strncmp(matching_string, line, matching_len) == 0) {
            in_array = true;
            continue;
        }

        if (in_array) {
            // get pos of {
            char* current_char = strchr(line, '{');
            if (!current_char) {
                fprintf(stderr, "failed parsing: %s at line: %d\nExpected: '{' found '%c'\n", filepath, line_index, *current_char);
                fclose(file);
                return;
            }

            // go to number
            current_char += 2;

            int32_t width = strtoul(current_char, &current_char, 10);
            if (*current_char != ',') {
                fprintf(stderr, "failed parsing: %s at line: %d when getting the glyph widht\nExpected: ',' found '%c'\n",
                        filepath, line_index, *current_char);
                fclose(file);
                return;
            }
            current_char += 2;

            int32_t offset_x = strtoul(current_char, &current_char, 10);
            if (*current_char != ',') {
                fprintf(stderr, "failed parsing: %s at line: %d when getting the glyph offset_x\nExpected: ',' found '%c'\n",
                        filepath, line_index, *current_char);                
                fclose(file);
                return;
            }
            current_char += 2;

            int32_t offset_y = strtol(current_char, &current_char, 10);
            
            bounding_boxes[bound_index++] = (GlyphData){width, offset_x, offset_y};
            if (bound_index == count) {
                fclose(file);
                fprintf(stderr, "successfuly importet file\n");
                return;
            }
            
        }
    }

    fclose(file);
}

void draw_ui_tab(int32_t* tab, bool* focus_char, Vector2I* focused_char, int32_t* current_char,
                 GlyphData* glyphs, RawImageData* image_data, uint8_t* auto_detection_threshold,
                 int32_t* auto_detection_padding, int32_t tabs_count, const char** tabs_name,
                 int32_t dst_width, int32_t dst_height) {
    uiInfo info = {
        .start_x = window_width * texture_width,
        .start_y = 0,
        .width = window_width * ui_width,
        .height = window_height,
        .padding_x = info.width * 0.1f,
        .padding_y = info.height * 0.01f,
        .element_height = 50,
    };

    uiBegin(&info);

    if (uiButton(NULL, "Focus Character")) *focus_char = !*focus_char;

    if (uiButton(NULL, "Next Char") && *focus_char) {
        if (*current_char + 1 < sym_total) (*current_char)++;
        focused_char->x = *current_char % sym_per_line;
        focused_char->y = *current_char / sym_per_line;
    }

    if (uiButton(NULL, "Previous Char") && *focus_char) {
        if (*current_char - 1 >= 0) (*current_char)--; 
        focused_char->x = *current_char % sym_per_line;
        focused_char->y = *current_char / sym_per_line;
    }

    Rectangle bounds = uiGetCurrentRect();

    // Tab Menu
    bounds.width /= tabs_count;
    for (int32_t i = 0; i < tabs_count; i++) {
        if (uiButton(&bounds, tabs_name[i])) {
            *tab = i;
        }
        bounds.x += bounds.width;
    }

    // check bounds for tab
    if (*tab < 0 || *tab > 3) *tab = 0;
    switch (*tab) {
        case 0: {
            uiSlider(NULL, "Width", NULL, &glyphs[*current_char].width, 1.0f, char_size);

            uiSlider(NULL, "Offset X", NULL, &glyphs[*current_char].offset_x, -char_size / 4.0f, char_size / 4.0f);
            break;
        }
        case 1: {
            if (uiButton(NULL, "Auto Detect")) {
                auto_detect_images(image_data, char_size, char_offset_x, char_offset_y, glyphs,
                                   sym_total, *auto_detection_threshold, *auto_detection_padding);
            }

            const uint8_t* threshold_text = "Threshold";
            float tmp_threshold = *auto_detection_threshold;
            uiSlider(NULL, threshold_text, NULL, &tmp_threshold, 1, 255);
            *auto_detection_threshold = tmp_threshold;

            float tmp_padding = *auto_detection_padding;
            uiSlider(NULL, "Padding", NULL, &tmp_padding, 1, 15);
            *auto_detection_padding = tmp_padding;

            break;
        }
        case 2: {
            static bool checked = false;
            uiCheckBox(NULL, "Show Base Line", &checked);

            uiSlider(NULL, "Offset Y", NULL, &glyphs[*current_char].offset_y, 0, char_size);

            if (checked) {
                float percent_offset_y = glyphs[*current_char].offset_y / char_size;
                // Base Line
                DrawRectangle(0, dst_height - dst_height * percent_offset_y, dst_width, dst_height * 0.05f, Fade(PURPLE, 0.5f));
            }
            break;
        }
    }

    if (uiButton(NULL, "Export")) {
        export(glyphs, sym_total);
    }

    if (uiButton(NULL, "Import")) {
        import("array.h", glyphs, sym_total);
    }

    uiEnd(&info);

}

// char because of lsp
int32_t main(int32_t argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: <%%s> image path to text\n");
        return 1;
    }

    fprintf(stderr, "Image Path: %s\n", argv[1]);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "Yoo");
    GuiLoadStyle("style_jungle.rgs");

    int32_t current_char = 0;
    Vector2I focused_char = (Vector2I){0};
    bool focus_char = false;
    GlyphData glyphs[sym_total];
    memset(glyphs, 0, sizeof(GlyphData) * sym_total);

    // Raylib texture loading
    Texture2D image = LoadTexture(argv[1]);
    float image_ratio = (float)image.width / (float)image.height;

    RawImageData image_data = {0};
    image_data.data = stbi_load(argv[1], &image_data.width, &image_data.height, &image_data.channels, 0);
    if (!image_data.data) {
        fprintf(stderr, "failed to load image: %s\n", argv[1]);
    }
    printf("Channels: %d\n", image_data.channels);

    uint8_t auto_detection_threshold = 120;
    int32_t auto_detection_padding = 0;

    // Ui stuff
    int32_t current_tab = 0;

    const int32_t tabs_count = 3;
    const char* tabs_name[tabs_count] ;
    tabs_name[0] = "Manual";
    tabs_name[1] = "Auto";
    tabs_name[2] = "Offset Y";

    while (!WindowShouldClose()) {
        window_width = GetScreenWidth();
        window_height = GetScreenHeight();

        BeginDrawing();
        ClearBackground(BLACK);

        Rectangle src = (Rectangle){0};
        if (!focus_char) {
            src.width = image.width;
            src.height = image.height;
        }
        else {
            src.x = (char_size * focused_char.x) + char_offset_x;
            src.y = (char_size * focused_char.y) + char_offset_y;
            src.width = char_size;
            src.height = char_size;
        }
        image_ratio = (float)src.width / (float)src.height;

        float dst_width = window_width * texture_width;
        float dst_height = dst_width / image_ratio;

        if (dst_height > window_height) {
            dst_height = window_height;
            dst_width = dst_height * image_ratio;
        }

        Rectangle dst = {
            .width = dst_width,
            .height = dst_height,
        };

        DrawTexturePro(image, src, dst, (Vector2){0, 0}, 0.0f, WHITE);

        if (focus_char) {
            GlyphData glyph = glyphs[current_char];
            float percent_size = glyph.width / char_size;
            float percent_offset_x = glyph.offset_x / char_size;
            int32_t pos_x = (dst.width / 2) - (percent_size * dst.width) / 2 + percent_offset_x * dst.width;
            DrawRectangle(pos_x, 0, percent_size * dst.width, dst.height, (Color){225, 24, 12, 120});
        }

        DrawRectangle(window_width * texture_width, 0, window_width * ui_width, window_height, DARKGRAY);

        draw_ui_tab(&current_tab, &focus_char, &focused_char, &current_char, glyphs, &image_data,
                    &auto_detection_threshold, &auto_detection_padding, tabs_count, tabs_name, dst.width, dst.height);

        EndDrawing();
    }

    stbi_image_free(image_data.data);
    CloseWindow();

    return 0;
}

