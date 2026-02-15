
#ifndef WEB_CONTEXT_H
#define WEB_CONTEXT_H

#include <stdint.h>

typedef struct Window {
    int32_t width;
    int32_t height;
} Window;

typedef struct WebContext {
    Window window;
} WebContext;

#endif // WEB_CONTEXT_H
