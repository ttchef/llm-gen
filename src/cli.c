
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <containers/darray.h>
#include <sys_utils.h>

#ifndef BUILD_DIR
#error "Need to definie BUILD_DIR as your build directory"
#endif

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

    create_dir_if_not_exists(BUILD_DIR"/generate");
    FILE* file = fopen(BUILD_DIR"/generate/file.txt", "w");
    fclose(file);
    remove_dir(BUILD_DIR"/generate");

    darrayDestroy(prompt);
    darrayDestroy(imgs);
    darrayDestroy(pdfs);

    return 0;
}

