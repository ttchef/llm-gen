
#include "website/web_context.h"
#include <website/ui.h>
#include <website/clay_renderer_raylib.h>
#include <clay.h>

/* Temp */
const Clay_Color UI_COLOR_LIGHT_GRAY = (Clay_Color){120, 120, 120, 255};
const Clay_Color UI_COLOR_DARK_GRAY = (Clay_Color){80, 80, 80, 255};
const Clay_Color UI_COLOR_DARK_DARK_GRAY = (Clay_Color){60, 60, 60, 255};
const Clay_Color UI_COLOR_DARK_DARK_DARK_GRAY = (Clay_Color){40, 40, 40, 255};
const Clay_Color UI_COLOR_BLACK = (Clay_Color){0, 0, 0, 255};
const Clay_Color UI_COLOR_WHITE = (Clay_Color){255, 255, 255, 255};
const Clay_Color UI_COLOR_RED = (Clay_Color){255, 0, 0, 255};
const Clay_Color UI_COLOR_LIGHT_BLUE = (Clay_Color){84, 145, 244, 125};

void compute_search_layout(WebContext* ctx) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
        },
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { CLAY_SIZING_FIXED(500), CLAY_SIZING_FIXED(175) },
                .childGap = 16,
            },
        }) {
            CLAY_AUTO_ID({
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.7f) },
                    .childGap = 16,
                },
            }) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    },
                    .aspectRatio = 1,
                    .backgroundColor = UI_COLOR_RED,
                    .cornerRadius = CLAY_CORNER_RADIUS(12),
                });

                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING("ttchef.org"), CLAY_TEXT_CONFIG({
                        .fontId = 1,
                        .fontSize = 40,
                        .textColor = UI_COLOR_WHITE,
                    }));
                }
            }

            CLAY_AUTO_ID({
                .layout = {
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            },
            }) {
                CLAY_AUTO_ID({
                    .layout = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .backgroundColor = UI_COLOR_DARK_DARK_GRAY,
                    .cornerRadius = CLAY_CORNER_RADIUS(12),
                }) {

                }
            }
        }
    }
}

void compute_ui_layout(WebContext *ctx) {
    Clay_BeginLayout();

    compute_search_layout(ctx);
}

void update_ui(WebContext *ctx) {
    bool is_mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Clay_SetLayoutDimensions((Clay_Dimensions){ctx->window.width, ctx->window.height});
    Clay_SetPointerState((Clay_Vector2){ctx->mouse_curr.x, ctx->mouse_curr.y}, is_mouse_down);
}

void draw_ui(WebContext *ctx, Font *fonts) {
    Clay_RenderCommandArray cmd_array = Clay_EndLayout();
    Clay_Raylib_Render(cmd_array, fonts);
}


