
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct PageDimensions {
    int32_t width;
    int32_t height;
} PageDimensions;

typedef struct PagePadding {
    int32_t x;
    int32_t y;
} PagePadding;

typedef struct Page {
    PageDimensions dim;
    PagePadding padding;
} Page;

#endif // UTILS_H
