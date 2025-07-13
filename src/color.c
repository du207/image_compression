#include "color.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "utils.h"


/*
Y=0.299⋅R+0.587⋅G+0.114⋅B
Cb=−0.1687⋅R−0.3313⋅G+0.5⋅B+128
Cr=0.5⋅R−0.4187⋅G−0.0813⋅B+128
*/

void rgb_block16_to_ycbcr(RGBBlock16* rgb_b, YCbCrBlock_full* ycbcr_b) {
    uint8_t (*r_b)[16] = rgb_b->r.b;
    uint8_t (*g_b)[16] = rgb_b->g.b;
    uint8_t (*b_b)[16] = rgb_b->b.b;

    Block_u8 (*y_bs)[2] = ycbcr_b->y; // [2][2]
    uint8_t (*cb_b)[16] = ycbcr_b->cb.b;
    uint8_t (*cr_b)[16] = ycbcr_b->cr.b;

    uint8_t r,g,b;

    for (int py = 0; py < 16; py++) {
        for (int px = 0; px < 16; px++) {
            r = r_b[py][px];
            g = g_b[py][px];
            b = b_b[py][px];

            y_bs[py/8][px/8].b[py%8][px%8] = clamp_uint8(0.299*r+0.587*g+0.114*b);
            cb_b[py][px] = clamp_uint8(-0.1687*r-0.3313*g+0.5*b+128);
            cr_b[py][px] = clamp_uint8(0.5*r-0.4187*g-0.0813*b+128);
        }
    }
}


/*
R = Y + 1.402 × (Cr - 128)
G = Y - 0.344136 × (Cb - 128) - 0.714136 × (Cr - 128)
B = Y + 1.772 × (Cb - 128)
*/

void ycbcr_block_to_rgb(YCbCrBlock_full* ycbcr_b, RGBBlock16* rgb_b) {
    Block_u8 (*y_bs)[2] = ycbcr_b->y;
    uint8_t (*cb_b)[16] = ycbcr_b->cb.b;
    uint8_t (*cr_b)[16] = ycbcr_b->cr.b;

    uint8_t (*r_b)[16] = rgb_b->r.b;
    uint8_t (*g_b)[16] = rgb_b->g.b;
    uint8_t (*b_b)[16] = rgb_b->b.b;

    uint8_t y,cb,cr;

    for (int py = 0; py < 16; py++) {
        for (int px = 0; px < 16; px++) {
            y = y_bs[py/8][px/8].b[py%8][px%8];
            cb = cb_b[py][px];
            cr = cr_b[py][px];

            r_b[py][px] = clamp_uint8(y + 1.402*(cr - 128));
            g_b[py][px] = clamp_uint8(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128));
            b_b[py][px] = clamp_uint8(y + 1.772 * (cb - 128));
        }
    }
}

void ycbcr_block_subsample(YCbCrBlock_full* full, YCbCrBlock_sampled* sampled) {
    memcpy(sampled->y, full->y, 2*2*sizeof(Block_u8));

    uint8_t (*full_cb_b)[16] = full->cb.b;
    uint8_t (*full_cr_b)[16] = full->cr.b;

    uint8_t (*sampled_cb_b)[8] = sampled->cb.b;
    uint8_t (*sampled_cr_b)[8] = sampled->cr.b;

    int y, x;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            sampled_cb_b[y][x] = full_cb_b[y*2][x*2];
            sampled_cr_b[y][x] = full_cr_b[y*2][x*2];
        }
    }
}

void ycbcr_block_upsample(YCbCrBlock_sampled* sampled, YCbCrBlock_full* full) {
    memcpy(full->y, sampled->y, 2*2*sizeof(Block_u8));

    uint8_t (*sampled_cb_b)[8] = sampled->cb.b;
    uint8_t (*sampled_cr_b)[8] = sampled->cr.b;

    uint8_t (*full_cb_b)[16] = full->cb.b;
    uint8_t (*full_cr_b)[16] = full->cr.b;

    int y, x;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            full_cb_b[y][x] = sampled_cb_b[y/2][x/2];
            full_cr_b[y][x] = sampled_cr_b[y/2][x/2];
        }
    }
}
