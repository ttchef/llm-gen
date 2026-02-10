
#include <generate.h>
#include <containers/darray.h>

#include <curl/curl.h>

#include <ctype.h>

#define JSON_AI_INPUT_HEADER_SIZE 128

struct Memory {
    uint8_t* data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    uint8_t *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

int32_t generate_ai_answer(char **text, wsJson **out_json) {
    CURL* curl;
    CURLcode res;

    struct Memory chunk = {0};

    char* text_string = darrayCreate(char);
    for (int32_t i = 0; i < darrayLength(text); i++) {
        for (int32_t j = 0; j < strlen(text[i]); j++) {
            if (isprint((uint8_t)text[i][j])) {
                darrayPush(text_string, text[i][j]);
            }
        }
        darrayPush(text_string, ' ');
    }
    darrayPush(text_string, '\0');

    int32_t needed_size = snprintf(NULL, 0,
                                   "{"
                                   "\"model\":\"deepseek-r1\","
                                   "\"prompt\":\"%s\","
                                   "\"stream\":false"
                                   "}", text_string);
    if (needed_size < 0) {
        fprintf(stderr, "snprintf failed\n");
        darrayDestroy(text_string);
        return 1;
    }

    char* buffer = malloc((size_t)needed_size + 1);
    if (!buffer) {
        fprintf(stderr, "failed to allocate input string buffer\n");
        return 1;
    }

    snprintf(buffer, needed_size + 1, 
             "{"
             "\"model\":\"deepseek-r1\","
             "\"prompt\":\"%s\","
             "\"stream\":false"
             "}", text_string);
    fprintf(stderr, "[DEBUG] input: %s\n", buffer);
    
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
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        free(chunk.data);
        curl_global_cleanup();
        return 1;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    curl_global_cleanup();

    const char* json_string_data = (const char*)chunk.data;
    *out_json = wsStringToJson(&json_string_data);
    free(chunk.data);
    darrayDestroy(text_string);

    return 0;
}

