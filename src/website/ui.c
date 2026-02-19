
#include "website/web_context.h"
#include <website/ui.h>
#include <website/clay_renderer_raylib.h>
#include <website/assets.h>
#include <website/ui_modules.h>
#include <website/ui_utils.h>

#include <clay.h>
#include <raylib.h>

#include <stdio.h>

static void compute_search_layout(WebContext* ctx, Texture2D* textures) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = { 0, 0, 280, 0 },
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, },
        },
        .backgroundColor = UI_COLOR_DARK_DARK_DARK_GRAY,
    }) {
        CLAY_AUTO_ID({
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .sizing = { CLAY_SIZING_FIXED(750), CLAY_SIZING_FIXED(205) },
                .childGap = 16,
            },
        }) {
            CLAY_AUTO_ID({
                .layout = {
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.62f) },
                    .childGap = 16,
                },
            }) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    },
                    .aspectRatio = 1,
                    .cornerRadius = CLAY_CORNER_RADIUS(12),
                    .image = {
                        .imageData = &textures[0],
                    },
                });

                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
                    },
                }) {
                    CLAY_TEXT(CLAY_STRING("ttchef.org"), CLAY_TEXT_CONFIG({
                        .fontId = ASSET_FONT_100,
                        .fontSize = 100,
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
                    .layout = { 
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    },
                    .backgroundColor = UI_COLOR_DARK_GRAY,
                    .cornerRadius = CLAY_CORNER_RADIUS(50),
                }) {

                    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && Clay_Hovered()) {
                        ctx->browse_font_maps.input = true;
                    }
                    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        ctx->browse_font_maps.input = false;
                    }

                    module_text_box(&ctx->browse_font_maps);
                }
            }
        }
    }
}

void compute_ui_layout(WebContext *ctx, Texture2D* textures) {
    Clay_BeginLayout();

    switch (ctx->state) {
        case WEB_STATE_BROWSE_FONT_MAP:
            compute_search_layout(ctx, textures);
            break;
    }
}

void init_ui(WebContext *ctx) {
    ctx->browse_font_maps.type = TEXT_BOX_TYPE_ALL;
    ctx->browse_font_maps.index = 0;
    ctx->browse_font_maps.len = 35;
}

void update_ui(WebContext *ctx) {
    bool is_mouse_down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ctx->mouse_prev = ctx->mouse_curr;
    ctx->mouse_curr = GetMousePosition();

    Clay_SetLayoutDimensions((Clay_Dimensions){ctx->window.width, ctx->window.height});
    Clay_SetPointerState((Clay_Vector2){ctx->mouse_curr.x, ctx->mouse_curr.y}, is_mouse_down);

    if (ctx->browse_font_maps.input) {
        module_text_box_add(&ctx->browse_font_maps, NULL);
    }
}

void draw_ui(WebContext *ctx, Font *fonts) {
    Clay_RenderCommandArray cmd_array = Clay_EndLayout();
    Clay_Raylib_Render(cmd_array, fonts);
}


