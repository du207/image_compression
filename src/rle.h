#ifndef __RLE_H__
#define __RLE_H__

#include <stdint.h>
#include "dct.h"

#define RLE_INITIAL_CAPACITY 1024

// *DC -> diff of previous dc (12 bits)
// *AC-> RLE encoding
//  -(RUN LENGTH, SIZE) + VALUE
//   -RUN LENGTH: continuous zero length (4 bits)
//   -SIZE: bit size of non-zero VALUE (4 bits)
//   -VALUE: non-zero value (<size> bits)
//
// *EOB -> all zero till end
// *(padding, byte align)


typedef struct {
    uint8_t run_length : 4; // (0-15)
    uint8_t size : 4; // (0-15)
    uint16_t value;
} RLEEntry;

RLEEntry create_rle_entry(uint8_t run_length, uint8_t size, int value);


// (15,0) -> 16 zero

typedef enum {
    RLE_DC, RLE_AC,
    RLE_EOB, // End of Block
} RLEType;


typedef struct {
    RLEType type;
    union {
        // ENC_DC
        uint16_t diff;

        // ENC_AC
        RLEEntry entry;

        // ENC_EOB
    };
} RLEUnit;

typedef struct {
    int units_length;
    int units_capacity; // dynamic allocate
    RLEUnit* units;
} RLEEncoder;

RLEEncoder* create_rle_encoder();
void destroy_rle_encoder(RLEEncoder* re);

void add_rle_unit(RLEEncoder* re, RLEUnit ru);


RLEEncoder* rle_encode(PreEncoding* pe);
void rle_encode_chunk(RLEEncoder* re, Chunk c, int prev_dc);


PreEncoding* rle_decode(RLEEncoder* re, int width, int height);




#endif
