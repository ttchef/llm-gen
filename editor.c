
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "utils.h"

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

static void detect_bounding_box(RawImageData* image_data, float char_size, Vector2I src_offset,
                                Vector2* bounding_box, uint8_t threshold, int32_t auto_detection_padding) {
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
        bounding_box->x = cell_px * 0.65f;
        bounding_box->y = 0;
    }
    else {
        bounding_box->x = (max_x - min_x) + 1;
        float glyph_center_x = min_x + bounding_box->x * 0.5f;
        bounding_box->y = glyph_center_x - (cell_px * 0.5f);
    }
    bounding_box->x += auto_detection_padding;
}

static void auto_detect_images(RawImageData* image_data, float char_size, int32_t char_offset_x,
                               int32_t char_offset_y, Vector2* bounding_boxes, size_t count, uint8_t threshold, int32_t auto_detection_padding) {
    for (int32_t i = 0; i < count; i++) {
        Vector2I focused_char = {0};
        focused_char.x = (i % sym_per_line) * char_size + char_offset_x;
        focused_char.y = (i / sym_per_line) * char_size + char_offset_y;
        detect_bounding_box(image_data, char_size, focused_char, &bounding_boxes[i], threshold, auto_detection_padding);
    }
}

static void export(Vector2* bounding_boxes, size_t count) {
    FILE* file = fopen("array.h", "wb");
    if (!file) {
        fprintf(stderr, "failed to write file\n");
        exit(1);
    }

    fprintf(file, "\nstruct Glyph {\n\tuint32_t width;\n\tint32_t offset;\n};\n\n");
    fprintf(file, "const struct Glyph font_widths[256] = {\n");

    for (int32_t i = 0; i < count; i++) {
        uint8_t ascii = tile_index_to_char(i);
        fprintf(file, "\t[%u] = { %d, %d },\n", ascii, (int32_t)bounding_boxes[i].x, (int32_t)bounding_boxes[i].y);
    }
    fprintf(file, "};\n\n");

    fclose(file);
}

static void import(const char* filepath, Vector2* bounding_boxes, size_t count) {
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
    
    while (fgets(line, max_line_length, file)) {
        if (!in_array && strncmp(matching_string, line, matching_len) == 0) {
            in_array = true;
            continue;
        }

        if (in_array) {
            // get pos of {
            char* current_char = strchr(line, '{');
            if (!current_char) {
                fprintf(stderr, "failed parsing: %s\n", filepath);
                fclose(file);
                return;
            }

            // go to number
            current_char += 2;

            int32_t width = strtoul(current_char, &current_char, 10);
            if (*current_char != ',') {
                fprintf(stderr, "failed to parse file: %s\n", filepath);
                fclose(file);
                return;
            }
            current_char += 2;

            int32_t offset = strtoul(current_char, &current_char, 10);
            bounding_boxes[bound_index++] = (Vector2){width, offset};
            if (bound_index == count) {
                fclose(file);
                fprintf(stderr, "successfuly importet file\n");
                return;
            }
            
        }
    }

    fclose(file);
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
    Vector2 bounding_box[sym_total];
    memset(bounding_box, 0, sizeof(Vector2) * sym_total);

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
            Vector2 dim = bounding_box[current_char];
            float percent_size = dim.x / char_size;
            float percent_offset = dim.y / char_size;
            DrawRectangle((dst.width / 2) - (percent_size * dst.width) / 2 + percent_offset * dst.width, 0,
                          percent_size * dst.width, dst.height, (Color){225, 24, 12, 120});
        }

        DrawRectangle(window_width * texture_width, 0, window_width * ui_width, window_height, DARKGRAY);

        // UI
        int32_t ui_start_X = window_width * texture_width;
        int32_t ui_width_pixels = window_width * ui_width;
        int32_t padding_x = ui_width_pixels * 0.1f;
        int32_t padding_y = window_height * 0.09f;
        int32_t ui_width_padding = ui_width_pixels - 2 * padding_x;
        int32_t current_y = padding_y;
        int32_t buttonHeight = window_height * 0.2f;

        Rectangle bounds = {
            .x = ui_start_X + padding_x,
            .y = current_y,
            .width = ui_width_padding,
            .height = 50,
        };
        if (GuiButton(bounds, "Focus Character")) focus_char = !focus_char;
        
        current_y += padding_y;
        bounds.y = current_y;
        if (GuiButton(bounds, "Next Char") && focus_char) {
            if (current_char + 1 < sym_total) current_char++;
            focused_char.x = current_char % sym_per_line;
            focused_char.y = current_char / sym_per_line;
        }

        current_y += padding_y;
        bounds.y = current_y;
        if (GuiButton(bounds, "Previous Char") && focus_char) {
            if (current_char - 1 >= 0) current_char--; 
            focused_char.x = current_char % sym_per_line;
            focused_char.y = current_char / sym_per_line;
        }

        current_y += padding_y;
        bounds.y = current_y;
        GuiSlider(bounds, "Left", "Right", &bounding_box[current_char].x, 1.0f, char_size);

        current_y += padding_y;
        bounds.y = current_y;
        GuiSlider(bounds, "Left", "Right", &bounding_box[current_char].y, -char_size / 4.0f, char_size / 4.0f);

        current_y += padding_y;
        bounds.y = current_y;
        if (GuiButton(bounds, "Auto Detect")) {
            auto_detect_images(&image_data, char_size, char_offset_x, char_offset_y, bounding_box, sym_total, auto_detection_threshold, auto_detection_padding);
        }

        current_y += padding_y;
        bounds.y = current_y;
        float tmp_threshold = auto_detection_threshold;
        GuiSlider(bounds, "Left", "Right", &tmp_threshold, 1, 255);
        auto_detection_threshold = tmp_threshold;

        bounds.width = ui_width_padding * 0.25f;
        bounds.x = ui_start_X + ui_width_padding * 0.75f;
        current_y += padding_y;
        bounds.y = current_y;
        static bool edit_mode = false;
        if (GuiValueBox(bounds, "Auto Detection Extra Padding", &auto_detection_padding, 0, char_size, edit_mode)) edit_mode = !edit_mode;
        bounds.width = ui_width_padding;
        bounds.x = ui_start_X + padding_x;

        current_y += padding_y;
        bounds.y = current_y;
        if (GuiButton(bounds, "Export")) {
            export(bounding_box, sym_total);
        }

        current_y += padding_y;
        bounds.y = current_y;
        if (GuiButton(bounds, "Import")) {
            import("array.h", bounding_box, sym_total);
        }

        EndDrawing();
    }

    stbi_image_free(image_data.data);
    CloseWindow();

    return 0;
}

