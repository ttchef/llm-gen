
#ifndef WEB_CONTEXT_H
#define WEB_CONTEXT_H

#include <stdint.h>
#include <raylib.h>

typedef struct Window {
    int32_t width;
    int32_t height;
} Window;

typedef struct WebContext {
    Window window;
    Vector2 mouse_curr;
    Vector2 mouse_prev;
} WebContext;

#endif // WEB_CONTEXT_H
