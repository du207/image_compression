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

typedef struct {
    RLEEncoder* re_y;
    RLEEncoder* re_cb;
    RLEEncoder* re_cr;
} AWIContent;

void write_awi_file(BitWriter* bw, AWIContent* content, int width, int height);
void read_awi_file(BitReader *br, AWIContent* content, int* width, int* height);

AWIContent* create_awi_content(RLEEncoder* re_y, RLEEncoder* re_cb, RLEEncoder* re_cr);
void destroy_awi_content(AWIContent* content);

AWIContent* rgb_img_to_awi_content(RGBImage* rgb_img);
RGBImage* awi_content_to_rgb_img(AWIContent* content, int width, int height);

#endif
