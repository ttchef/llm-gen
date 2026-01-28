
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <containers/darray.h>

enum {
    ARG_MODE_PROMPT,
    ARG_MODE_IMG,
    ARG_MODE_PDF,
};

int32_t main(int32_t argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: <prompt>\n");
        fprintf(stderr, "Help:\n"
            "\t--img:\t\tevery following argument will be interprated as a image path\n"
            "\t\t\tif a following argument is a flag it will switch\n"
            "\t--pdf:\t\tsame as --imgs but with pdf paths\n");
        return 0;
    }
    
    char** prompt = darrayCreate(char*);
    char** imgs = darrayCreate(char*);
    char** pdfs = darrayCreate(char*);

    int32_t current_mode = ARG_MODE_PROMPT;
    for (int32_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--img") == 0) {
            current_mode = ARG_MODE_IMG;   
            continue;
        }
        else if (strcmp(argv[i], "--pdf") == 0) {
            current_mode = ARG_MODE_PDF;   
            continue;
        }

        switch (current_mode) {
            case ARG_MODE_PROMPT:
                darrayPush(prompt, argv[i]);
                break;
            case ARG_MODE_IMG:
                darrayPush(imgs, argv[i]);
                break;
            case ARG_MODE_PDF:
                darrayPush(pdfs, argv[i]);
                break;
        };
    }

    for (int32_t i = 0; i < darrayLength(prompt); i++) {
        fprintf(stderr, "PROMPT: %s\n", prompt[i]);
    }
    for (int32_t i = 0; i < darrayLength(imgs); i++) {
        fprintf(stderr, "IMG: %s\n", imgs[i]);
    }
    for (int32_t i = 0; i < darrayLength(pdfs); i++) {
        fprintf(stderr, "PDF: %s\n", pdfs[i]);
    }

    darrayDestroy(prompt);
    darrayDestroy(imgs);
    darrayDestroy(pdfs);

    return 0;
}

