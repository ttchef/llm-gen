
#include <glyph.h>
#include <containers/darray.h>

#include <stb_image.h>

int32_t generate_glyphs(CharacterSet *sets, char **fonts) {
    for (int32_t i = 0; i < darrayLength(fonts); i++) {
        int32_t img_width, img_height, img_channels;
        uint8_t* font_data = stbi_load(fonts[i], &img_width, &img_height, &img_channels, 4);
        if (!font_data) {
            fprintf(stderr, "failed to load font template: %s\n", fonts[i]);
            return 1;
        }

        
    }
    return 0;
}

