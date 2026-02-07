
#ifndef CONTEXT_H
#define CONTEXT_H

#include <mupdf/fitz.h>
#include <tesseract/capi.h>

typedef struct Context {
    fz_context* pdf_ctx;
    TessBaseAPI* ocr_ctx;
} Context;

int32_t init_ctx(Context* ctx);
void deinit_ctx(Context* ctx);

#endif //CONTEXT_H
