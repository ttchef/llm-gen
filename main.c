
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

struct TileInfo {
    uint32_t tile_index;
    uint8_t* input;
    uint8_t* output;
    int32_t input_width;
    int32_t tiles;
    int32_t base_x;
    int32_t base_y;
    int32_t offset_x;
    int32_t offset_y;
    int32_t char_size;
    int32_t channels;
    int32_t max_tiles_horizontal;
    int32_t table_sym_horizontal;
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

static int32_t char_to_tile_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';           // 0-25
    if (c >= 'a' && c <= 'z') return 26 + (c - 'a');    // 26 - 51
    if (c == ' ') return 52;
    return 0;
}

static void add_tile(struct TileInfo* info) {
    const uint32_t tile_x = info->tile_index % info->max_tiles_horizontal;
    const uint32_t tile_y = info->tile_index / info->max_tiles_horizontal;

    for (int32_t y = 0; y < info->char_size; y++) {
        for (int32_t x = 0; x < info->char_size; x++) {
            for (int32_t c = 0; c < info->channels; c++) {
                uint32_t dst_x = tile_x * info->char_size + x;
                uint32_t dst_y = tile_y * info->char_size + y;
                uint32_t dst_offset = (dst_y * info->char_size * info->max_tiles_horizontal + dst_x) * info->channels + c;

                // Debug: Check if we're writing out of bounds
               int32_t total_size = info->char_size * info->max_tiles_horizontal * 
                     info->char_size * ((info->tiles + info->max_tiles_horizontal - 1) / info->max_tiles_horizontal) * 
                     info->channels;
                if (dst_offset >= total_size) {
                    fprintf(stderr, "OUT OF BOUNDS! tile=%d, offset=%d, max=%d\n", 
                            info->tile_index, dst_offset, total_size);
                }

                uint32_t src_offset = ((info->offset_y + y) * info->input_width + (info->offset_x + x)) * info->channels + c;
                uint8_t src_val = info->input[src_offset];
                info->output[dst_offset] = src_val < 120 ? 0 : 255;
            }
        }
    }
}

static void add_char(char c, struct TileInfo* info) {
    int32_t index = char_to_tile_index(c);
    info->offset_x = info->base_x + (index % info->table_sym_horizontal) * info->char_size;
    info->offset_y = info->base_y + (index / info->table_sym_horizontal) * info->char_size;
    add_tile(info);
}

static void skip_unicode(char* out, size_t* size, const char* input) {
    size_t i = 0;
    size_t out_index = 0;

    while (input[i] != '\0') {
        unsigned char c = (unsigned char)input[i];
        if (c < 0x80) {
            // ASCII (1 byte)
            if (out && out_index < *size - 1) out[out_index] = c;
            out_index++;
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
    if (out) out[out_index] = '\0'; 
    *size = out_index + 1;
}

int main(void) {
    CURL* curl;
    CURLcode res;

    struct Memory chunk = {0};
    
    const char* json =
        "{"
        "\"model\":\"deepseek-r1\","
        "\"prompt\":\"yooo how are you doing my friend!\","
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
        printf("Channels: %d\n", channels);

        size_t asci_string_len;
        skip_unicode(NULL, &asci_string_len, response);
        char asci_string[asci_string_len];
        skip_unicode(asci_string, &asci_string_len, response);

        fprintf(stderr, "%s\n", asci_string);
        fprintf(stderr, "str len %zu\n", asci_string_len);

        const int32_t offset_x = 58;
        const int32_t offset_y = 58;
        const int32_t char_size = 60;
        const int32_t char_size_squared = char_size * char_size;
        const int32_t img_size = char_size_squared * channels;
        const int32_t tiles = asci_string_len - 1;
        const int32_t tiles_horizontal = 10;


        const int32_t rows = (tiles + tiles_horizontal - 1) / tiles_horizontal;
        int32_t grid_width = char_size * tiles_horizontal;
        int32_t grid_height = char_size * rows;
        size_t total_bytes = (size_t)grid_width * (size_t)grid_height * (size_t)channels;
        uint8_t* crop_img = malloc(total_bytes);
        memset(crop_img, 255, grid_width * grid_height * channels);

        struct TileInfo info = {
            .input = data,
            .output = crop_img,
            .input_width = width,
            .tiles = tiles,
            .base_x = offset_x,
            .base_y = offset_y,
            .offset_x = offset_x,
            .offset_y = offset_y,
            .char_size = char_size,
            .channels = channels,
            .max_tiles_horizontal = tiles_horizontal,
            .table_sym_horizontal = 26,
        };

        for (int32_t i = 0; i < tiles; i++) {
            info.tile_index = i; 
            add_char(asci_string[i], &info);
        }
       
        stbi_write_jpg("damn.jpg", char_size * tiles_horizontal, char_size * rows, channels, crop_img, 90);

        wsJsonFree(root);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    free(chunk.data);
    curl_global_cleanup();

    return 0;
}

