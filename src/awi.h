#ifndef __AWI_H__
#define __AWI_H__

#include "bitrw.h"
#include "rle.h"
#include <stdint.h>
#include <stdbool.h>

/*
HEADER:
8 bytes : AWESOMEI (in ascii)
4 bytes : width
4 bytes : height

CONTENT:
// *DC -> diff of previous dc (12 bits)
// *AC-> RLE encoding
//  -(RUN LENGTH, SIZE) + VALUE
// *EOB(0x00)
// *(padding, byte align)
*/

#include "block.h"
#include "pixelrw.h"
#include <stdint.h>

typedef struct {
    char title[8];
    uint32_t width;
    uint32_t height;
} AWIHeader;

typedef struct {
    FILE* fp;
} AWIOutputContext;

typedef struct {
    FILE* fp;
} AWIInputContext;

void init_awi_bits_output(BitsOutput* bo, AWIOutputContext *ctx);
void init_awi_bits_input(BitsInput* bi, AWIInputContext* ctx);


bool compress_to_awi(PixelInput* pi, BitsOutput* bo);
bool decompress_awi(BitsInput* bi, PixelOutput* po);

#endif
