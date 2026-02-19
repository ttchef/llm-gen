
#ifndef WEB_CONTEXT_H
#define WEB_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>

#include <raylib.h>

#define TEXT_BOX_MAX_INPUT_CHARS 100

typedef enum WebState {
    WEB_STATE_BROWSE_FONT_MAP,
} WebState;

typedef enum TextBoxType {
    TEXT_BOX_TYPE_NUMBERS = (1 << 0),
    TEXT_BOX_TYPE_LOWERCASE_ALPHA = (1 << 1),
    TEXT_BOX_TYPE_UPPERCASE_ALHPA = (1 << 2),
    TEXT_BOX_TYPE_ALL_ALHPA = (3 << 1),
    TEXT_BOX_TYPE_ALL = 0xfffffff,
} TextBoxType;

typedef struct TextBox {
    TextBoxType type;
    bool input;
    char array[TEXT_BOX_MAX_INPUT_CHARS];
    int32_t len; /* Actualy limited len */
    int32_t index;
} TextBox;

typedef struct Window {
    int32_t width;
    int32_t height;
} Window;

typedef struct WebContext {
    WebState state;
    Window window;
    Vector2 mouse_curr;
    Vector2 mouse_prev;

    /* UI */
    TextBox browse_font_maps;
} WebContext;

#endif // WEB_CONTEXT_H

