#ifndef __UTILS_H__
#define __UTILS_H__

#include "block.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// check if pointer is null before free
void free_safe(void* ptr);
uint8_t clamp_uint8(double value);
bool is_end_with(const char* str, const char* end);

// returned string is on heap
char* replace_str_end(const char* str, const char* end);

void print_chunk(const char* title, const int* data);
void print_chunk_u8(const char* title, const uint8_t* data);
void print_block_u8(const char* title, const Block_u8* block);

#endif
