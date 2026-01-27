
#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>

/* using raylib backend */
#include <raylib.h>

typedef struct uiRect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height; 
} uiRect;

/* helper */
bool uiPointInRect(Vector2 point, uiRect rect);

bool uiButton(uiRect bounds);

#endif // UI_H
