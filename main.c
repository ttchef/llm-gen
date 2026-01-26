
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

#include "array.h"
#include "utils.h"

const bool generate_ai_answer = false;

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
    float char_size;
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

static inline uint32_t get_char_width(uint8_t c) {
    uint32_t width = font_widths[c].width;
    return width;
}

static inline int32_t get_char_offset(uint8_t c) {
    int32_t offset = font_widths[c].offset;
    return offset;

}

static int32_t count_rows(const char* text, size_t len, float char_size, int32_t pixels_horizontal) {
    int32_t current_x = 0;
    int32_t rows = 1;

    for (size_t i = 0; i < len; i++) {
        if (text[i] == '\n' || (text[i] == '\\' && text[i + 1] == 'n')) {
            if (text[i] == '\\') i++;
            rows++;
            current_x = 0;
            continue;
        }

        int32_t pixels_char = get_char_width((uint8_t)text[i]);

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

static void add_tile(struct TileInfo* info) {
    const uint32_t pixels_char_horizontal = get_char_width(info->character);
    const int32_t char_offset = get_char_offset(info->character);
    const uint32_t atlas_center_offset = info->char_size / 2 - pixels_char_horizontal / 2;
    const int32_t tile_center_x = info->offset_x + (info->char_size / 2);

    if (info->current_x + pixels_char_horizontal >= info->usable_pixels_horizontal) {
        info->current_y += info->char_size;
        info->current_x = 0;
    }

    if (info->current_x == 0) info->current_x = info->padding_x;

    for (int32_t y = 0; y < info->char_size; y++) {
        for (int32_t x = 0; x < pixels_char_horizontal; x++) {
            uint32_t dst_x = info->current_x + x;
            uint32_t dst_y = info->current_y + y;
            uint32_t dst_offset = (dst_y * info->max_pixels_horizontal + dst_x) * info->channels;

            uint32_t src_x = info->offset_x + (info->char_size / 2) - pixels_char_horizontal / 2 + x;
            uint32_t src_y = info->offset_y + y;
            uint32_t src_offset = (src_y * info->input_width + src_x) * info->channels;

            uint8_t* pixel = &info->input[src_offset];
            uint8_t luminance = pixel_luminance(pixel, info->channels);

            if (luminance < 120) {
                for (int32_t c = 0; c < info->channels; c++) {
                        info->output[dst_offset + c] = luminance;
                }
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
        "\"prompt\":\"Hier sind die wichtigsten Informationen für deine Aufgaben kompakt zusammengefasst:Zu Aufgabe 2: Wann ethischer Relativismus nach Law zulässig ist (M2)Ethischer Relativismus ist laut Stephen Law dann zulässig und begründet, wenn man erkennt, dass westliche Gesellschaften historisch dazu neigten, anderen ihre Moralvorstellungen aufzuzwingen. Er ist zulässig, weil es keine von moralsystemen unabhängigen Tatsachen gibt, die beweisen könnten, dass eine Handlung absolut richtig oder falsch ist. Law sieht ihn als berechtigt an, sobald man akzeptiert, dass der eigene moralische Standpunkt nur einer unter vielen ist und ständigen Wandlungen unterliegt. Zudem ist er Ausdruck von moralischer Bescheidenheit, da es als pure Überheblichkeit gilt, die eigene Sichtweise gegenüber anderen Kulturen als allgemeingültig zu behaupten.Zu Aufgabe 4: Bewertung von Feyerabends Position zu barbarischen Traditionen (M3)Für die Bewertung von Paul Feyerabends Position sind folgende Punkte zentral: Er bezeichnet den Relativismus als zivilisiert und klug, da er keine Kultur zum Nabel der Welt erhebt. Feyerabend argumentiert, dass Traditionen weder gut noch schlecht sind, sondern einfach existieren. Er provoziert mit der Ansicht, dass die Verurteilung von rituellen Tötungen oder Folter oft auf einer oberflächlichen und subjektiven Denkweise basiert. Seine Position besagt, dass man eine fremde Tradition nicht nach universellen Kriterien wie Glück oder Leiden bewerten darf, da diese Kriterien selbst nur Teil der eigenen (westlichen) Tradition sind. Wer versucht, humanitäre Standards als allgemeingültig vorzuschreiben, handelt laut Feyerabend wie ein gebildeter Sklavenhalter, der die Zerstörung fremder Werte billigend in Kauf nimmt. Aufgabe 2: Erklären Sie, wann nach Law ein ethischer Relativismus zulässig ist. (Bezieht sich auf Text M2)Aufgabe 4: Bewerten Sie Feyerabends Position im Hinblick auf barbarische Traditionen wie rituelle Tötungen, Folterungen, Verbrennungen usw. (Bezieht sich auf Text M3). Mache aufgabe 2 und 4 schrteibe in deutsch wie ein schüler. Benutze kein markdown und schreibe in paragraphen! Auch keine Emojies pures extended ascii\","
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

int32_t main(int32_t argc, char** argv) {
    wsJson* root = NULL;
    char* response = NULL;
    if (generate_ai_answer) {
        root = get_ai_json();
        response = wsJsonGetString(root, "response");
        fprintf(stderr, "Response: %s\n", response);
    }
    else {
        response = "Okay, hier ist eine einfache Erklärung des Sports \"Caletics\" in einfachem Deutsch, in Absätzen ohne Sonderzeichen:\n\nCaletics ist ein Krafttrainingssystem, das man zu Hause machen kann.\n\nMan benutzt dafür ein bestimmtes Gerät, das wie eine Stange mit zwei Armen aussehen könnte, an denen man Gewichte anbringen kann. Es gibt verschiedene Arten von Caletics-Geräten, aber sie haben meist etwas Gemeinsames.\n\nDie besondere Sache an Caletics ist, dass es oft mit Übungen wie Deadlifts, Bench Press (Brustdrücken) und Kniebeugen funktioniert. Aber anders als mit traditionellen Barren oder Kettlebells, wird beim Heben und Senken der Gewichte oft mit einem speziellen Seil gearbeitet. Dieses Seil verhindert, dass die Gewichte zu schnell nach unten fallen.\n\nDas Training auf dem Caletics-Gerät soll besonders effektiv sein für das Muskelaufbau und für das Vermeiden von Rückenverletzungen, besonders wenn man diese Übungen selbst zu Hause versuchen möchte. Es wird von Jonas Steinhoff, einem deutschen Profikletterer, stark beworben.\n\nMan kann damit viele verschiedene Muskelgruppen trainieren, ohne viel Equipment zu benötigen.";
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
    const float char_size = 58.601f;
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

        if (asci_string[i + 1] && asci_string[i + 1] != ' ') info.in_word = true;
        else info.in_word = false;
 
        // check end to dash
        const uint32_t pixels_char_horizontal = get_char_width(asci_string[i]);
        if (info.current_x + pixels_char_horizontal * 4 >= info.usable_pixels_horizontal && info.in_word) {
            add_char('-', &info);
            info.current_x = 0;
            info.current_y += info.char_size;
            continue;
        }

        add_char(asci_string[i], &info);
    }

    stbi_write_jpg("damn.jpg", grid_width, grid_height, channels, crop_img, 90);

    return 0;
}

