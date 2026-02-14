
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <containers/darray.h>
#include <sys_utils.h>
#include <pdf.h>
#include <ocr.h>
#include <generate.h>
#include <glyph.h>
#include <image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifndef BUILD_DIR
#error "Need to definie BUILD_DIR as your build directory"
#endif

#define MAX_STDIN_READ_SIZE 10000 /* Chars */

enum {
    ARG_MODE_PROMPT,
    ARG_MODE_IMG,
    ARG_MODE_PDF,
};

static const bool solve_ai = true;

void parse_args(int32_t argc, char** argv, char*** prompt, char*** imgs, char*** pdfs, char** font_dir) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: <prompt>\n");
        fprintf(stderr, "If no arguments are passed execpt a font directory it will read input throught stdin\n");
        fprintf(stderr, "Help:\n"
                "\t--img:\t\tevery following argument will be interprated as a image path\n"
                "\t\t\tif a following argument is a flag it will switch\n"
                "\t--pdf:\t\tsame as --imgs but with pdf paths\n"
                "\t--font:\t\tdirectory with images of the font template\n");
        exit(1);
    }

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
        else if (strcmp(argv[i], "--font") == 0) {
            if (i + 1 < argc) {
                *font_dir = argv[i + 1];
                i++;
                continue;
            }
            else {
                fprintf(stderr, "ERROR: --font flag specified but no template directory named\n");
                exit(1);
            }
            continue;
        }

        switch (current_mode) {
            case ARG_MODE_PROMPT:
                darrayPush(*prompt, argv[i]);
                break;
            case ARG_MODE_IMG:
                darrayPush(*imgs, argv[i]);
                break;
            case ARG_MODE_PDF:
                darrayPush(*pdfs, argv[i]);
                break;
        };
    }
}

int32_t main(int32_t argc, char** argv) {
    srand(time(NULL));

    char** prompt = darrayCreate(char*);
    char** imgs = darrayCreate(char*);
    char** pdfs = darrayCreate(char*);
    char* font_dir = NULL;

    parse_args(argc, argv, &prompt, &imgs, &pdfs, &font_dir);

    Context ctx = {0};
    init_ctx(&ctx);

    char** text_data = NULL;

    text_data = darrayCreate(char*);
    get_texts(&ctx, &text_data, pdfs, imgs, prompt);

    /* Cleanup */
    darrayDestroy(imgs);
    darrayDestroy(pdfs);

    if (get_character_sets(&ctx, font_dir)) {
        return 1;
    }

    char* response = NULL;
    if (solve_ai) {
        generate_ai_answer(&ctx, text_data, &response);
    }
    else {
        response =  "To find the derivative of \\(2x^2 + 4x\\), apply the power rule to each term.\n\nFor the term \\(2x^2\\):  \n- Coefficient \\(a = 2\\), exponent \\(n = 2\\)  \n- Derivative: \\(2 \\cdot 2 \\cdot x^{2-1} = 4x\\)\n\nFor the term \\(4x\\):  \n- Coefficient \\(a = 4\\), exponent \\(n = 1\\)  \n- Derivative: \\(4 \\cdot 1 \\cdot x^{1-1} = 4 \\cdot 1 \\cdot 1 = 4\\)\n\nCombine the results:  \n\\(\\frac{d}{dx}(2x^2 + 4x) = 4x + 4\\)\n\n\\boxed{4x+4}";
    }
    fprintf(stderr, "%s\n", response);

    Page page = {
        .bg_type = PAGE_BG_TYPE_LINES,
        .dim = {
            .width = 1500,
            .height = 2150,
        },
        .padding = {
            .x = 0,
            .y = 0,
        },
    };

    generate_font_image(&ctx, page, response, ctx.sets, ctx.sets_count);
    write_images(&ctx, ".");

    darrayDestroy(text_data);
    darrayDestroy(prompt);
    deinit_ctx(&ctx);

    return 0;
}


