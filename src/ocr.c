
#include <ocr.h>

char* string_from_img(Context* ctx, Image* img) {
    switch (img->type) {
        case IMAGE_TYPE_PPM:
            TessBaseAPISetImage(ctx->ocr_ctx,
                                img->data.pix->samples,
                                img->data.pix->w,
                                img->data.pix->h,
                                BPP, img->data.pix->stride);
            break;
        case IMAGE_TYPE_STBI:
            TessBaseAPISetImage(ctx->ocr_ctx, 
                                img->data.stbi.data,
                                img->data.stbi.width,
                                img->data.stbi.height,
                                BPP, img->data.stbi.width * BPP);
            break;
        default:
            return NULL;
    }

    if (TessBaseAPIRecognize(ctx->ocr_ctx, NULL) != 0) {
        fprintf(stderr, "failed to recognize image for ocr\n");
        return NULL;
    }

    char* text = TessBaseAPIGetUTF8Text(ctx->ocr_ctx);
    if (!text) {
        fprintf(stderr, "failed to get text from img ocr\n");
        return NULL;
    }

    return text;
}

