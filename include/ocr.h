
#ifndef OCR_H
#define OCR_H

#include <mupdf/fitz.h>

enum ImageType {
    IMAGE_TYPE_PPM,
    IMAGE_TYPE_STBI,
};

typedef struct Image {
    enum ImageType type;
    union {
        fz_pixmap* pix;
        struct {
            int32_t width;
            int32_t height;
            uint8_t* data;
        } stbi;
    } data;
} Image;

char* string_from_img(Image* img);

#endif // OCR_H
