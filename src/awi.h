#ifndef __AWI_H__
#define __AWI_H__

#include "bitrw.h"
#include "rle.h"
#include <stdint.h>

/*
HEADER:
8 bytes : AWESOMEI (in ascii)
4 bytes : width
4 bytes : height
4 bytes : y units length
4 bytes : cb units length
4 bytes : cr units length

CONTENT:
// *DC -> diff of previous dc (12 bits)
// *AC-> RLE encoding
//  -(RUN LENGTH, SIZE) + VALUE
// *EOB(0x00)
// *(padding, byte align)
*/

typedef struct {
    char title[8];
    uint32_t width;
    uint32_t height;
    uint32_t y_units_length;
    uint32_t cb_units_length;
    uint32_t cr_units_length;
} AWIHeader;

void write_awi_file(BitWriter* bw, RLEEncoder* re_y, RLEEncoder* re_cb, RLEEncoder* re_cr, int width, int height);
void read_awi_file(BitReader *br, RLEEncoder* re_y, RLEEncoder* re_cb, RLEEncoder* re_cr, int* width, int* height);

#endif
