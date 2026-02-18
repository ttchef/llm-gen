
#ifndef ASSETS_H
#define ASSETS_H

#include <raylib.h>
#include <stdint.h>

void download_texture(Texture2D* tex, const char* path);
void download_font(Font* font, const char* path, int32_t fontsize, const int32_t* codepoints, int32_t codepoints_count);

#endif // ASSETS_H
