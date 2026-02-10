
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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef BUILD_DIR
#error "Need to definie BUILD_DIR as your build directory"
#endif

enum {
    ARG_MODE_PROMPT,
    ARG_MODE_IMG,
    ARG_MODE_PDF,
    ARG_MODE_FONT,
};

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

    CharacterSet character_sets[darrayLength(fonts)];
    generate_glyphs(character_sets, fonts);

    wsJson* json_ai;
    generate_ai_answer(text_data, &json_ai);
    char* response = wsJsonGetString(json_ai, "response");
    fprintf(stderr, "%s\n", response);

    remove_dir(BUILD_DIR"/generate");

    darrayDestroy(text_data);

    darrayDestroy(prompt);
    deinit_ctx(&ctx);

    return 0;
}

