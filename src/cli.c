
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
#include <editor.h>

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

static const bool solve_ai = false;

void parse_args(int32_t argc, char** argv, char*** prompt, char*** imgs, char*** pdfs, char** font_dir) {
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

    if (argc < 2) {
        fprintf(stderr, "USAGE: <prompt>\n");
        fprintf(stderr, "If no arguments are passed execpt a font directory it will read input throught stdin\n");
        fprintf(stderr, "Help:\n"
                "\t--img:\t\tevery following argument will be interprated as a image path\n"
                "\t\t\tif a following argument is a flag it will switch\n"
                "\t--pdf:\t\tsame as --imgs but with pdf paths\n"
                "\t--font:\t\tdirectory with images of the font template\n");
        return 0;
    }
    
    char** prompt = darrayCreate(char*);
    char** imgs = darrayCreate(char*);
    char** pdfs = darrayCreate(char*);
    char* font_dir = NULL;

    parse_args(argc, argv, &prompt, &imgs, &pdfs, &font_dir);

    Context ctx = {0};
    init_ctx(&ctx);

    /* Image Generation */
    fz_pixmap** pdf_data = darrayCreate(fz_pixmap*);
    char** text_data = NULL;

    if (darrayLength(pdfs) == 0 && 
        darrayLength(imgs) == 0 && 
        darrayLength(prompt) == 0) {

        text_data = darrayCreate(char*);

        char* buffer = malloc(MAX_STDIN_READ_SIZE);
        fgets(buffer, MAX_STDIN_READ_SIZE, stdin);

    }
    else {
        for (int32_t i = 0; i < darrayLength(pdfs); i++) {
            convert_pdf_to_img(&ctx, pdfs[i], &pdf_data);
        }

        text_data = darrayCreate(char*);
        get_texts(&ctx, &text_data, pdf_data, imgs, prompt);
    }
    
    /* Cleanup */
    darrayDestroy(pdf_data);
    darrayDestroy(imgs);
    darrayDestroy(pdfs);

    if (!font_dir) {
        fprintf(stderr, "Must specify 1 font template directory\n");
        deinit_ctx(&ctx);
        exit(1);
    }

    get_character_sets(&ctx, font_dir);
    
    run_editor();

    wsJson* json_ai;
    if (solve_ai) {
        generate_ai_answer(text_data, &json_ai);
    }
    else {
        json_ai = wsJsonInitObject(NULL);
        wsJsonAddString(json_ai, "response", "To find the derivative of \\(2x^2 + 4x\\), apply the power rule to each term.\n\nFor the term \\(2x^2\\):  \n- Coefficient \\(a = 2\\), exponent \\(n = 2\\)  \n- Derivative: \\(2 \\cdot 2 \\cdot x^{2-1} = 4x\\)\n\nFor the term \\(4x\\):  \n- Coefficient \\(a = 4\\), exponent \\(n = 1\\)  \n- Derivative: \\(4 \\cdot 1 \\cdot x^{1-1} = 4 \\cdot 1 \\cdot 1 = 4\\)\n\nCombine the results:  \n\\(\\frac{d}{dx}(2x^2 + 4x) = 4x + 4\\)\n\n\\boxed{4x+4}");
    }
    char* response = wsJsonGetString(json_ai, "response");
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

    Images images = generate_font_image(page, response, ctx.sets, ctx.sets_count);

    for (int32_t i = 0; i < images.images_count; i++) {
        char buffer[30];
        snprintf(buffer, 30, "Page_%d.png", i);
        stbi_write_png(buffer, images.width, images.height, images.channels, images.images_data[i], images.width * images.channels);
    }

    destroy_images(&images);

    darrayDestroy(text_data);
    darrayDestroy(prompt);
    wsJsonFree(json_ai);
    deinit_ctx(&ctx);

    return 0;
}

