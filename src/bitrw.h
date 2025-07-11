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




typedef struct {
    void* context; // caller defined context (can be file pointer or whatever)

    // write to buffer
    // return read bytes length
    size_t (*read)(void* context, void* buf, size_t size);

    // move offset
    // return 0 if success, non-zero if fail
    size_t (*seek)(void* context, long offset, int whence);
    
} InputStream;

// for writing pixel data (BMP file, RGB pixel buffers, ..)
typedef struct {
    void* context; // caller defined context (can be file pointer or whatever)
    
    // optional, run before writing blocks
    void (*begin)(void* context, int width, int height);

    // write 8x8 block
    // width height for entire image size
    void (*write_block)(void* context, int bx, int by, int width, int height);
    
    // optional, run after writing blocks
    void (*end)(void* context);
    
} OutputPixelStream;

// for writing bits chunks (.AWI file encoded datas)
typedef struct {
    void* context; // caller defined context (can be file pointer or whatever)

    // optional, run before writing bits
    void (*begin)(void* context, int width, int height);

    // write bits chunk
    // width height for entire image size
    void (*write_bits)(void* context, uint32_t bits, int bit_size);

    // optional, run after writing bits
    void (*end)(void* context);

} OutputBitStream;



#endif
