#include "bitrw.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

BitWriter* create_bit_writer(FILE* fp) {
    BitWriter* bw = (BitWriter*) malloc(sizeof(BitWriter));
    bw->fp = fp;
    bw->buffer = 0;
    bw->bit_count = 0;
    return bw;
}

void destroy_bit_writer(BitWriter* bw) {
    if (bw == NULL) return;
    free(bw);
}

void bit_write(BitWriter* bw, uint32_t value, int bits_size) {
    assert(bits_size <= 32 && bits_size >= 0);

    for (int i = bits_size - 1; i >= 0; i--) {
        bw->buffer <<= 1;
        bw->buffer |= (value >> i) & 1;
        bw->bit_count++;

        if (bw->bit_count == 8) {
            if (fputc(bw->buffer, bw->fp) == EOF) {
                fprintf(stderr, "File bit write error!\n");
                return;
            }
            bw->buffer = 0;
            bw->bit_count = 0;
        }
    }
}


void bit_writer_flush(BitWriter* bw) {
    if (bw->bit_count > 0) {
        bw->buffer <<= (8 - bw->bit_count);
        fputc(bw->buffer, bw->fp);
        bw->buffer = 0;
        bw->bit_count = 0;
    }
}



BitReader* create_bit_reader(FILE* fp) {
    BitReader* br = (BitReader*) malloc(sizeof(BitReader));
    br->fp = fp;
    br->buffer = 0;
    br->bit_count = 0;
    return br;
}

void destroy_bit_reader(BitReader* br) {
    if (br == NULL) return;
    free(br);
}

int bit_read(BitReader* br, int bits_size, uint32_t* out) {
    assert(bits_size >= 0 && bits_size <= 32);

   *out = 0;
    for (int i = 0; i < bits_size; i++) {
        if (br->bit_count == 0) {
            int c = fgetc(br->fp);
            if (c == EOF) {
                return 0;
            };
            br->buffer = (uint8_t) c;
            br->bit_count = 8;
        }
        *out <<= 1;
        *out |= (br->buffer >> 7) & 1;
        br->buffer <<= 1;
        br->bit_count--;
    }

    return 1;
}


void bit_reader_align_to_byte(BitReader* br) {
    if (br->bit_count > 0) {
        br->bit_count = 0;
    }
}
