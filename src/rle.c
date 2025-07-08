#include "rle.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
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
    re->prev_dc = 0;
    re->units_length = 0;
    re->units_capacity = RLE_INITIAL_CAPACITY;
    re->units = (RLEUnit*) malloc(sizeof(RLEUnit) * RLE_INITIAL_CAPACITY);
    return re;
}

void destory_rle_encoder(RLEEncoder* re) {
    if (re == NULL) return;

    free_safe(re->units);
    free(re);
}

int add_rle_unit(RLEEncoder* re, RLEUnit ru) {
    if (++re->units_length > re->units_capacity) {
        re->units_capacity *= 2;
        RLEUnit* tmp = (RLEUnit*) realloc(re->units, sizeof(RLEUnit) * re->units_capacity);
        if (tmp == NULL) {
            return -1;
        }
        re->units = tmp;
    }

    re->units[re->units_length - 1] = ru;
}




char* rle_encode(PreEncoding* pe) {
    RLEEncoder* re = create_rle_encoder();
    int chunks_count = pe->c_height * pe->c_width;
    Chunk* chunks = pe->chunks;

    for (int i = 0; i < chunks_count; i++) {
        rle_encode_chunk(re, chunks->c[i]);
    }

    for (int i = 0; i < re->units_length; i++) {
        printf("%d ", re->units[i]);
    }
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

void rle_encode_chunk(RLEEncoder* re, Chunk* c) {
    int16_t dc = c->c[0];
    RLEUnit rle_dc = {
        .type = RLE_DC,
        .diff = dc - re->prev_dc
    };
    add_rle_unit(re, rle_dc);

    int ac, zero_c = 0, ac_val, ac_size;
    int l_i;

    for (l_i = 63; l_i > 0; l_i++) {
        if (c->c[l_i] != 0) break;
    }


    for (int i = 1; i <= l_i; i++) {
        ac = c->c[i];

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
                ac_val = pow(2, ac_size) - 1 + ac;
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
