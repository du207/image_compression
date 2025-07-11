#ifndef __AWI_H__
#define __AWI_H__

#include "bitrw.h"
#include "rle.h"
#include <stdint.h>
#include <stdbool.h>

/*
HEADER:
8 bytes : AWESOMEI (in ascii)
4 bytes : width
4 bytes : height

CONTENT:
// *DC -> diff of previous dc (12 bits)
// *AC-> RLE encoding
//  -(RUN LENGTH, SIZE) + VALUE
// *EOB(0x00)
// *(padding, byte align)
*/

#include "block.h"
#include <stdint.h>


typedef struct {
    void* context; // caller defined context (can be file pointer or whatever)
    int width;
    int height;
    
    // if success true, it fail false
    
    // optional, run before reading blocks (maybe read headers)
    bool (*begin)(PixelInput* self);

    // read 16x16 block
    bool (*read_block)(PixelInput* self, int bx, int by, RGBBlock16* rgb_out);

    // optional, run after reading blocks
    bool (*end)(PixelInput* self);
} PixelInput;

// for writing pixel data (BMP file, RGB pixel buffers, ..)
typedef struct PixelOutput {
    void* context; // caller defined context (can be file pointer or whatever)
    int width;
    int height;

    // if success true, it fail false

    // optional, run before writing blocks
    bool (*begin)(struct PixelOutput* self);

    // write 16x16 block
    // width height for entire image size
    bool (*write_block)(struct PixelOutput* self, int bx, int by, RGBBlock16* block);
    
    // optional, run after writing blocks
    bool (*end)(struct PixelOutput* self);

} PixelOutput;

typedef struct BitsInput {
    void* context; // caller defined context (can be file pointer or whatever)
    int units_length;
    
    // if success true, it fail false
    
    // optional, run before reading bits (maybe read headers)
    bool (*begin)(struct BitsInput* self);

    // read bits chunk
    bool (*read_bits)(struct BitsInput* self, uint32_t* bits, int bit_size);

    // optional, run after reading bits
    bool (*end)(struct BitsInput* self);
} BitsInput;

// for writing bits chunks (.AWI file encoded datas)
typedef struct BitsOutput {
    void* context; // caller defined context (can be file pointer or whatever)
    int units_length;

    // optional, run before writing bits
    bool (*begin)(struct BitsOutput* self);

    // write bits chunk
    bool (*write_bits)(struct BitsOutput* self, uint32_t bits, int bit_size);

    // optional, run after writing bits
    bool (*end)(struct BitsOutput* self);

} BitsOutput;

#endif
