#include "color.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "utils.h"



void rgb_block16_to_ycbcr(RGBBlock16* rgb_b, YCbCrBlock_full* ycbcr_b) {
    uint8_t** r_b = rgb_b->r.b;
    uint8_t** g_b = rgb_b->g.b;
    uint8_t** b_b = rgb_b->b.b;
    
    Block_u8** y_bs = ycbcr_b->y; // [2][2]
    uint8_t** cb_b = ycbcr_b->cb.b;
    uint8_t** cr_b = ycbcr_b->cr.b;

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


void ycbcr_block16_to_rgb(YCbCrBlock_full* ycbcr_b, RGBBlock16* rgb_b) {
    Block_u8** y_bs = ycbcr_b->y;
    uint8_t** cb_b = ycbcr_b->cb.b;
    uint8_t** cr_b = ycbcr_b->cr.b;

    uint8_t** r_b = rgb_b->r.b;
    uint8_t** g_b = rgb_b->g.b;
    uint8_t** b_b = rgb_b->b.b;
    
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

void ycbcr_block16_subsample(YCbCrBlock_full* full, YCbCrBlock_sampled* sampled) {
    memcpy(sampled->y, full->y, 2*2*sizeof(Block_u8));
   
    uint8_t** full_cb_b = full->cb.b;
    uint8_t** full_cr_b = full->cr.b;

    uint8_t** sampled_cb_b = sampled->cb.b;
    uint8_t** sampled_cr_b = sampled->cr.b;

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

    uint8_t** sampled_cb_b = sampled->cb.b;
    uint8_t** sampled_cr_b = sampled->cr.b;

    uint8_t** full_cb_b = full->cb.b;
    uint8_t** full_cr_b = full->cr.b;

    int y, x;
    
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            full_cb_b[y][x] = sampled_cb_b[y/2][x/2];
            full_cr_b[y][x] = sampled_cr_b[y/2][x/2];
        }
    } 
}


/*

YCbCrImage* ycbcr_420_sampling(YCbCrImage* img) {
    // img expected to have y, cb, cr all same width and height
    assert(!img->is_subsampled);

    int width = img->y->width;
    int height = img->y->height;
    int sub_width = (width + 1) / 2; // if odd number, ceil
    int sub_height = (height + 1) / 2;

    YCbCrImage* sub_img = create_ycbcr_image_subsize(width, height, sub_width, sub_height);

    // 4:2:0
    // just take the topleft value
    // (no calculating average cuz im lazy)
    int y, x;

    uint8_t **y_p  = img->y->p;
    uint8_t **cr_p = img->cr->p;
    uint8_t **cb_p = img->cb->p;

    uint8_t **sub_y_p  = sub_img->y->p;
    uint8_t **sub_cr_p = sub_img->cr->p;
    uint8_t **sub_cb_p = sub_img->cb->p;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sub_y_p[y][x] = y_p[y][x];

            if ((y%2 == 0) && (x%2 == 0)) {
                sub_cb_p[y/2][x/2] = cb_p[y][x];
                sub_cr_p[y/2][x/2] = cr_p[y][x];
            }
        }
    }

    return sub_img;
}


YCbCrImage* ycbcr_420_inverse_sampling(YCbCrImage* sub_img) {
    // img expected to have cb, cr sub_width*sub_height
    assert(sub_img->is_subsampled);

    int width = sub_img->y->width;
    int height = sub_img->y->height;

    YCbCrImage* img = create_ycbcr_image(width, height);

    uint8_t **y_p  = img->y->p;
    uint8_t **cr_p = img->cr->p;
    uint8_t **cb_p = img->cb->p;

    uint8_t **sub_y_p  = sub_img->y->p;
    uint8_t **sub_cr_p = sub_img->cr->p;
    uint8_t **sub_cb_p = sub_img->cb->p;

    int y, x, sub_y, sub_x;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sub_x = x/2; sub_y = y/2;

            y_p[y][x] = sub_y_p[y][x];
            cb_p[y][x] = sub_cb_p[sub_y][sub_x];
            cr_p[y][x] = sub_cr_p[sub_y][sub_x];
        }
    }

    return img;
}
*/


