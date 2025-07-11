#ifndef __DCT_H__
#define __DCT_H__

#include "color.h"
#include <stdint.h>

#define PI 3.14159265358979323846


typedef struct {
    uint8_t b[8][8];
} Block_u8;

typedef struct {
    int b[8][8];
} Block_int;

typedef struct {
    int c[64];
} Chunk;

typedef struct {
    // not the width height of pixels
    // but the width height of chunks (blocks)
    int c_width;
    int c_height;
    Chunk* chunks; // Chunk array
} PreEncoding;

PreEncoding* create_pre_encoding(int c_width, int c_height);
void destroy_pre_encoding(PreEncoding* pe);

// 'in_' means 'inverse'
void dct_block(Block_u8* in, Block_int* out);
void in_dct_block(Block_int* in, Block_u8* out);

// QM_LUMA for Y, QM_CHROM for Cb, Cr
typedef enum { QM_LUMA, QM_CHROM } QuantMode;

void quantize_block(Block_int* in, Block_int* out, QuantMode qm);
void in_quantize_block(Block_int* in, Block_int* out, QuantMode qm);

void zigzag_block(Block_int* in, Chunk* out);
void in_zigzag_block(Chunk* in, Block_int* out);



#endif
