
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

static inline uint8_t pixel_luminance(const uint8_t* p, int32_t channels) {
    switch (channels) {
        case 1:
            return p[0];
        case 2:
            return (uint8_t)(p[0] * (p[1] / 255.0f));
        case 3:
            return (uint8_t)(
                0.2126 * p[0] +
                0.7152 * p[1] +
                0.0722 * p[2]
            );
        case 4:
            return (uint8_t)(
                (0.2126 * p[0] +
                0.7152 * p[1] +
                0.0722 * p[2]) * (p[3] / 255.0f)
            );
        default:
            return 255;
    }
}

#endif // UTILS_H
