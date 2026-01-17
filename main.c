
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <curl/curl.h>
#include <wsJson/ws_json.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct Memory {
    uint8_t* data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

static void add_tile(uint32_t tile_index, uint8_t* input, uint8_t* output, int32_t input_width, int32_t tiles, int32_t offset_x, int32_t offset_y,
                     int32_t char_size, int32_t char_size_squared, int32_t channels, int32_t max_tiles_horizontal) {

    const uint32_t tile_x = tile_index % max_tiles_horizontal;
    const uint32_t tile_y = tile_index / max_tiles_horizontal;

    for (int32_t y = 0; y < char_size; y++) {
        for (int32_t x = 0; x < char_size; x++) {
            for (int32_t c = 0; c < channels; c++) {
                uint32_t dst_x = tile_x * char_size + x;
                uint32_t dst_y = tile_y * char_size + y;
                uint32_t dst_offset = (dst_y * char_size * max_tiles_horizontal + dst_x) * channels + c;

                uint32_t src_offset = ((offset_y + y) * input_width + (offset_x + x)) * channels + c;
                uint8_t src_val = input[src_offset];
                output[dst_offset] = src_val < 120 ? 0 : 255;
            }
        }
    }
}

int main(void) {
    CURL* curl;
    CURLcode res;

    struct Memory chunk = {0};
    
    const char* json =
        "{"
        "\"model\":\"deepseek-r1\","
        "\"prompt\":\"Yooo wsp\","
        "\"stream\":false"
        "}";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "failed to init curl\n");
        return 1;
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
    }
    else {
        const char* json_string_data = (const char*)chunk.data;
        wsJson* root = wsStringToJson(&json_string_data);
        if (!root) {
            fprintf(stderr, "error: json is NULL\n");
            exit(1);
        }
        const char* response = wsJsonGetString(root, "response");
        fprintf(stderr, "Response: %s\n", response);
        
        size_t len = strlen(response);

        // Load Image 
        int32_t width, height, channels;
        uint8_t* data = stbi_load("test.jpeg", &width, &height, &channels, 0);
        if (!data) {
            fprintf(stderr, "failed to load image\n");
            exit(1);
        }

        const int32_t offset_x = 57;
        const int32_t offset_y = 57;
        const int32_t char_size = 60;
        const int32_t char_size_squared = char_size * char_size;
        const int32_t img_size = char_size_squared * channels;
        const int32_t tiles = 20;
        const int32_t tiles_horizontal = 5;

        uint8_t* crop_img = malloc(img_size * tiles);

        size_t i = 0;
        while (response[i] != '\0') {
            unsigned char c = (unsigned char)response[i];
            if (c < 0x80) {
                // ASCII (1 byte)
                add_tile(i, data, crop_img, width, tiles, offset_x, offset_y, char_size, char_size_squared, channels, tiles_horizontal);
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                // 2-byte UTF-8
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                // 3-byte UTF-8
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                // 4-byte UTF-8 (emoji)
                i += 4;
            } else {
                // Invalid byte, skip
                i += 1;
            }
        }

        stbi_write_jpg("damn.jpg", char_size * tiles_horizontal, char_size * (tiles / tiles_horizontal), channels, crop_img, 90);

        wsJsonFree(root);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    free(chunk.data);
    curl_global_cleanup();

    return 0;
}

