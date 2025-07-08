#include "awi.h"
#include "bitrw.h"
#include "rle.h"
#include <stdint.h>
#include <stdio.h>


/*
HEADER:
8 bytes : AWESOMEI (in ascii)
4 bytes : width
4 bytes : height
4 bytes : y units length
4 bytes : cb units length
4 bytes : cr units length
*/


static void write_rle_file(BitWriter* bw, RLEEncoder* re) {
    int rle_units_length = re->units_length;
    RLEUnit* units = re->units;
    RLEUnit unit;
    uint8_t upper_bit, lower_bit, unit_symbol;
    uint8_t bitmask8 = (1U << 8) - 1;

    for (int i = 0; i < rle_units_length; i++) {
        unit = units[i];
        if (unit.type == RLE_DC) {
            bit_write(bw, unit.diff, 12);
        } else if (unit.type == RLE_AC) {
            unit_symbol = (unit.entry.run_length << 4) + unit.entry.size;

            bit_write(bw, unit_symbol, 8);
            bit_write(bw, unit.entry.value, unit.entry.size);
        } else { // RLE_EOB
            bit_write(bw, 0, 8);
            bit_writer_flush(bw);
        }
    }
}

void write_awi_file(BitWriter* bw, RLEEncoder* re_y, RLEEncoder* re_cb, RLEEncoder* re_cr, int width, int height) {
    // HEADER: 28 bytes
    AWIHeader header = {
        .title = {'A', 'W', 'E', 'S', 'O', 'M', 'E', 'I'},
        .width = width,
        .height = height,
        .y_units_length = re_y->units_length,
        .cb_units_length = re_cb->units_length,
        .cr_units_length = re_cr->units_length
    };

    if (fwrite(&header, sizeof(AWIHeader), 1, bw->fp) != 1) {
        fprintf(stderr, "File writing error!\n");
        return;
    }

    write_rle_file(bw, re_y);
    write_rle_file(bw, re_cb);
    write_rle_file(bw, re_cr);
}


static void read_rle_file(BitReader* br, RLEEncoder* re, int units_length) {
    uint16_t dc_diff;
    uint8_t symbol;
    uint8_t run_length, size;
    uint32_t value;
    int units_count = 0;

    while (units_count < units_length) {
        // dc
        bit_read(br, 12, (uint32_t*) &dc_diff);

        add_rle_unit(re, (RLEUnit) {
            .type = RLE_DC,
            .diff = dc_diff
        });
        units_count++;

        // ac
        while (1) {
            bit_read(br, 8, (uint32_t*) &symbol);
            if (symbol == 0) break; // eob!

            run_length = symbol >> 4;
            size = symbol & 0b1111;

            bit_read(br, size, &value);

            add_rle_unit(re, (RLEUnit) {
                .type = RLE_AC,
                .entry = create_rle_entry(run_length, size, value)
            });
            units_count++;
        }

        // EOB
        add_rle_unit(re, (RLEUnit) {
            .type = RLE_EOB
        });
        units_count++;

        bit_reader_align_to_byte(br); // padding
    }
}

void read_awi_file(BitReader *br, RLEEncoder* re_y, RLEEncoder* re_cb, RLEEncoder* re_cr, int* width, int* height) {
    AWIHeader header;
    if (fread(&header, sizeof(AWIHeader), 1, br->fp) != 1) {
        fprintf(stderr, "File reading error!\n");
        return;
    }

    *width = header.width;
    *height = header.height;
    int y_unit_length = header.y_units_length;
    int cb_unit_length = header.cb_units_length;
    int cr_unit_length = header.cr_units_length;

    read_rle_file(br, re_y, y_unit_length);
    read_rle_file(br, re_cb, cb_unit_length);
    read_rle_file(br, re_cr, cr_unit_length);
}
