
#ifndef GLYPH_UTILS_H
#define GLYPH_UTILS_H

#include <stdint.h>

uint8_t pixel_luminance(const uint8_t* p, int32_t channels);
int32_t char_to_tile_index(uint8_t c);
uint8_t tile_index_to_char(int32_t index);

#endif // GLYPH_UTILS_H
