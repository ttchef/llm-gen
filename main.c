
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h> 
#include <curl/curl.h>
#include <wsJson/ws_json.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const bool generate_ai_answer = false;

const int32_t arial_widths[128] = {
    ['A'] = 667, ['B'] = 667, ['C'] = 722, ['D'] = 722, ['E'] = 667,
    ['F'] = 611, ['G'] = 778, ['H'] = 722, ['I'] = 278, ['J'] = 500,
    ['K'] = 667, ['L'] = 556, ['M'] = 833, ['N'] = 722, ['O'] = 778,
    ['P'] = 667, ['Q'] = 778, ['R'] = 722, ['S'] = 667, ['T'] = 611,
    ['U'] = 722, ['V'] = 667, ['W'] = 944, ['X'] = 667, ['Y'] = 667,
    ['Z'] = 611,
    
    ['a'] = 556, ['b'] = 556, ['c'] = 500, ['d'] = 556, ['e'] = 556,
    ['f'] = 278, ['g'] = 556, ['h'] = 556, ['i'] = 222, ['j'] = 222,
    ['k'] = 500, ['l'] = 222, ['m'] = 833, ['n'] = 556, ['o'] = 556,
    ['p'] = 556, ['q'] = 556, ['r'] = 333, ['s'] = 500, ['t'] = 278,
    ['u'] = 556, ['v'] = 500, ['w'] = 722, ['x'] = 500, ['y'] = 500,
    ['z'] = 500,
    
    ['0'] = 556, ['1'] = 556, ['2'] = 556, ['3'] = 556, ['4'] = 556,
    ['5'] = 556, ['6'] = 556, ['7'] = 556, ['8'] = 556, ['9'] = 556,
    
    [' '] = 678,  
    ['!'] = 278, ['"'] = 355, ['#'] = 556, ['$'] = 556, ['%'] = 889,
    ['&'] = 667, ['\''] = 191, ['('] = 333, [')'] = 333, ['*'] = 389,
    ['+'] = 584, [','] = 278, ['-'] = 333, ['.'] = 278, ['/'] = 278,
    [':'] = 278, [';'] = 278, ['<'] = 584, ['='] = 584, ['>'] = 584,
    ['?'] = 556, ['@'] = 1015,
    ['['] = 278, ['\\'] = 278, [']'] = 278, ['^'] = 469, ['_'] = 556,
    ['`'] = 333, ['{'] = 334, ['|'] = 260, ['}'] = 334, ['~'] = 584
};

struct Memory {
    uint8_t* data;
    size_t size;
};

struct TileInfo {
    uint32_t current_x;
    uint32_t current_y;
    uint8_t* input;
    uint8_t* output;
    uint8_t character;
    int32_t input_width;
    int32_t tiles;
    int32_t base_x;
    int32_t base_y;
    int32_t offset_x;
    int32_t offset_y;
    int32_t char_size;
    int32_t channels;
    int32_t max_pixels_horizontal;
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

static int32_t count_rows(const char* text, size_t len, int32_t char_size, int32_t pixels_horizontal) {
    int32_t current_x = 0;
    int32_t rows = 1;

    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n' || (text[i] == '\\' && text[i + 1] == 'n')) {
            if (text[i] == '\\') i++;
            rows++;
            current_x = 0;
            continue;
        }

        float relative_width = arial_widths[(uint8_t)text[i]] / 1000.0f;
        int32_t pixels_char = char_size * relative_width;

        if (current_x + pixels_char >= pixels_horizontal) {
            rows++;
            current_x = pixels_char;
        }
        else {
            current_x += pixels_char;
        }
    }

    return rows;
}

static int32_t char_to_tile_index(char c) {
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
    }
    
    if (c >= '0' && c <= '9') return 82 + c - '0';

    return 52;
}

