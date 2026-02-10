
#include <glyph_utils.h>

uint8_t pixel_luminance(const uint8_t* p, int32_t channels) {
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

int32_t char_to_tile_index(uint8_t c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';           // 0-25
    if (c >= 'a' && c <= 'z') return 26 + (c - 'a');    // 26 - 51

    switch (c) {
        case ' ': return 52;
        case '!': return 53;
        case '?': return 54;
        case '\'': return 55;
        case ',': return 56;
        case ':': return 57;
        case '.': return 58;
        case ';': return 59;
        case '@': return 60;
        case '*': return 61;
        case '(': return 62;
        case ')': return 63;
        case '[': return 64;
        case ']': return 65;
        case '{': return 66;
        case '}': return 67;
        case '&': return 68;
        case '=': return 69;
        case '|': return 70;
        case '-': return 71;
        case '>': return 72;
        case '<': return 73;
        case '#': return 74;
        case '"': return 75;
        case '/': return 76;
        case '\\': return 77;
        case '%': return 78;
        case '$': return 79;
        case '^': return 80;
        case '_': return 81;
        case '+': return 82;
        case 0xC4: return 83;
        case 0xD6: return 84;
        case 0xDC: return 85;
        case 0xE4: return 86;
        case 0xF6: return 87;
        case 0xFC: return 88;
        case 0xDF: return 89;
    }
    
    if (c >= '0' && c <= '9') return 90 + c - '0';

    return 52;
}

uint8_t tile_index_to_char(int32_t index) {
    static const uint8_t table[] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M',
        'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',

        'a','b','c','d','e','f','g','h','i','j','k','l','m',
        'n','o','p','q','r','s','t','u','v','w','x','y','z',

        ' ', '!', '?', '\'', ',', ':', '.', ';', '@', '*',
        '(', ')', '[', ']', '{', '}', '&', '=', '|', '-',
        '>', '<', '#', '"', '/', '\\', '%', '$', '^', '_',
        '+', 0xC4, 0xD6, 0xDC, 0xE4, 0xF6, 0xFC, 0xDF,

        '0','1','2','3','4','5','6','7','8','9'
    };

    if (index < 0 || index >= (int32_t)(sizeof(table)))
        return ' ';

    return table[index];
}

