#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "dct.h"
#include "rle.h"
#include "test.h"

int convert_bmp_to_awi(char* filename);


int main (int argc, char** argv) {
    // expected command:
    // image_compression convert (.bmp image file)
    // image_compression revert (.awi image file)
    // image_compression show (.awi image file)
    // ONLY SUPPORT 24BITS BMP FILE

    int err_code = 0;
    char* task = argv[1];
    char* filename = argv[2];
    if (task == NULL) task = "";

    if (strcmp(task, "convert") == 0) {
        if (filename == NULL) {
            printf(".bmp image file name required!\n");
            return 1;
        }

        err_code = convert_bmp_to_awi(filename);
        if (err_code < 0) goto error_handle;
    } else if (strcmp(task, "revert") == 0) {
        if (filename == NULL) {
            printf(".awi image file name required!\n");
            return 1;
        }

    } else if (strcmp(task, "show") == 0) {
        if (filename == NULL) {
            printf(".awi image file name required!\n");
            return 1;
        }

    } else {
        // show help command
        printf("*********************************\n");
        printf("*** .awi project command list ***\n");
        printf("*********************************\n\n");
        printf("image_compression convert (.bmp image file)\n");
        printf("image_compression revert (.awi image file)\n");
        printf("image_compression show (.awi image file)\n");
        printf("\n* Only supports for 24bit bmp file\n");
    }

    return 0;


error_handle:
    switch (err_code) {
    case -1:
        printf("File reading/writing error!\n");
        return 1;
    case -2:
        printf("Not valid BMP file format!\n");
        return 2;
    default:
        printf("Unknown error!\n");
        return 1;
    }
}


int convert_bmp_to_awi(char* filename) {
    // 1. Read bmp file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    uint8_t header[54];
    if (fread(header, 1, 54, fp) != 54) {
        fclose(fp);
        return -2;
    }

    int width = *(int *) &header[18]; // 4bytes
    int height = *(int *) &header[22]; // 4bytes

    int bits_per_pixel = 24; // 24bit bmp file
    int pixel_byte = bits_per_pixel / 8;
    int row_size = ((bits_per_pixel * width + 31) / 32) * 4; // bmp file has padding

    // 2. Read rgb pixels
    RGBImage* rgb_image = create_rgb_image(width, height);

    uint8_t* row_buffer = (uint8_t*) malloc(row_size);
    int buf_idx, y_idx;

    for (int y = 0; y < height; y++) {
        if (fread(row_buffer, 1, row_size, fp) != row_size) {
            free(row_buffer);
            fclose(fp);
            return -2;
        }

        for (int x = 0; x < width; x++) {
            buf_idx = x * pixel_byte;
            y_idx = height - 1 - y; // BMP file is formated in reverse (wtf)

            rgb_image->r->p[y_idx][x] = row_buffer[buf_idx + 2]; // R
            rgb_image->g->p[y_idx][x] = row_buffer[buf_idx + 1]; // G
            rgb_image->b->p[y_idx][x] = row_buffer[buf_idx + 0]; // B
        }
    }

    free(row_buffer);
    fclose(fp);

    // 3. Convert to YCbCr and subsampling
    YCbCrImage* ycbcr_image = rgb_image_to_ycbcr(rgb_image);
    destroy_rgb_image(rgb_image); // rgb now useless

    YCbCrImage* sampled_image = ycbcr_420_sampling(ycbcr_image);

    // dct_test();

    // 4. DCT transform
    // int* y_dct = dct_channel(sampled_image->Y_plane, sampled_image->width, sampled_image->height);
    PreEncoding* pe = dct_channel(sampled_image->y, QM_LUMA);

    rle_encode(pe);




    // // __TEST__
    // printf("original channel\n\n");
    // for (int x = 0; x < sampled_image->y->width; x++) {
    //     printf("%d ", sampled_image->y->p[100][x]);
    // }

    // printf("\n\nPreEncoding chunks\n\n");
    // for (int i = 0; i < 20; i++) {
    //     for (int j = 0; j < 64; j++) {
    //         printf("%d ", pe->chunks[i].c[j]);
    //     }
    //     printf("\n");
    // }

    // Channel* in_c = in_dct_channel(pe, width, height, QM_LUMA);
    // printf("\n\nin-dct channel\n\n");
    // for (int x = 0; x < in_c->width; x++) {
    //     printf("%d ", in_c->p[100][x]);
    // }

    // destroy_channel(in_c);
    // // __END_TEST__

    destroy_pre_encoding(pe);
    destroy_ycbcr_image(sampled_image);
    destroy_ycbcr_image(ycbcr_image);
    return 0;
}
