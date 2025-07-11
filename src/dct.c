#include "dct.h"
#include "color.h"
#include "utils.h"
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>



// for optimization
static float cos_table[8][8];
static bool cos_table_initialized = false;

static void init_cos_table() {
    if (cos_table_initialized) return;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            cos_table[i][j] = cos((2 * i + 1) * j * PI / 16.0f);
        }
    }
    cos_table_initialized = true;
}


void dct_block(Block_u8* in, Block_int* out) {
    if (!cos_table_initialized) init_cos_table();

    uint8_t** in_b = in->b;
    int** out_b = out->b;

    int u, v, y, x;
    int8_t b_128;
    double cu, cv, sum;

    for (u = 0; u < 8; u++) {
        for (v = 0; v < 8; v++) {
            // normalize
            cu = (u == 0) ? 1.0 / sqrt(2.0) : 1.0;
            cv = (v == 0) ? 1.0 / sqrt(2.0) : 1.0;

            sum = 0.0;
            for (y = 0; y < 8; y++) {
                for (x = 0; x < 8; x++) {
                    b_128 = in_b[y][x] - 128;
                    sum += b_128 * cos_table[y][u] * cos_table[x][v];
                }
            }

            out_b[u][v] = (int) (0.25 * cu * cv * sum);
        }
    }
}

void in_dct_block(Block_int* in, Block_u8* out) {
    if (!cos_table_initialized) init_cos_table();

    int** in_b = in->b;
    uint8_t** out_b = out->b;

    int u, v, y, x;
    double cu, cv, sum;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8; x++) {
            sum = 0.0;

            for (u = 0; u < 8; u++) {
                for (v = 0; v < 8; v++) {
                    // normalize
                    cu = (u == 0) ? 1.0 / sqrt(2.0) : 1.0;
                    cv = (v == 0) ? 1.0 / sqrt(2.0) : 1.0;

                    sum += cu * cv * in_b[u][v] * cos_table[y][u] * cos_table[x][v];
                }
            }

            out_b[y][x] = clamp_uint8(0.25 * sum + 128);
        }
    }
}



// for Y
static const int quantization_table_luma[8][8] = {
    { 16, 11, 10, 16, 24, 40, 51, 61 },
    { 12, 12, 14, 19, 26, 58, 60, 55 },
    { 14, 13, 16, 24, 40, 57, 69, 56 },
    { 14, 17, 22, 29, 51, 87, 80, 62 },
    { 18, 22, 37, 56, 68, 109, 103, 77 },
    { 24, 35, 55, 64, 81, 104, 113, 92 },
    { 49, 64, 78, 87, 103, 121, 120, 101 },
    { 72, 92, 95, 98, 112, 100, 103, 99 }
};

// for Cb, Cr
static const int quantization_table_chrom[8][8] = {
    { 17, 18, 24, 47, 99,  99,  99,  99 },
    { 18, 21, 26, 66, 99,  99,  99,  99 },
    { 24, 26, 56, 99, 99,  99,  99,  99 },
    { 47, 66, 99, 99, 99,  99,  99,  99 },
    { 99, 99, 99, 99, 99,  99,  99,  99 },
    { 99, 99, 99, 99, 99,  99,  99,  99 },
    { 99, 99, 99, 99, 99,  99,  99,  99 },
    { 99, 99, 99, 99, 99,  99,  99,  99 }
};

void quantize_block(Block_int* in, Block_int* out, QuantMode qm) {
    const int (*quan_table)[8];
    
    if (qm == QM_LUMA) {
        quan_table = quantization_table_luma;
    } else {
        quan_table = quantization_table_chrom;
    }

    int** in_b = in->b;
    int** out_b = out->b;

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
             out_b[y][x] = round((float) in_b[y][x] / quan_table[y][x]);
        }
    }
}

