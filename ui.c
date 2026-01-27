
#include "ui.h"

#include <raylib.h>
#include <stdio.h>

bool uiPointInRect(Vector2 point, uiRect rect) {
    return (point.x > rect.x && 
            point.y > rect.y &&
            point.x < rect.x + rect.width && 
            point.y < rect.y + rect.height);
}

bool uiButton(uiRect bounds) {
    if (!uiPointInRect(GetMousePosition(), bounds)) return false;
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return false;

    return true;
}

