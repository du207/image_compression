#ifndef __UTILS_H__
#define __UTILS_H__

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

#endif
