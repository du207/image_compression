#include "awi.h"
#include "bitrw.h"
#include "color.h"
#include "block.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


static void transform_block(Block_u8* in, Chunk* out, QuantMode qm) {
    Block_int dct_out, quantize_out;
    dct_block(&in, &dct_out);
    quantize_block(&dct_out, &quantize_out, qm);
    zigzag_block(&quantize_out, &out);
}


int compress_to_awi(PixelInput* pi, BitsOutput* bo) {
    if (!bo->begin(bo)) {
        fprintf(stderr, "Compressing: Bits output Preprogressing error!\n");
        return 0;
    }

    int width = pi->width;
    int height = pi->height;

    // 16x16 blocks
    int blocks_16_w = (width + 15) / 16; // ceil
    int blocks_16_h = (height + 15) / 16;

    int b16_y, b16_x;

    for (b16_y = 0; b16_y < blocks_16_h; b16_y++) {
        for (b16_x = 0; b16_x < blocks_16_w; b16_x++) {
            RGBBlock16 rgb_b;
            YCbCrBlock_full ycbcr_full;
            YCbCrBlock_sampled ycbcr_sampled;

            if (!pi->read_block(pi, b16_x, b16_y, &rgb_b)) {
                fprintf(stderr, "Reading RGB blocks error!\n");
                return 0;
            }

            rgb_block16_to_ycbcr(&rgb_b, &ycbcr_full);
            ycbcr_block_subsample(&ycbcr_full, &ycbcr_sampled);

            Chunk y_cnks[4];
            Chunk cb_cnk, cr_cnk;

            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    transform_block(&ycbcr_sampled.y[i][j], &y_cnks[i*2 + j], QM_LUMA);
                }
            }

            transform_block()

            
        }
    }    

    return 1;
}





/*
RGBImage* awi_content_to_rgb_img(AWIContent* content, int width, int height) {
    int sub_width = (width + 1) / 2; // ceil
    int sub_height = (height + 1) / 2;

    RLEEncoder* re_y = content->re_y;
    RLEEncoder* re_cb = content->re_cb;
    RLEEncoder* re_cr = content->re_cr;

    // RLE Decoding
    PreEncoding* pe_y = rle_decode(re_y, width, height);
    PreEncoding* pe_cb = rle_decode(re_cb, sub_width, sub_height);
    PreEncoding* pe_cr = rle_decode(re_cr, sub_width, sub_height);

    // in-DCT transform
    Channel* c_y = in_dct_channel(pe_y, width, height, QM_LUMA);
    Channel* c_cb = in_dct_channel(pe_cb, sub_width, sub_height, QM_CHROM);
    Channel* c_cr = in_dct_channel(pe_cr, sub_width, sub_height, QM_CHROM);
    destroy_pre_encoding(pe_y);
    destroy_pre_encoding(pe_cb);
    destroy_pre_encoding(pe_cr);


    // in-subsampling
    YCbCrImage ycbcr_sampled_img = {
        .is_subsampled = true,
        .y = c_y,
        .cb = c_cb,
        .cr = c_cr
    };

    YCbCrImage* ycbcr_img = ycbcr_420_inverse_sampling(&ycbcr_sampled_img);

    RGBImage* rgb_img = ycbcr_image_to_rgb(ycbcr_img);

    destroy_channel(c_y);
    destroy_channel(c_cb);
    destroy_channel(c_cr);
    destroy_ycbcr_image(ycbcr_img);

    return rgb_img;
}
*/