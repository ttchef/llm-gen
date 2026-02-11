
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

enum PageBackgroundType {
    PAGE_BG_TYPE_LINES,
    PAGE_BG_TYPE_SQUARES,
};

typedef struct PageDimensions {
    int32_t width;
    int32_t height;
} PageDimensions;

typedef struct PagePadding {
    int32_t x;
    int32_t y;
} PagePadding;

typedef struct Page {
    enum PageBackgroundType bg_type;
    PageDimensions dim;
    PagePadding padding;
} Page;

#endif // UTILS_H
