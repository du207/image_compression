#include "awi.h"

#include "bitrw.h"
#include "color.h"
#include "block.h"
#include "dct.h"
#include "rle.h"
#include "pixelrw.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


// AWI Output
static bool awi_output_write(BitsOutput* self, const void* data, size_t size);

void init_awi_bits_output(BitsOutput* bo, AWIOutputContext *ctx) {
    bo->context = ctx;
    bo->write = &awi_output_write;
    bo->bit_count = 0;
    bo->buffer = 0;
}

static bool awi_output_write(BitsOutput* self, const void* data, size_t size) {
    AWIOutputContext* ctx = self->context;

    if (fwrite(data, size, 1, ctx->fp) == 1) {
        return true;
    } else {
        return false;
    }
}

// AWI Input
static bool awi_input_read(BitsInput* self, void* buf, size_t size);

void init_awi_bits_input(BitsInput* bi, AWIInputContext* ctx) {
    bi->context = ctx;
    bi->read = &awi_input_read;
    bi->bit_count = 0;
    bi->buffer = 0;
}

static bool awi_input_read(BitsInput* self, void* buf, size_t size) {
    AWIInputContext* ctx = self->context;

    if (fread(buf, size, 1, ctx->fp) == 1) {
        return true;
    } else {
        return false;
    }
}

// ---------------------------------

static void transform_block(Block_u8* in, Chunk* out, QuantMode qm) {
    Block_int dct_out, quantize_out;
    dct_block(in, &dct_out);
    quantize_block(&dct_out, &quantize_out, qm);
    zigzag_block(&quantize_out, out);
}


bool compress_to_awi(PixelInput* pi, BitsOutput* bo) {
    if (pi->begin != NULL && !pi->begin(pi)) {
        fprintf(stderr, "Compressing: Preprocessing error!\n");
        return false;
    }

    int width = pi->width;
    int height = pi->height;

    // write header
    AWIHeader header = {
        .title = {'A', 'W', 'E', 'S', 'O', 'M', 'E', 'I'},
        .width = width,
        .height = height
    };
    if (!bo->write(bo, &header, sizeof(AWIHeader))) {
        fprintf(stderr, "Writing header error!\n");
        return false;
    }

    // 16x16 blocks
    int blocks_16_w = (width + 15) / 16; // ceil
    int blocks_16_h = (height + 15) / 16;

    int b16_y, b16_x;

    int prev_dc_y[2][2] = { 0 };
    int prev_dc_cb = 0, prev_dc_cr = 0;

    for (b16_y = 0; b16_y < blocks_16_h; b16_y++) {
        for (b16_x = 0; b16_x < blocks_16_w; b16_x++) {
            RGBBlock16 rgb_b;
            YCbCrBlock_full ycbcr_full;
            YCbCrBlock_sampled ycbcr_sampled;

            if (!pi->read_block(pi, b16_x, b16_y, &rgb_b)) {
                fprintf(stderr, "Reading RGB blocks error!\n");
                return false;
            }

            rgb_block16_to_ycbcr(&rgb_b, &ycbcr_full);
            ycbcr_block_subsample(&ycbcr_full, &ycbcr_sampled);

            Chunk y_cnk[2][2];
            Chunk cb_cnk, cr_cnk;


            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    transform_block(&ycbcr_sampled.y[i][j], &y_cnk[i][j], QM_LUMA);
                    if (!rle_encode_chunk_and_write(bo, &y_cnk[i][j], prev_dc_y[i][j])) goto error;
                    prev_dc_y[i][j] = y_cnk[i][j].c[0];
                }
            }

            transform_block(&ycbcr_sampled.cb, &cb_cnk, QM_CHROM);
            if (!rle_encode_chunk_and_write(bo, &cb_cnk, prev_dc_cb)) goto error;
            prev_dc_cb = cb_cnk.c[0];

            transform_block(&ycbcr_sampled.cr, &cr_cnk, QM_CHROM);
            if (!rle_encode_chunk_and_write(bo, &cr_cnk, prev_dc_cr)) goto error;
            prev_dc_cr = cr_cnk.c[0];
        }
    }

    if (pi->end != NULL && !pi->end(pi)) {
        fprintf(stderr, "Compressing: Postprocessing error!\n");
        return false;
    }

    return true;

