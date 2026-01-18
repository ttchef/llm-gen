
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

const int32_t font_widths[256] = {
    ['A'] = 667, ['B'] = 667, ['C'] = 722, ['D'] = 722, ['E'] = 667,
    ['F'] = 611, ['G'] = 778, ['H'] = 722, ['I'] = 348, ['J'] = 500,
    ['K'] = 667, ['L'] = 556, ['M'] = 833, ['N'] = 722, ['O'] = 878,
    ['P'] = 667, ['Q'] = 778, ['R'] = 722, ['S'] = 667, ['T'] = 611,
    ['U'] = 722, ['V'] = 667, ['W'] = 944, ['X'] = 667, ['Y'] = 867,
    ['Z'] = 611,
    
    ['a'] = 556, ['b'] = 556, ['c'] = 500, ['d'] = 556, ['e'] = 556,
    ['f'] = 278, ['g'] = 556, ['h'] = 456, ['i'] = 222, ['j'] = 322,
    ['k'] = 500, ['l'] = 422, ['m'] = 633, ['n'] = 556, ['o'] = 576,
    ['p'] = 556, ['q'] = 556, ['r'] = 583, ['s'] = 530, ['t'] = 478,
    ['u'] = 556, ['v'] = 700, ['w'] = 722, ['x'] = 500, ['y'] = 650,
    ['z'] = 800,
    
    ['0'] = 756, ['1'] = 556, ['2'] = 556, ['3'] = 556, ['4'] = 556,
    ['5'] = 556, ['6'] = 556, ['7'] = 556, ['8'] = 556, ['9'] = 556,
    
    [' '] = 578,  
    ['!'] = 278, ['"'] = 355, ['#'] = 856, ['$'] = 556, ['%'] = 889,
    ['&'] = 667, ['\''] = 191, ['('] = 633, [')'] = 633, ['*'] = 629,
    ['+'] = 784, [','] = 278, ['-'] = 333, ['.'] = 278, ['/'] = 878,
    [':'] = 378, [';'] = 378, ['<'] = 584, ['='] = 584, ['>'] = 584,
    ['?'] = 556, ['@'] = 1015,
    ['['] = 578, ['\\'] = 878, [']'] = 578, ['^'] = 469, ['_'] = 556,
    ['`'] = 333, ['{'] = 454, ['|'] = 260, ['}'] = 454, ['~'] = 584,
    [0xC4] = 668, // Ä
    [0xD6] = 878, // Ö
    [0xDC] = 722, // Ü
    [0xE4] = 556, // ä
    [0xF6] = 576, // ö
    [0xFC] = 556, // ü
    [0xDF] = 556, // ß
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
    bool in_word;
    uint32_t input_width;
    uint32_t tiles;
    uint32_t base_x;
    uint32_t base_y;
    uint32_t offset_x;
    uint32_t offset_y;
    uint32_t char_size;
    uint32_t channels;
    uint32_t usable_pixels_horizontal;
    uint32_t max_pixels_horizontal;
    uint32_t table_sym_horizontal;
    uint32_t grid_width;
    uint32_t grid_height;
    uint32_t padding_x; // padding_y is hardcoded
    uint32_t rows_count;
};

static void add_char(uint8_t c, struct TileInfo* info);

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

