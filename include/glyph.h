
#ifndef GLYPH_H
#define GLYPH_H

#include <stdint.h>

typedef struct Glyph {
    uint32_t width;
    int32_t offset_x;
    int32_t offset_y;
} Glyph;

typedef struct CharacterSet {
    Glyph font_widths[256];
} CharacterSet;

int32_t generate_glyphs(CharacterSet* sets, char** fonts);

#endif // GLYPH_H
