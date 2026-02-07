
#include <mupdf/fitz/color.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <containers/darray.h>
#include <sys_utils.h>
#include <pdf.h>
#include <ocr.h>

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

    Context ctx = {0};
    init_ctx(&ctx);

    fz_pixmap** pdf_data = darrayCreate(fz_pixmap*);
    for (int32_t i = 0; i < darrayLength(pdfs); i++) {
        convert_pdf_to_img(&ctx, pdfs[i], &pdf_data);
    }

    for (int32_t i = 0; i < darrayLength(pdf_data); i++) {
        Image img = {
            .type = IMAGE_TYPE_PPM,
            .data.pix = pdf_data[i],
        };
        char* string = string_from_img(&ctx, &img);
        printf("%s\n", string);
    }

    darrayDestroy(pdf_data);
    darrayDestroy(prompt);
    darrayDestroy(imgs);
    darrayDestroy(pdfs);

    deinit_ctx(&ctx);

    return 0;
}

