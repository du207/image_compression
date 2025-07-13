#ifndef __BMP_H__
#define __BMP_H__

#include "color.h"
#include "pixelrw.h"
#include <stdint.h>
#include <stdio.h>

;
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BMPHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)



typedef struct {
    FILE* fp;
    uint32_t pixel_data_offset;
    int row_size;

    // cache for 16 rows
    uint8_t* rows_buffer;
    int rows_top_y; // y of the first row in the cached rows
} BMPInputContext;


typedef struct {
    FILE* fp;
    uint32_t pixel_data_offset;
    int row_size;

    // buffer for write
    uint8_t* rows_buffer;
    int rows_top_y; // y of the first row in the buffer rows
} BMPOutputContext;

void init_bmp_pixel_input(PixelInput* pi, BMPInputContext* context);
void init_bmp_pixel_output(PixelOutput* po, BMPOutputContext* context);



typedef struct {
    uint8_t** pixels;
} TextureOutputContext;

void init_rgb_pixel_output(PixelOutput* po, TextureOutputContext* context, uint8_t** pixels);


bool read_bmp_header(FILE* fp, BMPHeader* header, BMPInfoHeader* info_header);



#endif