error:
    fprintf(stderr, "Encoding and writing error!\n");
    fprintf(stderr, "b16_y=%d, b16_x=%d\n", b16_y, b16_x);
    return false;
}


// ---------------------------------

static void inverse_transform_block(Chunk* in, Block_u8* out, QuantMode qm) {
    Block_int in_zig, in_quan;
    in_zigzag_block(in, &in_zig);
    in_quantize_block(&in_zig, &in_quan, qm);
    in_dct_block(&in_quan, out);
}

bool decompress_awi(BitsInput* bi, PixelOutput* po) {
    // read header
    AWIHeader header;
    if (!bi->read(bi, &header, sizeof(AWIHeader))) {
        fprintf(stderr, "Reading header error!\n");
        return false;
    }

    char expected_title[8] = {'A', 'W', 'E', 'S', 'O', 'M', 'E', 'I'};
    if (memcmp(header.title, expected_title, 8) != 0) {
        fprintf(stderr, "Invalid file format!\n");
        return false;
    }

    int width = header.width;
    int height = header.height;

    if (po->begin != NULL && !po->begin(po, width, height)) {
        fprintf(stderr, "Decompressing: Preprocessing error!\n");
        return false;
    }

    // 16x16 blocks
    int blocks_16_w = (width + 15) / 16; // ceil
    int blocks_16_h = (height + 15) / 16;

    int b16_y, b16_x;

    int prev_dc_y[2][2] = { 0 };
    int prev_dc_cb = 0, prev_dc_cr = 0;

    for (b16_y = 0; b16_y < blocks_16_h; b16_y++) {
        for (b16_x = 0; b16_x < blocks_16_w; b16_x++) {
            Chunk y_cnk[2][2];
            Chunk cb_cnk, cr_cnk;

            YCbCrBlock_sampled ycbcr_sampled;
            YCbCrBlock_full ycbcr_full;
            RGBBlock16 rgb_b;

            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    if (!read_and_rle_decode_chunk(bi, &y_cnk[i][j], prev_dc_y[i][j])) goto error;
                    inverse_transform_block(&y_cnk[i][j], &ycbcr_sampled.y[i][j], QM_LUMA);
                    prev_dc_y[i][j] = y_cnk[i][j].c[0];
                }
            }

            if (!read_and_rle_decode_chunk(bi, &cb_cnk, prev_dc_cb)) goto error;
            inverse_transform_block(&cb_cnk, &ycbcr_sampled.cb, QM_CHROM);
            prev_dc_cb = cb_cnk.c[0];

            if (!read_and_rle_decode_chunk(bi, &cr_cnk, prev_dc_cr)) goto error;
            inverse_transform_block(&cr_cnk, &ycbcr_sampled.cr, QM_CHROM);
            prev_dc_cr = cr_cnk.c[0];

            ycbcr_block_upsample(&ycbcr_sampled, &ycbcr_full);
            ycbcr_block_to_rgb(&ycbcr_full, &rgb_b);

            if (!po->write_block(po, b16_x, b16_y, &rgb_b)) {
                fprintf(stderr, "Writing rgb blocks error!\n");
                return false;
            }
        }
    }

    if (po->end != NULL && !po->end(po)) {
        fprintf(stderr, "Decompressing: Postprocessing error!\n");
        return false;
    }

    return true;

error:
    fprintf(stderr, "Reading and decoding error!\n");
    fprintf(stderr, "b16_y=%d, b16_x=%d\n", b16_y, b16_x);
    return false;
}
