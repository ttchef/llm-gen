
#include "website/clay_renderer_raylib.h"
#include <website/ui_modules.h>
#include <website/ui_utils.h>
#include <website/assets.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>

static void check_input_number(char* number, char* number_string) {
    size_t max_num_len = strlen(number);
    if (strlen(number_string) != max_num_len) return;

    if (number_string[0] >= number[0]) {
        number_string[0] = number[0];
        for (int32_t i = 1; i < (int32_t)max_num_len; i++) {
            if (number_string[i] > number[i] || number_string[0] == number[0]) {
                number_string[i] = number[i];
            }
        }
    }
}

void module_text_box_add(TextBox* box, char* max_num) {
    char c = GetCharPressed();
    char key = GetKeyPressed();

    if (box->type & TEXT_BOX_TYPE_NUMBERS) {
        if (c >= 48 && c <= 57 && box->index < box->len) {
            if (!(c == 48 && box->index == 0)) {
                box->array[box->index++] = c;
            }
        }
    }

    if (box->type & TEXT_BOX_TYPE_LOWERCASE_ALPHA) {
        if (((c >= 97 && c <= 122) || (c == 32)) && box->index < box->len) {
            box->array[box->index++] = c;
        }
    }
    if (box->type & TEXT_BOX_TYPE_UPPERCASE_ALHPA && IsKeyDown(KEY_LEFT_SHIFT)) {
        if (((c >= 65 && c <= 90) || (c == 32)) && box->index < box->len) {
            box->array[box->index++] = c;
        }
    }

    /* Universal Delte and Enter */
    if (key == 3 && box->index > 0) {
        box->array[--box->index] = 0;
    }
    else if (key == 1) {
        box->input = false;
        if (max_num) {
            check_input_number(max_num, box->array);
        }
    }
}

void module_text_box(TextBox *box) {
    CLAY_AUTO_ID({
        .layout = {
            .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
            .padding = { 10, 0, 0, 0 },
            .childAlignment = { CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_CENTER },
            .childGap = 0,
        },
    }) {
        Clay_String dym_str = {
            .chars = box->array,
            .length = strlen(box->array),
            .isStaticallyAllocated = true,
        };

        CLAY_TEXT(dym_str, CLAY_TEXT_CONFIG({
            .fontId = ASSET_FONT_40,
            .fontSize = 40,
            .textColor = UI_COLOR_WHITE,
        }));

        if (box->input) {
            CLAY_AUTO_ID({
                .layout = { 
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_PERCENT(0.8f) },   
                },
                .border = {
                    .color = UI_COLOR_WHITE,
                    .width = (Clay_BorderWidth){2, 0, 0, 0, 2 },
                },
            });
        }
    }
}


