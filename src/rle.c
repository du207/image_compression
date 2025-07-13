#include "rle.h"

#include "bitrw.h"
#include <stdlib.h>


static bool write_rle_dc(BitsOutput* bo, uint32_t diff) {
    if (!bits_output_write(bo, diff, 12)) {
        fprintf(stderr, "Writing DC diff error!\n");
        return false;
    }
    return true;
}

static bool write_rle_ac(BitsOutput* bo, RLEEntry* entry) {
    uint8_t unit_symbol = (entry->run_length << 4) + entry->size;

    bool w1 = bits_output_write(bo, unit_symbol, 8);
    bool w2 = bits_output_write(bo, entry->value, entry->size);

    if (!w1 || !w2) {
        fprintf(stderr, "Writing AC entry error!\n");
        return false;
    }

    return true;
}

static bool write_rle_eob(BitsOutput* bo) {
    bool w1 = bits_output_write(bo, 0, 8);

   if (!w1) {
       fprintf(stderr, "Writing EOB error!\n");
       return false;
   }

    return true;
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

bool rle_encode_chunk_and_write(BitsOutput* bo, const Chunk* cnk, int prev_dc) {
    int dc = cnk->c[0];
    int diff = dc - prev_dc;
    uint32_t diff_raw = diff >= 0 ? diff : 4095 + diff; // 2^12 - 1 + diff

    if (!write_rle_dc(bo, diff_raw)) return false;

    int zero_c = 0;
    int l_i = 63;

    while (l_i > 0) { // find last non-zero
        if (cnk->c[l_i] != 0) break;
        l_i--;
    }

    for (int i = 1; i <= l_i; i++) {
        int ac = cnk->c[i];

        if (ac == 0) {
            if (zero_c < 15) {
                zero_c++;
            } else { // (15, 0) symbol
                RLEEntry entry = {
                    .run_length = 15,
                    .size = 0,
                    .value = 0
                };

                if (!write_rle_ac(bo, &entry)) return false;

                zero_c = 0;
            }
        } else {
            int ac_val, ac_size = get_bit_size(ac);

            if (ac > 0) {
                ac_val = ac;
            } else {
                ac_val = (1 << ac_size) - 1 + ac;
            }

            RLEEntry entry = {
                .run_length = zero_c,
                .size = ac_size,
                .value = ac_val
            };


            if (!write_rle_ac(bo, &entry)) return false;

            zero_c = 0;
        }
    }

    if (!write_rle_eob(bo)) return false;
    if (!bits_output_flush(bo)) return false;

    return true;
}


bool read_and_rle_decode_chunk(BitsInput* bi, Chunk* cnk, int prev_dc) {
    int cc_idx = 0;

    // DC
    uint32_t dc_diff_raw;
    if (!bits_input_read(bi, 12, &dc_diff_raw)) {
        fprintf(stderr, "DC Reading error!\n");
        return false;
    }

    int dc_diff = dc_diff_raw > 2047
                    ? dc_diff_raw - 4095
                    : dc_diff_raw;

    int dc = dc_diff + prev_dc;
    cnk->c[cc_idx++] = dc; // cnk->c[0] = dc

    uint8_t run_length, size;
    uint32_t symbol, value;

    // AC
    while (1) {
        if (!bits_input_read(bi, 8, &symbol)) {
            fprintf(stderr, "AC: %d Reading symbol error!\n", cc_idx);
            return false;
        }
        run_length = (symbol >> 4) & 0b1111;
        size = symbol & 0b1111;

        if (symbol == 0) break; // EOB

        if (!bits_input_read(bi, size, &value)) {
            fprintf(stderr, "AC: %d Reading value error!\n", cc_idx);
            return false;
        }

        for (int i = 0; i < run_length; i++) {
            cnk->c[cc_idx++] = 0;
        }

        if (size > 0) {
            cnk->c[cc_idx++] = value & (1 << (size - 1))
                ? value                     // MSB == 1: positive num
                : value - (1 << size) + 1;  // MSB == 0: negative num
        } else { // size = 0
            cnk->c[cc_idx++] = 0;
        }
    }

    // EOB
    for (int i = cc_idx; i < 64; i++) {
        cnk->c[i] = 0;
    }

    bits_input_align_to_byte(bi);

    return true;
}
