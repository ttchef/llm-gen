
#include <stdio.h>
#include <stdint.h>

int32_t main(int32_t argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: <prompt>\n");
        fprintf(stderr, "Help:\n"
            "\t--imgs:\t\tevery following argument will be interprated as a image path\n"
            "\t\t\tif a following argument is a flag it will switch\n"
            "\t--pdf:\t\tsame as --imgs but with pdf paths\n");
    }

    for (int32_t i = 1; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    return 0;
}


