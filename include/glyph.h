
#ifndef GLYPH_H
#define GLYPH_H

#include <stdint.h>

extern const float CHAR_SIZE;
extern const uint32_t CHAR_OFFSET_X;
extern const uint32_t CHAR_OFFSET_Y;

extern const uint32_t SYM_PER_LINE;
extern const uint32_t SYM_TOTAL;

extern const uint8_t DETECTION_THRESHOLD;

typedef struct Glyph {
    uint32_t width;
    int32_t offset_x;
    int32_t offset_y;
} Glyph;

typedef struct CharacterSet {
    Glyph font_widths[256];
    int32_t image_width;
    int32_t image_height;
    int32_t image_channels;
    uint8_t* image_data;
} CharacterSet;

int32_t generate_glyphs(CharacterSet* sets, char** fonts);
void destroy_character_sets(CharacterSet* sets, int32_t sets_count);

#endif // GLYPH_H
