#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

typedef struct {
    int width;
    int height;
    uint8_t** p; // [y][x] 2dim array
} Channel;

Channel* create_channel(int width, int height);
void destroy_channel(Channel* c);


typedef struct {
    Channel* r;
    Channel* g;
    Channel* b;
} RGBImage;

RGBImage* create_rgb_image(int width, int height);
void destroy_rgb_image(RGBImage* img);


typedef struct {
    // if 1: y is width*height, cb, cr is sub_width*sub_height
    // if 0: y, cb, cr all has same width*height
    int is_subsampled;
    Channel* y;
    Channel* cb;
    Channel* cr;
} YCbCrImage;

// Y, Cb, Cr all same width*height size
YCbCrImage* create_ycbcr_image(int width, int height);
// used for 4:2:0 subsampling
// width*height for Y channel, sub_width*sub_height for Cb, Cr Channel
YCbCrImage* create_ycbcr_image_subsize(int width, int height, int sub_width, int sub_height);
void destroy_ycbcr_image(YCbCrImage* img);

/*
Y=0.299⋅R+0.587⋅G+0.114⋅B
Cb=−0.1687⋅R−0.3313⋅G+0.5⋅B+128
Cr=0.5⋅R−0.4187⋅G−0.0813⋅B+128
*/

// rgb_img expected to have r, g, b all same width and height
YCbCrImage* rgb_image_to_ycbcr(RGBImage* rgb_img);
// ycbcr_image expected to have y, cb, cr all same width and height (is_sampled = false)
RGBImage* ycbcr_image_to_rgb(YCbCrImage* ycbcr_img);


// img expected to have y, cb, cr all same width and height (is_sampled = false)
YCbCrImage* ycbcr_420_sampling(YCbCrImage* img);
// img expected to have cb, cr sub_width*sub_height (is_sampled = true)
YCbCrImage* ycbcr_420_inverse_sampling(YCbCrImage* img);



#endif
