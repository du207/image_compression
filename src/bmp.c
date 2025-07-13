#include "bmp.h"
#include "block.h"
#include "pixelrw.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>



static bool pixel_input_begin(PixelInput* self);
static bool pixel_input_read_block(PixelInput* self, int bx, int by, RGBBlock16* rgb_out);
static bool pixel_input_end(PixelInput* self);


void init_bmp_pixel_input(PixelInput* pi, BMPInputContext* context) {
    pi->context = context;
    pi->begin = &pixel_input_begin;
    pi->read_block = &pixel_input_read_block;
    pi->end = &pixel_input_end;
}

static bool pixel_input_begin(PixelInput* self) {
    BMPHeader header;
    BMPInfoHeader info_header;
    BMPInputContext* ctx = self->context;

    if (!read_bmp_header(ctx->fp, &header, &info_header)) return false;

    self->width = info_header.biWidth;
    self->height = info_header.biHeight;

    ctx->pixel_data_offset = header.bfOffBits; // offset
    ctx->row_size = ((24 * self->width + 31) / 32) * 4; // bmp file has 4byte padding
    ctx->rows_top_y = -1;
    ctx->rows_buffer = (uint8_t*) malloc(16 * ctx->row_size);

    return true;
}


static inline int min(int a, int b) { return a < b ? a : b; }

// read 16x16 block from bmp
// bmp file store pixels in bottom-up order (wtf)
static bool pixel_input_read_block(PixelInput* self, int bx, int by, RGBBlock16* rgb_out) {
    BMPInputContext* ctx= (BMPInputContext*) self->context;
    FILE* fp = ctx->fp;
    int row_size = ctx->row_size;
    int pixel_data_offset = (int) ctx->pixel_data_offset;
    uint8_t* rows_buffer = ctx->rows_buffer;

    int width = self->width;
    int height = self->height;

    int start_y = 16 * by;
    int start_x = 16 * bx;

    // when caller's by is changed, the whole rows(all bx) are stored in buffer
    // becuase 'read_block' will be called in order (by: 0, 1, 2.. bx: 0, 1, 2)
    if (start_y != ctx->rows_top_y) { // cache update
        // because bmp is top-down, gonna read from dy=15 to dy=0
        // so seek to the end(dy=15) and fread continously
        // however if dy=15 > height, then fseek will fail
        // so should find the most bottom row's dy
        int effective_height = height - start_y;
        int top_valid_dy = min(15, effective_height - 1); // highest index we can read

        if (top_valid_dy < 0) {
            // all padding
            memset(rgb_out->r.b, 0, 16*16);
            memset(rgb_out->g.b, 0, 16*16);
            memset(rgb_out->b.b, 0, 16*16);
            return true; // nothing to do more!
        }

        int current_y = start_y + top_valid_dy;
        int bmp_y = height - 1 - current_y;
        int row_bottom_position = pixel_data_offset + bmp_y * row_size;

        if (fseek(fp, row_bottom_position, SEEK_SET) != 0) return false;

        for (int dy = top_valid_dy; dy >= 0; dy--) {
            if (fread(&rows_buffer[dy * row_size], row_size, 1, fp) != 1) return false;
        }

        if (top_valid_dy < 15) { // padding
            for (int dy = 15; dy > top_valid_dy; dy--) {
                memset(&rows_buffer[dy * row_size], 0, row_size);
            }
        }

        ctx->rows_top_y = start_y;
    }

    uint8_t (*r_b)[16] = rgb_out->r.b;
    uint8_t (*g_b)[16] = rgb_out->g.b;
    uint8_t (*b_b)[16] = rgb_out->b.b;

    for (int dy = 0; dy < 16; dy++) {
        for (int dx = 0; dx < 16; dx++) {
            int current_x = start_x + dx;

            if (current_x >= width) { // padding
                r_b[dy][dx] = 0;
                g_b[dy][dx] = 0;
                b_b[dy][dx] = 0;
            } else {
                r_b[dy][dx] = rows_buffer[dy * row_size + 3*current_x + 2];
                g_b[dy][dx] = rows_buffer[dy * row_size + 3*current_x + 1];
                b_b[dy][dx] = rows_buffer[dy * row_size + 3*current_x + 0];
            }
        }
    }

    return true;
}


static bool pixel_input_end(PixelInput* self) {
    BMPInputContext* ctx = self->context;

    free_safe(ctx->rows_buffer);

    return true;
}


// -------------------

static bool pixel_output_begin(PixelOutput* self, int width, int height);
static bool pixel_output_write_block(PixelOutput* self, int bx, int by, RGBBlock16* block);
static bool pixel_output_end(PixelOutput* self);

void init_bmp_pixel_output(PixelOutput* po, BMPOutputContext* context) {
    po->context = context;
    po->begin = &pixel_output_begin;
    po->write_block = &pixel_output_write_block;
    po->end = &pixel_output_end;
}


static bool pixel_output_begin(PixelOutput* self, int width, int height) {
    BMPOutputContext* ctx = self->context;
    FILE* fp = ctx->fp;

    self->width = width;
    self->height = height;

    ctx->row_size = ((24 * self->width + 31) / 32) * 4; // bmp file has 4byte padding
    ctx->rows_top_y = -1;
    ctx->rows_buffer = (uint8_t*) malloc(16 * ctx->row_size);
    ctx->pixel_data_offset = 14+40;

    int img_size = ctx->row_size * height;
    int file_size = 14 + 40 + img_size;

    // write bmp header
    BMPHeader file_header = {
        .bfType = 0x4D42,
        .bfSize = file_size,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = ctx->pixel_data_offset
    };

    BMPInfoHeader info_header = {
        .biSize = 40,
        .biWidth = width,
        .biHeight = height,
        .biPlanes = 1,
        .biBitCount = 24,
        .biCompression = 0,
        .biSizeImage = img_size,
        .biXPelsPerMeter = 2835, // 72 DPI
        .biYPelsPerMeter = 2835,
        .biClrUsed = 0,
        .biClrImportant = 0
    };

    int h1 = fwrite(&file_header, sizeof(file_header), 1, fp);
    int h2 = fwrite(&info_header, sizeof(info_header), 1, fp);

    if (!h1 || !h2) return false;

    return true;
}

