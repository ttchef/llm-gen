
#ifndef UI_H
#define UI_H

#include <website/web_context.h>
#include <raylib.h>

void compute_ui_layout(WebContext* ctx);
void update_ui(WebContext* ctx);
void draw_ui(WebContext* ctx, Font* fonts);

#endif // UI_H
