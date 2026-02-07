
#include <context.h>
#include <mupdf/fitz/context.h>

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
    fz_drop_context(ctx->pdf_ctx);

    TessBaseAPIEnd(ctx->ocr_ctx);
    TessBaseAPIDelete(ctx->ocr_ctx);
}