static inline int32_t get_char_width(uint8_t c) {
    int32_t width = font_widths[c];
    if (width == 0) return 500;
    return width;
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

        float relative_width = get_char_width((uint8_t)(text[i])) / 1000.0f;
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

static int32_t char_to_tile_index(uint8_t c) {
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

static void add_tile(struct TileInfo* info) {
    const float relative_font_width = get_char_width(info->character) / 1000.0f;
    const uint32_t pixels_char_horizontal = info->char_size * relative_font_width;
    const uint32_t atlas_center_offset = info->char_size / 2 - pixels_char_horizontal / 2;

    if (info->current_x + pixels_char_horizontal >= info->usable_pixels_horizontal) {
        info->current_y += info->char_size;
        info->current_x = 0;
    }

    if (info->current_x == 0) info->current_x = info->padding_x;

    for (int32_t y = 0; y < info->char_size; y++) {
        for (int32_t x = 0; x < pixels_char_horizontal; x++) {
            for (int32_t c = 0; c < info->channels; c++) {
                uint32_t dst_x = info->current_x + x;
                uint32_t dst_y = info->current_y + y;
                uint32_t dst_offset = (dst_y * info->max_pixels_horizontal + dst_x) * info->channels + c;

                uint32_t src_x = info->offset_x + x + atlas_center_offset;
                uint32_t src_y = info->offset_y + y;
                uint32_t src_offset = (src_y * info->input_width + src_x) * info->channels + c;

                uint8_t src_val = info->input[src_offset];
                if (src_val < 120) info->output[dst_offset] = src_val;
            }
        }
    }

    info->current_x += pixels_char_horizontal;
}

static void add_char(uint8_t c, struct TileInfo* info) {
    int32_t index = char_to_tile_index(c);
    info->offset_x = info->base_x + (index % info->table_sym_horizontal) * info->char_size;
    info->offset_y = info->base_y + (index / info->table_sym_horizontal) * info->char_size;
    info->character = c;

    add_tile(info);
}

static void skip_unicode(char* out, size_t* size, const char* input) {
    size_t i = 0;
    size_t out_index = 0;

    while (input[i] != '\0') {
        unsigned char c = (unsigned char)input[i];
        if (c < 0x80) {
            // ASCII (1 byte)
            if (out && out_index < *size - 1) {
                if (c == '\\' && input[i + 1] == 'u' && 
                input[i + 2] && input[i + 3] && input[i + 4] && input[i + 5]) {

                    char s[5];
                    s[0] = input[i + 2];
                    s[1] = input[i + 3];
                    s[2] = input[i + 4];
                    s[3] = input[i + 5];
                    s[4] = '\0';
                    uint32_t hex_val = strtoul(s, NULL, 16);
                    if (hex_val < 128) { // TODO: change later to extended
                        out[out_index] = (char)hex_val;
                        out_index++;
                    }
                    i += 6;
                    continue;
                }
                out[out_index] = c;
            }
            out_index++;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            // german ä ö etc..
            if (input[i + 1]) {
                uint8_t c1 = (uint8_t)input[i];
                uint8_t c2 = (uint8_t)input[i + 1];

                uint32_t codepoint = ((c1 & 0x1F) << 6) | (c2 & 0x3F);

                if (codepoint < 256 && out && out_index < *size - 1) {
                    out[out_index] = (uint8_t)codepoint;
                }
                out_index++;
                i += 2;
            }
            else {
                i += 2;
            }
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

static void draw_background(struct TileInfo* info, bool grid) {
    // Horizontal
    for (int32_t y = 0; y < info->grid_height; y += info->char_size) {
        for (int32_t x = 0; x < info->grid_width; x++) {
            for (int32_t c = 0; c < info->channels; c++) {
                info->output[(y * info->max_pixels_horizontal + x) * info->channels + c] = 211;
            }
        }
    }

    if (!grid) return;
    for (int32_t y = 0; y < info->grid_height; y++) {
        for (int32_t x = 0; x < info->grid_width; x += info->char_size) {
            for (int32_t c = 0; c < info->channels; c++) {
                info->output[(y * info->max_pixels_horizontal + x) * info->channels + c] = 211;
            }
        }
    }
}

static wsJson* get_ai_json() {
    CURL* curl;
    CURLcode res;

    struct Memory chunk = {0};

    const char* json =
        "{"
        "\"model\":\"deepseek-r1\","
        "\"prompt\":\"explain simple what minecraft hypixel bedwars is!  make use of paragraphs DO NOT USE MARKDOWN AND ONLY ASCII AND WRITE IN GERMAN\","
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

    curl_global_cleanup();

    const char* json_string_data = (const char*)chunk.data;
    wsJson* root = wsStringToJson(&json_string_data);
    free(chunk.data);
    return root;
}

int main(void) {
    wsJson* root = NULL;
    char* response = NULL;
    if (generate_ai_answer) {
        root = get_ai_json();
        response = wsJsonGetString(root, "response");
        fprintf(stderr, "Response: %s\n", response);
    }
    else {
        response = "Pointers in C are variables that store memory addresses. When used with structures, they allow efficient access to structure members and enable dynamic memory management. Here's a breakdown:\n\n### 1. **Structure Definition**\n   ```c\n   struct Person {\n       char name[50];\n       int age;\n   };\n   ```\n\n### 2. **Structure Variable vs. Pointer to Structure**\n   - **Direct Access**: `struct Person john = {\"John\", 30}; john.age++;`  \n     Access members with `.` operator.\n   - **Pointer Access**: `struct Person *ptr = \u0026john; ptr-\u003eage++;`  \n     Use `-\u003e` (combines dereferencing and member access).\n\n### 3. **Dereferencing with `-\u003e`**\n   - The `-\u003e` operator is shorthand for `(*ptr).member`.  \n     Example: `ptr-\u003eage` is equivalent to `(*ptr).age`.\n\n### 4. **Dynamic Memory Allocation**\n   - Allocate memory for a structure dynamically:\n     ```c\n     struct Person *ptr = malloc(sizeof(struct Person));\n     ```\n   - Check allocation success:\n     ```c\n     if (ptr == NULL) { /* Handle error */ }\n     ```\n   - Free memory when done:\n     ```c\n     free(ptr);\n     ```\n\n### 5. **Function Parameters**\n   - Pass structure pointers to functions to modify the original structure without returning it:\n     ```c\n     void updateAge(struct Person *p) {\n         p-\u003eage++;\n     }\n     ```\n\n### 6. **Linked Structures**\n   - Pointers enable linked data structures (e.g., linked lists):\n     ```c\n     struct Node {\n         int data;\n         struct Node *next;\n     };\n\n     struct Node *head = malloc(sizeof(struct Node));\n     head-\u003enext = malloc(sizeof(struct Node));\n     head-\u003enext-\u003enext = NULL;\n     ```\n\n### 7. **Memory Layout**\n   - Structures are stored in contiguous memory. Pointers reference the entire structure, not individual members.\n\n### 8. **Key Rules**\n   - Use `\u0026` to get a pointer to a structure.\n   - Use `-\u003e` to access members via a pointer.\n   - Always free dynamically allocated memory to avoid leaks.\n\n### Example Code:\n```c\n#include \u003cstdio.h\u003e\n#include \u003cstdlib.h\u003e\n#include \u003cstring.h\u003e\n\nstruct Employee {\n    char name[50];\n    int id;\n};\n\nvoid printEmployee(struct Employee *emp) {\n    printf(\"Name: %s, ID: %d\\n\", emp-\u003ename, emp-\u003eid);\n}\n\nint main() {\n    struct Employee emp1 = {\"Alice\", 101};\n    struct Employee *emp_ptr = \u0026emp1;\n\n    printEmployee(emp_ptr); // Output: Name: Alice, ID: 101\n\n    // Dynamic allocation\n    struct Employee *emp2 = malloc(sizeof(struct Employee));\n    if (emp2 == NULL) exit(1);\n    strcpy(emp2-\u003ename, \"Bob\");\n    emp2-\u003eid = 102;\n    printEmployee(emp2);\n    free(emp2);\n\n    return 0;\n}\n```\n\nPointers to structures streamline memory usage, enable efficient data manipulation, and are essential for advanced C programming.";
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
    uint8_t asci_string[asci_string_len];
    skip_unicode(asci_string, &asci_string_len, response);

    // free old string resources
    if (root) wsJsonFree(root);

    fprintf(stderr, "%s\n", asci_string);
    fprintf(stderr, "str len %zu\n", asci_string_len);

    const uint32_t offset_x = 58;
    const uint32_t offset_y = 58;
    const uint32_t char_size = 58;
    const uint32_t char_size_squared = char_size * char_size;
    const uint32_t img_size = char_size_squared * channels;
    const uint32_t tiles = asci_string_len - 1;

    const uint32_t padding_x = char_size;
    const uint32_t pixels_horizontal = 50 * char_size;
    const uint32_t usable_pixels_horizontal = pixels_horizontal - 2 * padding_x;

    uint32_t rows = count_rows(asci_string, asci_string_len, char_size, usable_pixels_horizontal);
    uint32_t grid_width = pixels_horizontal;
    uint32_t grid_height = char_size * rows + char_size * 2;
    size_t total_bytes = (size_t)grid_width * (size_t)grid_height * (size_t)channels;;
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
        .usable_pixels_horizontal = usable_pixels_horizontal,
        .max_pixels_horizontal = pixels_horizontal,
        .table_sym_horizontal = 26,
        .grid_width = grid_width,
        .grid_height = grid_height,
        .padding_x = padding_x,
        .rows_count = rows,
        .current_y = char_size,
    };

    draw_background(&info, false);
    for (int32_t i = 0; i < tiles; i++) {
        if (asci_string[i] == '\n' || (asci_string[i] == '\\' && asci_string[i + 1] == 'n')) {
            if (asci_string[i] == '\\') i++;
            info.current_y += info.char_size;
            info.current_x = 0;
            continue;
        }

        if (asci_string[i] && asci_string[i] != ' ') info.in_word = true;
        else info.in_word = false;
 
        // check end to dash
        const float relative_font_width = get_char_width(asci_string[i]) / 1000.0f;
        const uint32_t pixels_char_horizontal = info.char_size * relative_font_width;
        if (info.current_x + pixels_char_horizontal * 2 >= info.max_pixels_horizontal && info.in_word) {
            add_char('-', &info);
            info.current_x = 0;
            info.current_y += info.char_size;
        }

        add_char(asci_string[i], &info);
    }

    stbi_write_jpg("damn.jpg", grid_width, grid_height, channels, crop_img, 90);

    return 0;
}

