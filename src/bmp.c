#include "bmp.h"
#include "color.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



RGBImage* read_bmp_file(FILE* fp, int* width, int* height) {
    uint8_t header[54];
    if (fread(header, 1, 54, fp) != 54) {
        fclose(fp);
        return NULL;
    }

    *width = *(int *) &header[18]; // 4bytes
    *height = *(int *) &header[22]; // 4bytes

    // move pointer to bmp data section
    uint32_t pixel_data_offset = *(uint32_t*)&header[10];
    fseek(fp, pixel_data_offset, SEEK_SET);

    RGBImage* img = create_rgb_image(*width, *height);

    int bits_per_pixel = 24; // 24bit bmp file
    int pixel_byte = bits_per_pixel / 8;
    int row_size = ((bits_per_pixel * *width + 31) / 32) * 4; // bmp file has padding

    uint8_t* row_buffer = (uint8_t*) malloc(row_size);
    int buf_idx, y_idx;

    uint8_t** r_p = img->r->p;
    uint8_t** g_p = img->g->p;
    uint8_t** b_p = img->b->p;

    for (int y = 0; y < *height; y++) {
        if (fread(row_buffer, 1, row_size, fp) != row_size) {
            free(row_buffer);
            free(img);
            return NULL;
        }

        for (int x = 0; x < *width; x++) {
            buf_idx = x * pixel_byte;
            y_idx = *height - 1 - y; // BMP file is formated in reverse (wtf)

            r_p[y_idx][x] = row_buffer[buf_idx + 2]; // R
            g_p[y_idx][x] = row_buffer[buf_idx + 1]; // G
            b_p[y_idx][x] = row_buffer[buf_idx + 0]; // B
        }
    }

    free(row_buffer);
    return img;
}



int write_bmp_file(FILE* fp, RGBImage* img, int width, int height) {
    int row_stride = (3 * width + 3) & ~3; // row has padding, aligned to 4 bytes
    int img_size = row_stride * height;
    int file_size = 14 + 40 + img_size;


    BMPHeader file_header = {
        .bfType = 0x4D42,
        .bfSize = file_size,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = 14 + 40
    };

    BMPInfoHeader info_header = {
        .biSize = 40,
        .biWidth = width,
        .biHeight = height,
        .biPlanes = 1,
        .biBitCount = 24,
        .biCompression = 0,
        .biSizeImage = img_size,
        .biXPelsPerMeter = 2835, // 72 DPI
        .biYPelsPerMeter = 2835,
        .biClrUsed = 0,
        .biClrImportant = 0
    };

    int h1 = fwrite(&file_header, sizeof(file_header), 1, fp);
    int h2 = fwrite(&info_header, sizeof(info_header), 1, fp);

    if (h1 != 1 || h2 != 1) return -1;

    int wri;

    uint8_t** r_p = img->r->p;
    uint8_t** g_p = img->g->p;
    uint8_t** b_p = img->b->p;

    uint8_t* row = (uint8_t*) malloc(row_stride);
    for (int y = height - 1; y >= 0; y--) { // BMP file is formated in reverse (wtf)
        memset(row, 0, row_stride); // initialize including paddings

        for (int x = 0; x < width; x++) {
            row[x * 3 + 0] = b_p[y][x]; // B
            row[x * 3 + 1] = g_p[y][x]; // G
            row[x * 3 + 2] = r_p[y][x]; // R
        }
        wri = fwrite(row, 1, row_stride, fp);

        if (wri != row_stride){
            free(row);
            return -1;
        };
    }

    free(row);
    return 0;
}
