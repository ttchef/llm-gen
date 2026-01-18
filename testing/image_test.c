
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void get_image_crop_rgb(const uint8_t* input_pixels, int32_t input_w, int32_t crop_x, int32_t crop_y, uint8_t* output_pixels, int32_t output_w, int32_t output_h) {
    int32_t i, j;
    int32_t ch_num = 3;
    int32_t r_channel = 0;
    int32_t g_channel = 1;
    int32_t b_channel = 2;
    for (i = 0; i < output_h; i++) {
        for (j = 0; j < output_w; j++) {
            output_pixels[(i * output_w + j) * ch_num + r_channel] = input_pixels[((crop_y + i) * input_w + crop_x + j) * ch_num + r_channel];
            output_pixels[(i * output_w + j) * ch_num + g_channel] = input_pixels[((crop_y + i) * input_w + crop_x + j) * ch_num + g_channel];
            output_pixels[(i * output_w + j) * ch_num + b_channel] = input_pixels[((crop_y + i) * input_w + crop_x + j) * ch_num + b_channel];
        }
    }
} 

int main(void) {
    int32_t width, height, channels;
    uint8_t* data = stbi_load("test.jpeg", &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "failed to load image\n");
        return 1;
    }
    printf("Channels: %d\n", channels);

    int32_t crop_x = 57, crop_y = 57, crop_w = 60, crop_h = 60;
    uint8_t* crop_img = malloc(crop_w * crop_h * channels * 2);
    uint8_t* tmp = crop_img;

    for (int32_t i = 0; i < 2; i++) {
        get_image_crop_rgb(data, width, crop_x, crop_y, tmp, crop_w, crop_h);
        tmp += crop_w * channels;
        crop_x += crop_w;
    }

    stbi_write_jpg("crop.jpg", crop_w * 2, crop_h, channels, crop_img, 90);

    free(crop_img);
    stbi_image_free(data);

    return 0;
}

