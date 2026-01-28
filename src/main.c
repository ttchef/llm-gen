
#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

    int32_t current_mode = ARG_MODE_PROMPT;
    for (int32_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--imgs") == 0) {
            current_mode = ARG_MODE_IMG;   
        }
        else if (strcmp(argv[i], "--pdf") == 0) {
            current_mode = ARG_MODE_PDF;   
        }
    }

    return 0;
}


