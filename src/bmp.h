#ifndef __BMP_H__
#define __BMP_H__

#include "color.h"
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



RGBImage* read_bmp_file(FILE* fp, int* width, int* height);
int write_bmp_file(FILE* fp, RGBImage* img, int width, int height);



#endif
