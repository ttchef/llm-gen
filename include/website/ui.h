
#ifndef UI_H
#define UI_H

#include <website/web_context.h>
#include <raylib.h>

void init_ui(WebContext* ctx);
void compute_ui_layout(WebContext* ctx, Texture2D* textures);
void update_ui(WebContext* ctx);
void draw_ui(WebContext* ctx, Font* fonts);

#endif // UI_H
