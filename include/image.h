
#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stddef.h>
#include <glyph.h>
#include <utils.h>

typedef struct Images {
    int32_t width;
    int32_t height;
    int32_t channels;
    int32_t images_count;
    uint8_t** images_data;
} Images;

Images generate_font_image(Page page, char* text, CharacterSet* sets, size_t sets_count);
void destroy_images(Images* images);

#endif // IMAGE_H
