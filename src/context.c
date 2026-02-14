
#include <context.h>
#include <mupdf/fitz/context.h>
#include <image.h>
#include <ocr.h>
#include <containers/darray.h>

#include <stb_image.h>

int32_t init_ctx(Context *ctx) {
    ctx->pdf_ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
    if (!ctx->pdf_ctx) {
        fprintf(stderr, "failed to create mupdf context\n");
        return 1;
    }

    ctx->ocr_ctx = TessBaseAPICreate();
    if(TessBaseAPIInit3(ctx->ocr_ctx, NULL, "deu") != 0) {
        fprintf(stderr, "failed to init ocr ctx\n");
        return 1;
    }

    TessBaseAPISetPageSegMode(ctx->ocr_ctx, PSM_AUTO);

    return 0;
}

void deinit_ctx(Context *ctx) {
    destroy_character_sets(ctx->sets, ctx->sets_count);
    fz_drop_context(ctx->pdf_ctx);

    TessBaseAPIEnd(ctx->ocr_ctx);
    TessBaseAPIDelete(ctx->ocr_ctx);
}

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

