
#include <website/assets.h>

#include <raylib.h>
#include <emscripten/fetch.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct FontData {
    Font* font;
    int32_t fontsize;
    int32_t* codepoints;
    int32_t codepoints_count;
} FontData;

static void download_texture_success(emscripten_fetch_t* fetch) {
    Texture2D* tex = (Texture2D*)fetch->userData;

    Image img = LoadImageFromMemory(".png", fetch->data, fetch->numBytes);
    *tex = LoadTextureFromImage(img);

    emscripten_fetch_close(fetch);
}

static void download_texture_failure(emscripten_fetch_t* fetch) {
    fprintf(stderr, "Failed loading image!\n");
    emscripten_fetch_close(fetch);
    exit(EXIT_FAILURE);
}

void download_texture(Texture2D *tex, const char *path) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = download_texture_success;
    attr.onerror = download_texture_failure;
    attr.userData = tex;

    emscripten_fetch(&attr, path);
}

static void download_font_success(emscripten_fetch_t* fetch) {
    FontData* data = (FontData*)fetch->userData;
    if (!data) {
        fprintf(stderr, "web fetching font failed!\n");
        exit(EXIT_FAILURE);
    }

    *data->font = LoadFontFromMemory(".ttf", fetch->data, fetch->numBytes, data->fontsize, data->codepoints, data->codepoints_count);
    free(data);
}

static void download_font_failure(emscripten_fetch_t* fetch) {
    fprintf(stderr, "Failed loading font!\n");
    emscripten_fetch_close(fetch);
    exit(EXIT_FAILURE);
}

void download_font(Font *font, const char *path, int32_t fontsize, const int32_t* codepoints, int32_t codepoints_count) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = download_font_success;
    attr.onerror = download_font_failure;

    FontData* data = malloc(sizeof(FontData)); // TODO: change to arena alloc?
    if (!data) {
        fprintf(stderr, "Allocation fail!\n");
        exit(EXIT_FAILURE);
    }

    data->font = font;
    data->fontsize = fontsize;
    data->codepoints = codepoints;
    data->codepoints_count = codepoints_count;

    attr.userData = data;

    emscripten_fetch(&attr, path);
}

