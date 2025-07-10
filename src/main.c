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

    // Read bmp file
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    int width, height;
    RGBImage* rgb_image = read_bmp_file(fp, &width, &height);
    if (rgb_image == NULL) {
        fclose(fp);
        return -2;
    }

    fclose(fp);

    AWIContent* content = rgb_img_to_awi_content(rgb_image);


   // Write .awi file
    char* awi_filename = replace_str_end(filename, ".awi");
    if (awi_filename == NULL) {
        destroy_awi_content(content);
        return -2;
    }

    FILE* fp_write_awi = fopen(awi_filename, "wb");
    if (fp_write_awi == NULL) {
        destroy_awi_content(content);
        return -1;
    }

    BitWriter* bw = create_bit_writer(fp);

    write_awi_file(bw, content, width, height);

    destroy_awi_content(content);
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

    AWIContent* content;

    int width, height;

    read_awi_file(br, content, &width, &height);
    destroy_bit_reader(br);

    RGBImage* rgb_img = awi_content_to_rgb_img(content, width, height);

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
