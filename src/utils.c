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

    char* out = (char*) malloc(sizeof(char) * len_str);
    strcpy(out, str);
    strcpy(out + (len_str - len_end), end);

    return out;
}
