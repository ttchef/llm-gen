
#include "utils.h"
#include <mupdf/fitz/color.h>
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

enum {
    ARG_MODE_PROMPT,
    ARG_MODE_IMG,
    ARG_MODE_PDF,
    ARG_MODE_FONT,
};

static const bool solve_ai = false;

void get_texts(Context* ctx, char*** text_data, fz_pixmap** pdf_data, char** img_data, char** prompt_data) {
    for (int32_t i = 0; i < darrayLength(pdf_data); i++) {
        Image img = {
            .type = IMAGE_TYPE_PPM,
            .data.pix = pdf_data[i],
        };
        char* string = string_from_img(ctx, &img);
        if (!string) continue;
        darrayPush(*text_data, string);
    }

    for (int32_t i = 0; i < darrayLength(img_data); i++) {
        Image img = {
            .type = IMAGE_TYPE_STBI,
        };

        int32_t channels;
        img.data.stbi.data = stbi_load(img_data[i], &img.data.stbi.width, &img.data.stbi.height, &channels, BPP);
        if (!img.data.stbi.data) {
            fprintf(stderr, "Failed to load image: %s\n", img_data[i]);
            continue;
        }

        char* string = string_from_img(ctx, &img);
        if (!string) continue;
        darrayPush(*text_data, string);
        free(img.data.stbi.data);
    }

    for (int32_t i = 0; i < darrayLength(prompt_data); i++) {
        char* string = prompt_data[i];
        darrayPush(*text_data, string);
    }
} 

int32_t main(int32_t argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: <prompt>\n");
        fprintf(stderr, "Help:\n"
            "\t--img:\t\tevery following argument will be interprated as a image path\n"
            "\t\t\tif a following argument is a flag it will switch\n"
            "\t--pdf:\t\tsame as --imgs but with pdf paths\n"
            "\t--font:\t\timages with your font in the specified format\n");
        return 0;
    }
    
    char** prompt = darrayCreate(char*);
    char** imgs = darrayCreate(char*);
    char** pdfs = darrayCreate(char*);
    char** fonts = darrayCreate(char*);

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
            current_mode = ARG_MODE_FONT;   
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
            case ARG_MODE_FONT:
                darrayPush(fonts, argv[i]);
                break;
        };
    }

    create_dir_if_not_exists(BUILD_DIR"/generate");

    Context ctx = {0};
    init_ctx(&ctx);

    /* Image Generation */
    fz_pixmap** pdf_data = darrayCreate(fz_pixmap*);
    for (int32_t i = 0; i < darrayLength(pdfs); i++) {
        convert_pdf_to_img(&ctx, pdfs[i], &pdf_data);
    }

    char** text_data = darrayCreate(char*);
    get_texts(&ctx, &text_data, pdf_data, imgs, prompt);

    darrayDestroy(pdf_data);
    darrayDestroy(imgs);
    darrayDestroy(pdfs);

    if (darrayLength(fonts) <= 0) {
        fprintf(stderr, "You must specify at least 1 font template image via the --font flag\n");
        darrayDestroy(text_data);
        darrayDestroy(prompt);
        darrayDestroy(fonts);
        deinit_ctx(&ctx);
        return 1;
    }

    int32_t character_sets_count = darrayLength(fonts);
    CharacterSet character_sets[character_sets_count];
    generate_glyphs(character_sets, fonts);
    darrayDestroy(fonts);

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
            .x = 10,
            .y = 5,
        },
    };

    Images images = generate_font_image(page, response, character_sets, character_sets_count);

    for (int32_t i = 0; i < images.images_count; i++) {
        char buffer[30];
        snprintf(buffer, 30, "Page_%d.png", i);
        stbi_write_png(buffer, images.width, images.height, images.channels, images.images_data[i], images.width * images.channels);
    }

    destroy_images(&images);

    remove_dir(BUILD_DIR"/generate");

    darrayDestroy(text_data);
    darrayDestroy(prompt);
    destroy_character_sets(character_sets, character_sets_count);
    wsJsonFree(json_ai);
    deinit_ctx(&ctx);

    return 0;
}

