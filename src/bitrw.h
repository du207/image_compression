#ifndef __BITRW_H__
#define __BITRW_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

// write to the buffer and
// write to the file when buffer 8 bits all filled
typedef struct BitsOutput {
    void* context; // caller defined context (can be file pointer or whatever)

    uint8_t buffer; // a byte
    int bit_count; // 0~7

    // if success true, it fail false

    // write bits chunk
    bool (*write)(struct BitsOutput* self, const void* data, size_t size);

} BitsOutput;

// if success true, if fail false
bool bits_output_write(BitsOutput* bo, uint32_t value, int bits_size);

// add padding to align byte and write to file
bool bits_output_flush(BitsOutput* bo);




typedef struct BitsInput {
    void* context; // caller defined context (can be file pointer or whatever)

    uint8_t buffer; // a byte
    int bit_count; // 0~7

    // if success true, if fail false

    // read bits chunk
    bool (*read)(struct BitsInput* self, void* buf, size_t size);

} BitsInput;

// 0 <= bits_size <= 32 (max: 4bytes = 32bits)
// return 0 when EOF
bool bits_input_read(BitsInput* bo, int bits_size, uint32_t* out);
// align to byte
void bits_input_align_to_byte(BitsInput* bo);

#endif
