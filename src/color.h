#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>
#include "block.h"


void rgb_block16_to_ycbcr(RGBBlock16* rgb_b, YCbCrBlock_full* ycbcr_b);
void ycbcr_block_to_rgb(YCbCrBlock_full* ycbcr_b, RGBBlock16* rgb_b);
void ycbcr_block_subsample(YCbCrBlock_full* full, YCbCrBlock_sampled* sampled);
void ycbcr_block_upsample(YCbCrBlock_sampled* sampled, YCbCrBlock_full* full);


#endif
