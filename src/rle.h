#ifndef __RLE_H__
#define __RLE_H__

#include <stdint.h>
#include <stdbool.h>
#include "bitrw.h"
#include "dct.h"


// *DC -> diff of previous dc (12 bits)
// *AC-> RLE encoding
//  -(RUN LENGTH, SIZE) + VALUE
//   -RUN LENGTH: continuous zero length (4 bits)
//   -SIZE: bit size of non-zero VALUE (4 bits)
//   -VALUE: non-zero value (<size> bits)
//
// *EOB -> all zero till end
// *(padding, byte align)

// (15,0) -> 16 zero


typedef struct {
    uint8_t run_length : 4; // (0-15)
    uint8_t size : 4; // (0-15)
    uint16_t value;
} RLEEntry;

bool rle_encode_chunk_and_write(BitsOutput* bo, const Chunk* cnk, int prev_dc);
bool read_and_rle_decode_chunk(BitsInput* bi, Chunk* cnk, int prev_dc);



#endif
