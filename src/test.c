#include "test.h"

#include "bitrw.h"
#include "dct.h"
#include "bmp.h"
#include <stdint.h>
#include <stdio.h>
#include <threads.h>


static inline void print_block(int* a) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            printf("%d ", a[y*8 +x]);
        }
        printf("\n");
    }
}

void dct_test() {
    Block_u8 test_block = {
        .b = {
            {  35,  42,  55,  70,  68,  54,  40,  33 },
            {  46,  65,  88, 120, 125,  98,  70,  48 },
            {  60,  92, 135, 180, 185, 150, 105,  70 },
            {  75, 120, 180, 235, 240, 200, 140,  90 },
            {  70, 118, 178, 238, 245, 195, 135,  85 },
            {  55,  90, 135, 180, 185, 145, 100,  65 },
            {  40,  70,  98, 125, 130, 100,  72,  50 },
            {  30,  45,  60,  80,  82,  65,  48,  35 }
        }
    };

    Block_int dct_res = dct_block(test_block);
    printf("\n1. DCT\n");
    print_block((int*) dct_res.b);

    printf("\n2. Quantize\n");
    Block_int quan_res = quantize_block(dct_res, QM_LUMA);
    print_block((int*) quan_res.b);

    printf("\n3. ZigZag\n");
    Chunk zig_res = zigzag_block(quan_res);
    for (int i = 0; i < 64; i++) {
        printf("%d ", zig_res.c[i]);
    }

    printf("\n\n4. Inverse zigzag\n");
    Block_int iz_res = in_zigzag_block(zig_res);
    print_block((int*) iz_res.b);

    printf("\n5. Inverse Quantize\n");
    Block_int iq_res = in_quantize_block(iz_res, QM_LUMA);
    print_block((int*) iq_res.b);

    printf("\n6. Inverse DCT\n");
    Block_u8 id_res = in_dct_block(iq_res);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            printf("%d ", id_res.b[y][x]);
        }
        printf("\n");
    }
}

void bit_write_test() {
    FILE* fp = fopen("./build/test", "w");

    BitWriter* bw = create_bit_writer(fp);
    bit_write(bw, 0b110, 3);
    bit_write(bw, 0b100101, 6);
    bit_write(bw, 0b101, 3);
    bit_write(bw, 0b10, 2);
    bit_write(bw, 0b10110110, 8);
    bit_write(bw, 0b01, 2);
    bit_write(bw, 0b0010011, 7);
    bit_writer_flush(bw);

    // 11010010110110101101100100100110
    // 11010010110110101101100100100110
}



void bmp_read_write_test() {
    FILE* fp = fopen("sample/sample1.bmp", "rb");
    int width, height;

    RGBImage* rgb_img = read_bmp_file(fp, &width, &height);

    FILE *write_fp = fopen("build/fuck.bmp", "wb");

    write_bmp_file(write_fp, rgb_img, width, height);
}