static void add_tile(struct TileInfo* info) {
    float relative_font_width = arial_widths[info->character] / 1000.0f;
    int32_t pixels_char_horizontal = info->char_size * relative_font_width;

    for (int32_t y = 0; y < info->char_size; y++) {
        for (int32_t x = 0; x < pixels_char_horizontal; x++) {
            for (int32_t c = 0; c < info->channels; c++) {
                uint32_t dst_x = info->current_x + x;
                uint32_t dst_y = info->current_y + y;
                uint32_t dst_offset = (dst_y * info->max_pixels_horizontal + dst_x) * info->channels + c;

                uint32_t src_x = info->offset_x + x + pixels_char_horizontal / 2;
                uint32_t src_y = info->offset_y + y;
                uint32_t src_offset = (src_y * info->input_width + src_x) * info->channels + c;

                uint8_t src_val = info->input[src_offset];
                if (src_val < 120) info->output[dst_offset] = src_val;
            }
        }
    }

    uint32_t new_x = info->current_x + info->char_size * relative_font_width;
    if (new_x >= info->max_pixels_horizontal) {
        info->current_y += info->char_size;
        info->current_x = 0;
    }
    else {
        info->current_x = new_x;
    }
}

static void add_char(char c, struct TileInfo* info) {
    int32_t index = char_to_tile_index(c);
    info->offset_x = info->base_x + (index % info->table_sym_horizontal) * info->char_size;
    info->offset_y = info->base_y + (index / info->table_sym_horizontal) * info->char_size;
    info->character = c;

    add_tile(info);

    /*
    fprintf(stderr, "Char '%c' (idx=%2d) atlas(%d,%d) -> output tile %d\n", 
            c, index, 
            info->offset_x, info->offset_y,
            info->tile_index);
    */
}

static void skip_unicode(char* out, size_t* size, const char* input) {
    size_t i = 0;
    size_t out_index = 0;

    while (input[i] != '\0') {
        unsigned char c = (unsigned char)input[i];
        if (c < 0x80) {
            // ASCII (1 byte)
            if (out && out_index < *size - 1) {
                if (c == '\\' && input[i + 1] == 'u') {
                    char s[5];
                    s[0] = input[i + 2];
                    s[1] = input[i + 3];
                    s[2] = input[i + 4];
                    s[3] = input[i + 5];
                    s[4] = '\0';
                    uint32_t hex_val = strtoul(s, NULL, 16);
                    c = hex_val;
                    i += 5;
                }
                out[out_index] = c;
            }
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

static wsJson* get_ai_json() {
    CURL* curl;
    CURLcode res;

    struct Memory chunk = {0};

    const char* json =
        "{"
        "\"model\":\"deepseek-r1\","
        "\"prompt\":\"explain to me how pointers work in c with structures! DO NOT USE MARKDOWN AND ONLY ASCII\","
        "\"stream\":false"
        "}";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "failed to init curl\n");
        return NULL;
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

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        free(chunk.data);
        curl_global_cleanup();
        return NULL;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    free(chunk.data);
    curl_global_cleanup();

    const char* json_string_data = (const char*)chunk.data;
    wsJson* root = wsStringToJson(&json_string_data);
    return root;
}

int main(void) {
    char* response = NULL;
    if (generate_ai_answer) {
        wsJson* root = get_ai_json();
        response = wsJsonGetString(root, "response");
        fprintf(stderr, "Response: %s\n", response);
        wsJsonFree(root);
    }
    else {
        response = "Yooo im good how are you\n12345:!!!?<>";
    }

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

    const int32_t offset_x = 57;
    const int32_t offset_y = 57;
    const int32_t char_size = 58;
    const int32_t char_size_squared = char_size * char_size;
    const int32_t img_size = char_size_squared * channels;
    const int32_t tiles = asci_string_len - 1;
    const int32_t pixels_horizontal = 50 * char_size;

    int32_t grid_width = pixels_horizontal;
    int32_t grid_height = char_size * count_rows(asci_string, asci_string_len, char_size, pixels_horizontal);
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
        .max_pixels_horizontal = pixels_horizontal,
        .table_sym_horizontal = 26,
    };

    for (int32_t i = 0; i < tiles; i++) {
        if (asci_string[i] == '\n' || (asci_string[i] == '\\' && asci_string[i + 1] == 'n')) {
            if (asci_string[i] == '\\') i++;
            info.current_y += info.char_size;
            info.current_x = 0;
            i++;
        }
        add_char(asci_string[i], &info);
    }

    stbi_write_jpg("damn.jpg", grid_width, grid_height, channels, crop_img, 90);

    return 0;
}

