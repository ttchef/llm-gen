
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// global because i dont care rn
int32_t window_width = 1200;
int32_t window_height = 800;

const float ui_width = 0.3f;
const float texture_width = 1.0f - ui_width;

const uint32_t char_offset_x = 58;
const uint32_t char_offset_y = 58;
const float char_size = 58.601f;
const uint32_t sym_per_line = 26;

const uint32_t sym_total = 99;

typedef struct Vector2I {
    int32_t x;
    int32_t y;
} Vector2I;

// char because of lsp
int32_t main(int32_t argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: <%%s> image path to text\n");
        return 1;
    }

    fprintf(stderr, "Image Path: %s\n", argv[1]);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(window_width, window_height, "Yoo");

    int32_t current_char = 0;
    Vector2I focused_char = (Vector2I){0};
    bool focus_char = false;
    Vector2 bounding_box[sym_total];
    memset(bounding_box, 0, sizeof(Vector2) * sym_total);

    Texture2D image = LoadTexture(argv[1]);
    float image_ratio = (float)image.width / (float)image.height;

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
            DrawRectangle((dst.width / 2) - ((dim.x / 60.0f) * dst.width) / 2 + dim.y, 0, (dim.x / 60.0f) * dst.width, dst.height, (Color){225, 24, 12, 120});
        }

        DrawRectangle(window_width * texture_width, 0, window_width * ui_width, window_height, GRAY);

        // UI
        int32_t ui_start_X = window_width * texture_width;
        int32_t ui_width_pixels = window_width * ui_width;
        int32_t padding_x = ui_width_pixels * 0.1f;
        int32_t padding_y = window_height * 0.1f;
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
        if (GuiButton(bounds, "Next Char")) {
            if (current_char + 1 < sym_total) current_char++;
            focused_char.x = current_char % sym_per_line;
            focused_char.y = current_char / sym_per_line;
        }

        current_y += padding_y;
        bounds.y = current_y;
        if (GuiButton(bounds, "Previous Char")) {
            if (current_char - 1 >= 0) current_char--; 
            focused_char.x = current_char % sym_per_line;
            focused_char.y = current_char / sym_per_line;
        }

        current_y += padding_y;
        bounds.y = current_y;
        GuiSlider(bounds, "Left", "Right", &bounding_box[current_char].x, 1.0f, char_size);

        current_y += padding_y;
        bounds.y = current_y;
        GuiSlider(bounds, "Left", "Right", &bounding_box[current_char].y, -char_size, char_size);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}

