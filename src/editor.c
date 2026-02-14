
#define CLAY_IMPLEMENTATION
#include <editor.h>

#include <clay-renderers/clay_renderer_raylib.h>
#include <raylib.h>

int32_t run_editor() {
    Clay_Raylib_Initialize(800, 600, "Glyph Editor", FLAG_WINDOW_RESIZABLE);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
    }

    Clay_Raylib_Close();

    return 0;
}
