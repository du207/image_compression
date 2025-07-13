#ifndef __DCT_H__
#define __DCT_H__

#include "color.h"
#include "block.h"
#include <stdint.h>

#define PI 3.14159265358979323846


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
