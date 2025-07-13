#include "bitrw.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>


bool bits_output_write(BitsOutput* bo, uint32_t value, int bits_size) {
    assert(bits_size <= 32 && bits_size >= 0);

    for (int i = bits_size - 1; i >= 0; i--) {
        bo->buffer <<= 1;
        bo->buffer |= (value >> i) & 1;
        bo->bit_count++;

        if (bo->bit_count == 8) {
            if (!bo->write(bo, &bo->buffer, 1)) {
                fprintf(stderr, "File bit write error!\n");
                return false;
            }
            bo->buffer = 0;
            bo->bit_count = 0;
        }
    }

    return true;
}


bool bits_output_flush(BitsOutput* bo) {
    if (bo->bit_count > 0) {
        bo->buffer <<= (8 - bo->bit_count);
        if (!bo->write(bo, &bo->buffer, 1)) {
            fprintf(stderr, "File bit flush error!\n");
            return false;
        }
        bo->buffer = 0;
        bo->bit_count = 0;
    }

    return true;
}


bool bits_input_read(BitsInput* bo, int bits_size, uint32_t* out) {
    assert(bits_size >= 0 && bits_size <= 32);

    *out = 0;
    for (int i = 0; i < bits_size; i++) {
        if (bo->bit_count == 0) {
            uint8_t c;
            if (!bo->read(bo, &c, 1)) {
                fprintf(stderr, "File bit read error!\n");
                return false;
            }

            bo->buffer = c;
            bo->bit_count = 8;
        }

        *out <<= 1;
        *out |= (bo->buffer >> 7) & 1;
        bo->buffer <<= 1;
        bo->bit_count--;
    }

    return true;
}


void bits_input_align_to_byte(BitsInput* bi) {
    if (bi->bit_count > 0) {
        bi->bit_count = 0;
    }
}
