
#ifndef PDF_H
#define PDF_H 

#include <stdint.h>
#include <mupdf/fitz.h>

#include <context.h> 

int32_t convert_pdf_to_img(Context* ctx, char* path, fz_pixmap* piximg);   

#endif // PDF_H
