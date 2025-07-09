#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "awi.h"
#include "bitrw.h"
#include "bmp.h"
#include "color.h"
#include "dct.h"
#include "rle.h"
#include "test.h"
#include "utils.h"

int convert_bmp_to_awi(char* filename);
int revert_awi_to_bmp(char* filename);


int main (int argc, char** argv) {
    // expected command:
    // image_compression convert (.bmp image file)
    // image_compression revert (.awi image file)
    // image_compression show (.awi image file)
    // ONLY SUPPORT 24BITS BMP FILE

    int err_code = 0;
    char* task = argv[1];
    char* filename = argv[2];
    if (task == NULL) task = "";

    if (strcmp(task, "convert") == 0) {
        if (filename == NULL) {
            printf(".bmp image file name required!\n");
            return 1;
        }

        err_code = convert_bmp_to_awi(filename);
        if (err_code < 0) goto error_handle;
    } else if (strcmp(task, "revert") == 0) {
        if (filename == NULL) {
            printf(".awi image file name required!\n");
            return 1;
        }

        err_code = revert_awi_to_bmp(filename);
        if (err_code < 0) goto error_handle;
    } else if (strcmp(task, "show") == 0) {
        if (filename == NULL) {
            printf(".awi image file name required!\n");
            return 1;
        }

    } else {
        // show help command
        printf("*********************************\n");
        printf("*** .awi project command list ***\n");
        printf("*********************************\n\n");
        printf("image_compression convert (.bmp image file)\n");
        printf("image_compression revert (.awi image file)\n");
        printf("image_compression show (.awi image file)\n");
        printf("\n* Only supports for 24bit bmp file\n");
        bmp_read_write_test();
    }

    return 0;


error_handle:
    switch (err_code) {
    case -1:
        printf("File reading/writing error!\n");
        return 1;
    case -2:
        printf("Invalid file format!\n");
        return 2;
    default:
        printf("Unknown error!\n");
        return 1;
    }
}


int convert_bmp_to_awi(char* filename) {
    if (!is_end_with(filename, ".bmp")) return -2;

    // 1. Read bmp file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    int width, height;
    RGBImage* rgb_image = read_bmp_file(fp, &width, &height);
    if (rgb_image == NULL) {
        fclose(fp);
        return -2;
    }

    fclose(fp);

    // 2. Convert to YCbCr and subsampling
    YCbCrImage* ycbcr_image = rgb_image_to_ycbcr(rgb_image);
    destroy_rgb_image(rgb_image); // rgb now useless

    YCbCrImage* sampled_image = ycbcr_420_sampling(ycbcr_image);
    destroy_ycbcr_image(ycbcr_image); // ycbcr now useless

    // 3. DCT transform
    PreEncoding* pe_y = dct_channel(sampled_image->y, QM_LUMA);
    PreEncoding* pe_cb = dct_channel(sampled_image->cb, QM_CHROM);
    PreEncoding* pe_cr = dct_channel(sampled_image->cr, QM_CHROM);
    destroy_ycbcr_image(sampled_image); // sampled now useless


    // 4. RLE Encode
    RLEEncoder* re_y = rle_encode(pe_y);
    destroy_pre_encoding(pe_y);

    RLEEncoder* re_cb = rle_encode(pe_cb);
    destroy_pre_encoding(pe_cb);

    RLEEncoder* re_cr = rle_encode(pe_cr);
    destroy_pre_encoding(pe_cr);



   // 5. Write .awi file
    char* awi_filename = replace_str_end(filename, ".awi");
    if (awi_filename == NULL) {
        destroy_rle_encoder(re_y);
        destroy_rle_encoder(re_cb);
        destroy_rle_encoder(re_cr);
        return -2;
    }

    FILE* fp_write_awi = fopen(awi_filename, "wb");
    if (fp_write_awi == NULL) {
        destroy_rle_encoder(re_y);
        destroy_rle_encoder(re_cb);
        destroy_rle_encoder(re_cr);
        return -1;
    }

    BitWriter* bw = create_bit_writer(fp);

    write_awi_file(bw, re_y, re_cb, re_cr, width, height);

    destroy_rle_encoder(re_y);
    destroy_rle_encoder(re_cb);
    destroy_rle_encoder(re_cr);
    destroy_bit_writer(bw);
    fclose(fp_write_awi);

    printf("%s: convert completed!\n", awi_filename);
    free(awi_filename);

    return 0;
}

int revert_awi_to_bmp(char* filename) {
    if (!is_end_with(filename, ".awi")) return -2;

    // 1. Read awi file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    BitReader* br = create_bit_reader(fp);

    RLEEncoder* re_y = create_rle_encoder();
    RLEEncoder* re_cb = create_rle_encoder();
    RLEEncoder* re_cr = create_rle_encoder();

    int width, height;

    read_awi_file(br, re_y, re_cb, re_cr, &width, &height);
    destroy_bit_reader(br);

    int sub_width = (width + 1) / 2; // ceil
    int sub_height = (height + 1) / 2;

    // 2. RLE Decoding
    PreEncoding* pe_y = rle_decode(re_y, width, height);
    PreEncoding* pe_cb = rle_decode(re_cb, sub_width, sub_height);
    PreEncoding* pe_cr = rle_decode(re_cr, sub_width, sub_height);
    destroy_rle_encoder(re_y);
    destroy_rle_encoder(re_cb);
    destroy_rle_encoder(re_cr);


    // 3. in-DCT transform

    
    Channel* c_y = in_dct_channel(pe_y, width, height, QM_LUMA);
    Channel* c_cb = in_dct_channel(pe_cb, sub_width, sub_height, QM_CHROM);
    Channel* c_cr = in_dct_channel(pe_cr, sub_width, sub_height, QM_CHROM);
    destroy_pre_encoding(pe_y);
    destroy_pre_encoding(pe_cb);
    destroy_pre_encoding(pe_cr);

    
    // 4. in-subsampling
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


    // 5. write .bmp file
    char* bmp_filename = replace_str_end(filename, ".bmp");
    if (bmp_filename == NULL) {
        destroy_rgb_image(rgb_img);
        return -2;
    }

    FILE* bmp_fp = fopen(bmp_filename, "wb");
    if (write_bmp_file(bmp_fp, rgb_img, width, height) < 0) {
        destroy_rgb_image(rgb_img);
        fclose(bmp_fp);
        return -1;
    }

    destroy_rgb_image(rgb_img);
    fclose(bmp_fp);

    printf("%s: revert completed!\n", bmp_filename);
    free(bmp_filename);

    return 0;
}
