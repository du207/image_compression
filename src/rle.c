#include "rle.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dct.h"
#include "utils.h"

#define BIT4_SIZE 16

RLEEntry create_rle_entry(uint8_t run_length, uint8_t size, int value) {
    assert(run_length < BIT4_SIZE && size < BIT4_SIZE);

    RLEEntry e = {
        .run_length = run_length,
        .size = size,
        .value = value
    };

    return e;
}

RLEEncoder* create_rle_encoder() {
    RLEEncoder* re = (RLEEncoder*) malloc(sizeof(RLEEncoder));
    re->units_length = 0;
    re->units_capacity = RLE_INITIAL_CAPACITY;
    re->units = (RLEUnit*) malloc(sizeof(RLEUnit) * RLE_INITIAL_CAPACITY);
    return re;
}

void destroy_rle_encoder(RLEEncoder* re) {
    if (re == NULL) return;

    free_safe(re->units);
    free(re);
}

void add_rle_unit(RLEEncoder* re, RLEUnit ru) {
    if (++re->units_length > re->units_capacity) {
        re->units_capacity *= 2;
        RLEUnit* tmp = (RLEUnit*) realloc(re->units, sizeof(RLEUnit) * re->units_capacity);
        if (tmp == NULL) {
            fprintf(stderr, "Memory reallocation error!\n");
            return;
        }
        re->units = tmp;
    }

    re->units[re->units_length - 1] = ru;
}



RLEEncoder* rle_encode(PreEncoding* pe) {
    RLEEncoder* re = create_rle_encoder();
    int chunks_count = pe->c_height * pe->c_width;
    Chunk* chunks = pe->chunks;
    int prev_dc = 0;

    for (int i = 0; i < chunks_count; i++) {
        rle_encode_chunk(re, chunks[i], prev_dc);
        prev_dc = chunks[i].c[0];
    }

    return re;
}

// get bit size of abs(a)
static int get_bit_size(int a) {
    int abs_a = abs(a);
    int count = 0;

    while(abs_a > 0) {
        count++;
        abs_a >>= 1;
    }
    return count;
}

void rle_encode_chunk(RLEEncoder* re, Chunk c, int prev_dc) {
    int dc = c.c[0];
    int diff = dc - prev_dc;

    RLEUnit rle_dc = {
        .type = RLE_DC,
        .diff = diff >= 0 ? diff : 4095 + diff // 2^12 - 1 + diff
    };

    add_rle_unit(re, rle_dc);

    int ac, zero_c = 0, ac_val, ac_size;
    int l_i = 63;

    while (l_i > 0) {
        if (c.c[l_i] != 0) break;
        l_i--;
    }

    for (int i = 1; i <= l_i; i++) {
        ac = c.c[i];

        if (ac == 0) {
            if (zero_c < 15) {
                zero_c++;
            } else { // (15, 0) symbol
                add_rle_unit(re, (RLEUnit) {
                    .type = RLE_AC,
                    .entry = create_rle_entry(15, 0, 0)
                });
                zero_c = 0;
            }
        } else {
            ac_size = get_bit_size(ac);
            if (ac > 0) {
                ac_val = ac;
            } else {
                ac_val = (1 << ac_size) - 1 + ac;
            }

            add_rle_unit(re, (RLEUnit) {
                .type = RLE_AC,
                .entry = create_rle_entry(zero_c, ac_size, ac_val)
            });
            zero_c = 0;
        }
    }

    add_rle_unit(re, (RLEUnit) {
        .type = RLE_EOB
    });
}



PreEncoding* rle_decode(RLEEncoder* re, int width, int height) {
    int c_width = (width + 7) / 8; // ceil
    int c_height = (height + 7) / 8;
    int chunks_count = c_width * c_height;

    PreEncoding* pe = create_pre_encoding(c_width, c_height);

    int rle_unit_count = re->units_length;
    RLEUnit* units = re->units;
    RLEUnit unit;
    Chunk* chunks = pe->chunks;

    int i, j;
    int chunk_idx = 0, cc_idx = 0;
    int diff, prev_dc = 0, dc;
    uint32_t val; uint8_t size;

    for (i = 0; i < rle_unit_count; i++) {
        unit = units[i];

        if (unit.type == RLE_DC) {
            diff = unit.diff > 2047
                ? unit.diff - 4095
                : unit.diff;

            dc = diff + prev_dc;
            chunks[chunk_idx].c[cc_idx++] = dc;
            prev_dc = dc;
        } else if (unit.type == RLE_AC) {
            for (j = 0; j < unit.entry.run_length; j++) {
                chunks[chunk_idx].c[cc_idx++] = 0;
            }

            val = unit.entry.value;
            size = unit.entry.size;

            if (size > 0) {
                chunks[chunk_idx].c[cc_idx++] = val & (1 << (size - 1))
                    ? val                     // MSB == 1: positive num
                    : val - (1 << size) + 1;  // MSB == 0: negative num
            } else { // (15, 0) val=0 symbol
                chunks[chunk_idx].c[cc_idx++] = 0;
            }
        } else { // EOB
            for (j = cc_idx; j < 64; j++) {
                chunks[chunk_idx].c[j] = 0;
            }

            chunk_idx++;
            cc_idx = 0;
        }
    }

    return pe;
}
