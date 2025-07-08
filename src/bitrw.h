#ifndef __BITRW_H__
#define __BITRW_H__

#include <stdint.h>
#include <stdio.h>

// write to the buffer and
// write to the file when buffer 8 bits all filled
typedef struct {
    FILE* fp;
    uint8_t buffer; // a byte
    int bit_count; // 0~7
} BitWriter;

BitWriter* create_bit_writer(FILE* fp);
// 'FILE* fp' won't be closed
void destroy_bit_writer(BitWriter* bw);
// 0 <= bits_size <= 32
void bit_write(BitWriter* bw, uint32_t value, int bits_size);
// add padding to align byte and write to file
void bit_writer_flush(BitWriter* bw);




typedef struct {
    FILE* fp;
    uint8_t buffer; // a byte
    int bit_count; // 0~7
} BitReader;

BitReader* create_bit_reader(FILE* fp);
// 'FILE* fp' won't be closed
void destroy_bit_reader(BitReader* br);
// 0 <= bits_size <= 32 (max: 4bytes = 32bits)
// return 0 when EOF
int bit_read(BitReader* br, int bits_size, uint32_t* out);
// align to byte
void bit_reader_align_to_byte(BitReader* br);

#endif
