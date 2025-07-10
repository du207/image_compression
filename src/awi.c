#include "awi.h"
#include "bitrw.h"
#include "rle.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>


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

void write_awi_file(BitWriter* bw, AWIContent *content, int width, int height) {
    RLEEncoder* re_y = content->re_y;
    RLEEncoder* re_cb = content->re_cb;
    RLEEncoder* re_cr = content->re_cr;

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

    if (units_count != units_length) {
        fprintf(stderr, "Units count not match!\n");
    }
}

void read_awi_file(BitReader *br, AWIContent* content, int* width, int* height) {
    
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

    read_rle_file(br, content->re_y, y_unit_length);
    read_rle_file(br, content->re_cb, cb_unit_length);
    read_rle_file(br, content->re_cr, cr_unit_length);
}


AWIContent* create_awi_content(RLEEncoder* re_y, RLEEncoder* re_cb, RLEEncoder* re_cr) {
    AWIContent* content = (AWIContent*) malloc(sizeof(AWIContent));
    content->re_y = re_y;
    content->re_cb = re_cb;
    content->re_cr = re_cr;
    return content;
}

void destroy_awi_content(AWIContent* content) {
    if (content == NULL) return;

    destroy_rle_encoder(content->re_y);
    destroy_rle_encoder(content->re_cb);
    destroy_rle_encoder(content->re_cr);
    free(content);
}


AWIContent* rgb_img_to_awi_content(RGBImage* rgb_img) {
    // rgb to ycbcr, 4:2:0 sampling
    YCbCrImage* ycbcr_img = rgb_image_to_ycbcr(rgb_img);
    destroy_rgb_image(rgb_img); // rgb now useless

    YCbCrImage* sampled_img = ycbcr_420_sampling(ycbcr_img);
    destroy_ycbcr_image(ycbcr_img); // ycbcr now useless

    // DCT transform
    PreEncoding* pe_y = dct_channel(sampled_img->y, QM_LUMA);
    PreEncoding* pe_cb = dct_channel(sampled_img->cb, QM_CHROM);
    PreEncoding* pe_cr = dct_channel(sampled_img->cr, QM_CHROM);
    destroy_ycbcr_image(sampled_img); // sampled now useless


    // RLE Encode
    RLEEncoder* re_y = rle_encode(pe_y);
    destroy_pre_encoding(pe_y);

    RLEEncoder* re_cb = rle_encode(pe_cb);
    destroy_pre_encoding(pe_cb);

    RLEEncoder* re_cr = rle_encode(pe_cr);
    destroy_pre_encoding(pe_cr);

    AWIContent* content = create_awi_content(re_y, re_cb, re_cr);
    return content;
}

RGBImage* awi_content_to_rgb_img(AWIContent* content, int width, int height) {
    int sub_width = (width + 1) / 2; // ceil
    int sub_height = (height + 1) / 2;

    RLEEncoder* re_y = content->re_y;
    RLEEncoder* re_cb = content->re_cb;
    RLEEncoder* re_cr = content->re_cr;

    // RLE Decoding
    PreEncoding* pe_y = rle_decode(re_y, width, height);
    PreEncoding* pe_cb = rle_decode(re_cb, sub_width, sub_height);
    PreEncoding* pe_cr = rle_decode(re_cr, sub_width, sub_height);
    destroy_rle_encoder(re_y);
    destroy_rle_encoder(re_cb);
    destroy_rle_encoder(re_cr);


    // in-DCT transform
    Channel* c_y = in_dct_channel(pe_y, width, height, QM_LUMA);
    Channel* c_cb = in_dct_channel(pe_cb, sub_width, sub_height, QM_CHROM);
    Channel* c_cr = in_dct_channel(pe_cr, sub_width, sub_height, QM_CHROM);
    destroy_pre_encoding(pe_y);
    destroy_pre_encoding(pe_cb);
    destroy_pre_encoding(pe_cr);

    
    // in-subsampling
    YCbCrImage ycbcr_sampled_img = {
        .is_subsampled = true,
        .y = c_y,
        .cb = c_cb,
        .cr = c_cr
    };
    
    YCbCrImage* ycbcr_img = ycbcr_420_inverse_sampling(&ycbcr_sampled_img);
    
    RGBImage* rgb_img = ycbcr_image_to_rgb(ycbcr_img);

    destroy_channel(c_y);
    destroy_channel(c_cb);
    destroy_channel(c_cr);
    destroy_ycbcr_image(ycbcr_img);

    return rgb_img;
}
