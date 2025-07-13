#include "utils.h"

#include <stdlib.h>
#include <string.h>

void free_safe(void* ptr) {
    if (ptr != NULL) free(ptr);
}

uint8_t clamp_uint8(double value) {
    if (value > 255)
        return 255;
    else if (value < 0)
        return 0;
    else
        return (uint8_t) value;
}


bool is_end_with(const char* str, const char* end) {
    int len_str = strlen(str);
    int len_end = strlen(end);

    if (len_str < len_end) return false;

    return strcmp(str + (len_str - len_end), end) == 0;
}


char* replace_str_end(const char* str, const char* end) {
    int len_str = strlen(str);
    int len_end = strlen(end);

    if (len_str < len_end) return NULL;

    char* out = (char*) malloc(len_str + 1);
    strcpy(out, str);
    strcpy(out + (len_str - len_end), end);

    return out;
}


void print_chunk(const char* title, const int* data) {
    printf("--- %s ---\n", title);
    for (int i = 0; i < 64; i++) {
        printf("%d ", data[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}


void print_chunk_u8(const char* title, const uint8_t* data) {
    printf("--- %s ---\n", title);
    for (int i = 0; i < 64; i++) {
        printf("%u ", data[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void print_block_u8(const char* title, const Block_u8* block) {
    printf("--- %s ---\n", title);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            printf("%d ", block->b[y][x]);
        }
        printf("\n");
    }
    printf("\n");
}
