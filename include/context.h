
#ifndef CONTEXT_H
#define CONTEXT_H

#include <mupdf/fitz.h>
#include <tesseract/capi.h>
#include <glyph.h>

typedef struct Context {
    fz_context* pdf_ctx;
    TessBaseAPI* ocr_ctx;
    CharacterSet* sets;
    int32_t sets_count;
} Context;

int32_t init_ctx(Context* ctx);
void get_texts(Context* ctx, char*** text_data, fz_pixmap** pdf_data, char** img_data, char** prompt_data);
void deinit_ctx(Context* ctx);

#endif //CONTEXT_H