static bool write_block_flush(PixelOutput* po, int start_y) {
    int height = po->height;
    BMPOutputContext* ctx = po->context;
    FILE* fp = ctx->fp;
    int row_size = ctx->row_size;
    uint8_t* rows_buffer = ctx->rows_buffer;

    // because bmp is top-down, gonna write from dy=15 to dy=0
    // so seek to the end(dy=15) and fwrite continously
    // however if dy=15 > height, then fseek will fail
    // so should find the most bottom row's dy
    int effective_height = height - start_y;
    int top_valid_dy = min(15, effective_height - 1); // highest index we can read

    if (top_valid_dy < 0) {
        return true; // nothing to do
    }

    int current_y = start_y + top_valid_dy;
    int bmp_y = height - 1 - current_y;
    int row_bottom_position = ctx->pixel_data_offset + bmp_y * row_size;

    if (fseek(fp, row_bottom_position, SEEK_SET) != 0) return false;

    for (int dy = top_valid_dy; dy >= 0; dy--) {
        if (fwrite(&rows_buffer[dy * row_size], row_size, 1, fp) != 1) return false;
    }

    return true;
}

static bool pixel_output_write_block(PixelOutput* self, int bx, int by, RGBBlock16* block) {
    BMPOutputContext* ctx = self->context;
    int row_size = ctx->row_size;
    uint8_t* rows_buffer = ctx->rows_buffer;

    int width = self->width;

    int start_y = 16 * by;
    int start_x = 16 * bx;

    // write buffer to BMP
    // when caller's by is changed, the whole rows(all bx) are stored in buffer
    // becuase 'write_block' will be called in order (by: 0, 1, 2.. bx: 0, 1, 2)
    if (ctx->rows_top_y != start_y) {
        if (ctx->rows_top_y != -1) {
            if (!write_block_flush(self, ctx->rows_top_y)) return false;
        }
        ctx->rows_top_y = start_y;
    }

    uint8_t (*r_b)[16] = block->r.b;
    uint8_t (*g_b)[16] = block->g.b;
    uint8_t (*b_b)[16] = block->b.b;

    for (int dy = 0; dy < 16; dy++) {
        for (int dx = 0; dx < 16; dx++) {
            int current_x = start_x + dx;

            if (current_x < width) {
                rows_buffer[row_size * dy + 3 * current_x + 2] = r_b[dy][dx];
                rows_buffer[row_size * dy + 3 * current_x + 1] = g_b[dy][dx];
                rows_buffer[row_size * dy + 3 * current_x + 0] = b_b[dy][dx];
            }
        }
    }


    return true;
}

static bool pixel_output_end(PixelOutput* self) {
    BMPOutputContext* ctx = self->context;
    write_block_flush(self, ctx->rows_top_y); // flush the remained buffer
    free_safe(ctx->rows_buffer);
    return true;
}

// -------------------
static bool rgb_pixel_output_begin(PixelOutput* self, int width, int height);
static bool rgb_pixel_output_write_block(PixelOutput* self, int bx, int by, RGBBlock16* block);

void init_rgb_pixel_output(PixelOutput* po, TextureOutputContext* context, uint8_t** pixels) {
    po->context = context;
    po->begin = &rgb_pixel_output_begin;
    po->write_block = &rgb_pixel_output_write_block;
    po->end = NULL;
    context->pixels = pixels;
}

static bool rgb_pixel_output_begin(PixelOutput* self, int width, int height) {
    TextureOutputContext* ctx = self->context;

    self->width = width;
    self->height = height;

    *ctx->pixels = (uint8_t*) malloc(width * height * 3);
    return true;
}

static bool rgb_pixel_output_write_block(PixelOutput* self, int bx, int by, RGBBlock16* block) {
    TextureOutputContext* ctx = self->context;
    int width = self->width;
    int height = self->height;
    uint8_t* pixels = *ctx->pixels;

    uint8_t (*r_b)[16] = block->r.b;
    uint8_t (*g_b)[16] = block->g.b;
    uint8_t (*b_b)[16] = block->b.b;

    for (int dy = 0; dy < 16; dy++) {
        int current_y = by*16 + dy;

        if (current_y < height) {
            for (int dx = 0; dx < 16; dx++) {
                int current_x = bx*16 + dx;

                if (current_x < width) {
                    int offset = (width * current_y + current_x) * 3;
                    pixels[offset + 0] = r_b[dy][dx];
                    pixels[offset + 1] = g_b[dy][dx];
                    pixels[offset + 2] = b_b[dy][dx];
                }
            }
        }

    }

    return true;
}





// -------------------
bool read_bmp_header(FILE* fp, BMPHeader* header, BMPInfoHeader* info_header) {
    size_t f1 = fread(header, sizeof(BMPHeader), 1, fp);

    size_t f2 = fread(info_header, sizeof(BMPInfoHeader), 1, fp);

    if (f1 != 1 || f2 != 1) {
        fprintf(stderr, "Reading bmp header error!\n");
        return 0;
    }

    if (header->bfType != 0x4D42) { // 'BM' (little endian)
        fprintf(stderr, "Invalid BMP format!\n");
        return 0;
    }

    return 1;
}
