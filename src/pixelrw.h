#ifndef __PIXELRW_H__
#define __PIXELRW_H__

#include <stdbool.h>
#include "block.h"


// for reading pixel data (BMP file, RGB pixel buffers, ..)
typedef struct PixelInput {
    void* context; // caller defined context (can be file pointer or whatever)
    int width;
    int height;

    // if success true, it fail false

    // optional, run before reading blocks (maybe read headers)
    bool (*begin)(struct PixelInput* self);

    // read 16x16 block
    bool (*read_block)(struct PixelInput* self, int bx, int by, RGBBlock16* rgb_out);

    // optional, run after reading blocks
    bool (*end)(struct PixelInput* self);
} PixelInput;

// for writing pixel data (BMP file, RGB pixel buffers, ..)
typedef struct PixelOutput {
    void* context; // caller defined context (can be file pointer or whatever)
    int width;
    int height;

    // if success true, it fail false

    // optional, run before writing blocks
    bool (*begin)(struct PixelOutput* self, int width, int height);

    // write 16x16 block
    // width height for entire image size
    bool (*write_block)(struct PixelOutput* self, int bx, int by, RGBBlock16* block);

    // optional, run after writing blocks
    bool (*end)(struct PixelOutput* self);

} PixelOutput;


#endif
