
#ifndef PDF_H
#define PDF_H 

#include <stdint.h>

#include <mupdf/fitz.h>

int32_t convert_pdf_to_img(char* path, fz_pixmap*** pix_array);   

#endif // PDF_H
