
#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stddef.h>
#include <glyph.h>
#include <utils.h>

uint8_t* generate_font_image(Page page, char* text, CharacterSet* sets, size_t sets_count);

#endif // IMAGE_H
