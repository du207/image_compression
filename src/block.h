#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <stdint.h>

typedef struct {
    uint8_t b[8][8];
} Block_u8;

typedef struct {
    int b[8][8];
} Block_int;


typedef struct {
    uint8_t b[16][16];
} Block16_u8;

typedef struct {
    Block16_u8 r;
    Block16_u8 g;
    Block16_u8 b;
} RGBBlock16;


// before 4:2:0 subsampling
typedef struct {
    Block_u8 y[2][2]; // Y: 4 * (8x8 block)
    Block16_u8 cb;
    Block16_u8 cr;
} YCbCrBlock_full;

// after 4:2:0 subsampling
typedef struct {
    Block_u8 y[2][2]; // Y: 4 * (8x8 block)
    Block_u8 cb;
    Block_u8 cr;
} YCbCrBlock_sampled;


typedef struct {
    int c[64];
} Chunk;

#endif