void in_quantize_block(Block_int* in, Block_int* out, QuantMode qm) {
    const int (*quan_table)[8];
    
    if (qm == QM_LUMA) {
        quan_table = quantization_table_luma;
    } else {
        quan_table = quantization_table_chrom;
    }

    int** in_b = in->b;
    int** out_b = out->b;
    
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            out_b[y][x] = in_b[y][x] * quan_table[y][x];
        }
    }

    return out;
}

static const int zigzag_order[64] = {
    0,  1,  5,  6, 14, 15, 27, 28,
    2,  4,  7, 13, 16, 26, 29, 42,
    3,  8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};


void zigzag_block(Block_int* in, Chunk* out) {
    int** in_b = in->b;
    int* out_c = out->c;
    
    for (int i = 0; i < 64; i++) {
        int pos = zigzag_order[i];
        int y = pos / 8;
        int x = pos % 8;
        out_c[i] = in_b[y][x];
    }
}

void in_zigzag_block(Chunk* in, Block_int* out) {
    int* in_c = in->c;
    int** out_b = out->b;

    for (int i = 0; i < 64; i++) {
        int pos = zigzag_order[i];
        int y = pos / 8;
        int x = pos % 8;
        out_b[y][x] = in_c[i];
    }
}



/*
static Block_u8 get_block_from_channel(Channel* c, int block_x, int block_y) {
    Block_u8 block;

    int p_x = block_x * 8;
    int p_y = block_y * 8;

    int width = c->width;
    int height = c->height;
    int img_x, img_y;

    for (int y = 0; y < 8; y++) {
        img_y = p_y + y;
        for (int x = 0; x < 8; x++) {
            img_x = p_x + x;
            if (img_x < width && img_y < height) {
                block.b[y][x] = c->p[img_y][img_x];
            } else {
                // padding added
                block.b[y][x] = 0;
            }
        }
    }

    return block;
}


// dct transform + quantize + zigzag
PreEncoding* dct_channel(Channel* c, QuantMode qm) {
    int width = c->width;
    int height = c->height;

    int blocks_w = (width + 7) / 8; // ceil
    int blocks_h = (height + 7) / 8;

    PreEncoding* pe = create_pre_encoding(blocks_w, blocks_h);
    pe->c_width = blocks_w;
    pe->c_height = blocks_h;

    Block_u8 block;
    Block_int dct_b, quan_b;
    Chunk zz_c;

    for (int by = 0; by < blocks_h; by++) {
        for (int bx = 0; bx < blocks_w; bx++) {
            block = get_block_from_channel(c, bx, by);

            dct_b = dct_block(block);
            quan_b = quantize_block(dct_b, qm);
            zz_c = zigzag_block(quan_b);

            pe->chunks[by * blocks_w + bx] = zz_c;
        }
    }

    return pe;
}

static void write_channel_from_block(Channel* c, Block_u8 b, int bx, int by, int width, int height) {
    int p_y = by * 8;
    int p_x = bx * 8;
    int c_y, c_x;

    for (int y = 0; y < 8; y++) {
        c_y = p_y + y;
        for (int x = 0; x < 8; x++) {
            c_x = p_x + x;

            if (c_y < height && c_x < width) { // padding ignored
                c->p[c_y][c_x] = b.b[y][x];
            }
        }
    }
}


// inverse-zigzag + inverse-quntize + inverse-dct
// width, height for pixels
Channel* in_dct_channel(PreEncoding* pe, int width, int height, QuantMode qm) {
    int blocks_w = pe->c_width;
    int blocks_h = pe->c_height;

    Channel* c = create_channel(width, height);

    Chunk* chunks = pe->chunks;
    Chunk chunk;
    Block_int in_zig_res, in_quan_res;
    Block_u8 in_dct_res;

    for (int by = 0; by < blocks_h; by++) {
        for (int bx = 0; bx < blocks_w; bx++) {
            chunk = chunks[by * blocks_w + bx];
            in_zig_res = in_zigzag_block(chunk);
            in_quan_res = in_quantize_block(in_zig_res, qm);
            in_dct_res = in_dct_block(in_quan_res);

            write_channel_from_block(c, in_dct_res, bx, by, width, height);
        }
    }

    return c;
}
*/

