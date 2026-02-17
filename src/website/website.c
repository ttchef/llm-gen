
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <raylib.h> 

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <website/web_context.h>
#include <website/clay_renderer_raylib.h>
#include <website/ui.h>

#define CLAY_IMPLEMENTATION
#include <clay.h>

void handle_clay_errors(Clay_ErrorData error_data) {
    const char* msg = error_data.errorText.chars
                      ? error_data.errorText.chars
                      : "Unknown Clay error (null message)";
    
    fprintf(stderr, "[CLAY_ERROR]: %s\n", msg);
}


int main(void) {
    WebContext ctx = { 
        .window = {
            .width = 800,
            .height = 600,
        },
    };

    uint64_t total_mem = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(total_mem, malloc(total_mem));
    Clay_Initialize(arena, (Clay_Dimensions){ctx.window.width, ctx.window.height}, (Clay_ErrorHandler){ handle_clay_errors });
    Clay_Raylib_Initialize(ctx.window.width, ctx.window.height,
                           "idk bro", FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    Font fonts[] = {
        LoadFontEx("../../"ASSETS_DIR"/fonts/AdwaitaSans-Regular.ttf", 20, 0, 250), // TODO: tmp with the path
        LoadFontEx("../../"ASSETS_DIR"/fonts/AdwaitaSans-Regular.ttf", 40, 0, 250), 
    };

    Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);
    
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    emscripten_set_canvas_element_size("#canvas", (int)w, (int)h);

    SetTargetFPS(60);

    SetWindowSize(w, h);
    ctx.window.width = GetScreenWidth();
    ctx.window.height = GetScreenHeight();

    while (!WindowShouldClose()) {


        update_ui(&ctx); 
        compute_ui_layout(&ctx);

        BeginDrawing();
        ClearBackground(PINK);
        draw_ui(&ctx, fonts);

        EndDrawing();
    }

    Clay_Raylib_Close();

    return 0;
}

