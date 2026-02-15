
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <raylib.h> 
#include <math.h>

#define CLAY_IMPLEMENTATION
#include <clay.h>

int main(void)
{
    InitWindow(800, 600, "ttchef");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    // Get browser size
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);

    // Match internal render resolution to browser size
    emscripten_set_canvas_element_size("#canvas", (int)w, (int)h);

    SetTargetFPS(60);

    Vector2 cube_pos = {400, 300};

    while (!WindowShouldClose()) {
        SetWindowSize(w, h);
        int width = GetScreenWidth();
        int height = GetScreenHeight();

        if (IsKeyDown(KEY_A)) {
            cube_pos.x -= 10;
        }
        if (IsKeyDown(KEY_D)) {
            cube_pos.x += 10;
        }
        if (IsKeyDown(KEY_W)) {
            cube_pos.y -= 10;
        }
        if (IsKeyDown(KEY_S)) {
            cube_pos.y += 10;
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            cube_pos = GetMousePosition();
        }

        Vector2 center = (Vector2){width * 0.5f, height * 0.5f};
        const float dist = 400.0f;

        Vector2 pos;
        pos.x = center.x - cos(GetTime()) * dist;
        pos.y = center.y - sin(GetTime()) * dist;        

        BeginDrawing();
        ClearBackground(PINK);

            DrawLine(pos.x, center.y, pos.x, pos.y, BLACK);   
            DrawLine(center.x, center.y, pos.x, center.y, BLACK);   
            DrawCircleV(pos, 15.0f, BLUE);
            DrawCircleLinesV(center, dist, WHITE);

            const char* text_x = TextFormat("%.2f", (pos.x - center.x) / dist);
            int32_t text_x_width = MeasureText(text_x, 20);
            DrawText(text_x, center.x + (pos.x - center.x) * 0.5f - text_x_width * 0.5f, center.y - 22, 20, WHITE);

            const char* text_y = TextFormat("%.2f", -(pos.y - center.y) / dist);
            int32_t text_y_width = MeasureText(text_y, 20);
            DrawText(text_y, pos.x - text_y_width - 6, center.y + (pos.y - center.y) * 0.5f + 10, 20, WHITE);

            DrawRectangleV(cube_pos, (Vector2){20.0f, 20.0f}, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

