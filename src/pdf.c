
#include <mupdf/fitz/context.h>
#include <mupdf/fitz/document.h>
#include <mupdf/fitz/types.h>
#include <mupdf/fitz/util.h>
#include <pdf.h>

#include <containers/darray.h>

#include <stdio.h>
#include <stdlib.h>

int32_t convert_pdf_to_img(Context* ctx, char *path, fz_pixmap*** pix_array) {
    if (!pix_array) {
        fprintf(stderr, "pix_array == NULL\n");
        return 1;
    }

    fz_try(ctx->pdf_ctx)
        fz_register_document_handlers(ctx->pdf_ctx);
    fz_catch(ctx->pdf_ctx) {
        fz_report_error(ctx->pdf_ctx);
        fprintf(stderr, "failed to register document handlers\n");
        return 1;
    }

    fz_document* doc = NULL;

    fz_try(ctx->pdf_ctx)
        doc = fz_open_document(ctx->pdf_ctx, path);
    fz_catch(ctx->pdf_ctx) {
        fz_report_error(ctx->pdf_ctx);
        fprintf(stderr, "failed to open pdf document: %s\n", path);
        return 1;
    }

    int32_t page_count = 0;
    fz_try(ctx->pdf_ctx)
        page_count = fz_count_pages(ctx->pdf_ctx, doc);
    fz_catch(ctx->pdf_ctx) {
        fz_report_error(ctx->pdf_ctx);
        fprintf(stderr, "failed to get page number\n");
        fz_drop_document(ctx->pdf_ctx, doc);
        return 1;
    }

    fz_matrix matrix = fz_scale(1.0f, 1.0f);

    for (int32_t i = 0; i < page_count; i++) {
        fz_pixmap* pix = NULL;
        fz_try(ctx->pdf_ctx)
                pix = fz_new_pixmap_from_page_number(ctx->pdf_ctx, doc, i, matrix, fz_device_rgb(ctx->pdf_ctx), 0);
        fz_catch(ctx->pdf_ctx) {
            fz_report_error(ctx->pdf_ctx);
            fprintf(stderr, "failed to render page: %d\n", i);
            fz_drop_document(ctx->pdf_ctx, doc);
            return 1;
        }
        
        darrayPush(*pix_array, pix);
    }

    fz_drop_document(ctx->pdf_ctx, doc);

    return 0;
}