/*Channel* create_channel(int width, int height) {
    Channel* c = (Channel*) malloc(sizeof(Channel));
    c->width = width;
    c->height = height;

    // 2dim array
    c->p = (uint8_t**) malloc(sizeof(uint8_t*) * height);
    for (int y = 0; y < height; y++) {
        c->p[y] = (uint8_t*) malloc(sizeof(uint8_t) * width);
    }

    return c;
}

void destroy_channel(Channel* c) {
    if (c == NULL) return;
    if (c->p != NULL) {
        for (int y = 0; y < c->height; y++) {
            free_safe(c->p[y]);
        }
        free(c->p);
    }

    free(c);
}


RGBImage* create_rgb_image(int width, int height) {
    RGBImage* img = (RGBImage*) malloc(sizeof(RGBImage));
    img->r = create_channel(width, height);
    img->g = create_channel(width, height);
    img->b = create_channel(width, height);

    return img;
}

void destroy_rgb_image(RGBImage* img) {
    if (img == NULL) return;

    destroy_channel(img->r);
    destroy_channel(img->g);
    destroy_channel(img->b);
    free(img);
}


YCbCrImage* create_ycbcr_image(int width, int height) {
    YCbCrImage* img = (YCbCrImage*) malloc(sizeof(YCbCrImage));
    img->is_subsampled = false;
    img->y = create_channel(width, height);
    img->cb = create_channel(width, height);
    img->cr = create_channel(width, height);

    return img;
}

YCbCrImage* create_ycbcr_image_subsize(int width, int height, int sub_width, int sub_height) {
    YCbCrImage* img = (YCbCrImage*) malloc(sizeof(YCbCrImage));
    img->is_subsampled = true;
    img->y = create_channel(width, height);
    img->cb = create_channel(sub_width, sub_height);
    img->cr = create_channel(sub_width, sub_height);

    return img;
};


void destroy_ycbcr_image(YCbCrImage* img) {
    if (img == NULL) return;

    destroy_channel(img->y);
    destroy_channel(img->cb);
    destroy_channel(img->cr);
    free(img);
}


/*
Y=0.299⋅R+0.587⋅G+0.114⋅B
Cb=−0.1687⋅R−0.3313⋅G+0.5⋅B+128
Cr=0.5⋅R−0.4187⋅G−0.0813⋅B+128

YCbCrImage* rgb_image_to_ycbcr(RGBImage* rgb_img) {
    // rgb_img expected to have r, g, b all same width and height
    int width = rgb_img->r->width;
    int height = rgb_img->r->height;

    YCbCrImage* ycbcr_img = create_ycbcr_image(width, height);

    uint8_t r, g, b;

    // for optimization
    uint8_t **r_p = rgb_img->r->p;
    uint8_t **g_p = rgb_img->g->p;
    uint8_t **b_p = rgb_img->b->p;

    uint8_t **y_p  = ycbcr_img->y->p;
    uint8_t **cr_p = ycbcr_img->cr->p;
    uint8_t **cb_p = ycbcr_img->cb->p;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            r = r_p[y][x];
            g = g_p[y][x];
            b = b_p[y][x];

            y_p[y][x] = clamp_uint8(0.299*r+0.587*g+0.114*b);
            cb_p[y][x] = clamp_uint8(-0.1687*r-0.3313*g+0.5*b+128);
            cr_p[y][x] = clamp_uint8(0.5*r-0.4187*g-0.0813*b+128);
        }
    }

    return ycbcr_img;
}

/*
R = Y + 1.402 × (Cr - 128)
G = Y - 0.344136 × (Cb - 128) - 0.714136 × (Cr - 128)
B = Y + 1.772 × (Cb - 128)

RGBImage* ycbcr_image_to_rgb(YCbCrImage* ycbcr_img) {
    // ycbcr_image expected to have y, cb, cr all same width and height
    assert(!ycbcr_img->is_subsampled);

    int width = ycbcr_img->y->width;
    int height = ycbcr_img->y->height;

    RGBImage* rgb_img = create_rgb_image(width, height);
    uint8_t y, cb, cr;

    uint8_t **r_p = rgb_img->r->p;
    uint8_t **g_p = rgb_img->g->p;
    uint8_t **b_p = rgb_img->b->p;

    uint8_t **y_p  = ycbcr_img->y->p;
    uint8_t **cr_p = ycbcr_img->cr->p;
    uint8_t **cb_p = ycbcr_img->cb->p;

    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            y = y_p[py][px];
            cb = cb_p[py][px];
            cr = cr_p[py][px];

            r_p[py][px] = clamp_uint8(y + 1.402*(cr - 128));
            g_p[py][px] = clamp_uint8(y - 0.344136 * (cb - 128) - 0.714136 * (cr - 128));
            b_p[py][px] = clamp_uint8(y + 1.772 * (cb - 128));

        }
    }

    return rgb_img;
}


*/