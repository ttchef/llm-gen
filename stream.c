
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <curl/curl.h>
#include <wsJson/ws_json.h>

struct Memory {
    uint8_t* data;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    uint8_t* data = contents;

    for (size_t i = 0; i < realsize; i++) {
        putchar(data[i]);
    }

    fflush(stdout);

    return realsize;
}


int main(void) {
    CURL* curl;
    CURLcode res;

    struct Memory chunk = {0};
    
    const char* json =
        "{"
        "\"model\":\"deepseek-r1\","
        "\"prompt\":\"Yooo wsp\","
        "\"stream\":true"
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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
    }
    else {
        wsJson* root = wsStringToJson((const char**)&chunk.data);
        char out[10000];
        wsJsonToStringPretty(root, out, 10000);
        fprintf(stderr, "%s\n", out);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    free(chunk.data);
    curl_global_cleanup();

    return 0;
}

