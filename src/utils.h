#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdlib.h>
#include <stdint.h>

// check if pointer is null before free
static inline void free_safe(void* ptr) {
    if (ptr != NULL) free(ptr);
}

static inline uint8_t clamp_uint8(double value) {
    if (value > 255)
        return 255;
    else if (value < 0)
        return 0;
    else
        return (uint8_t) value;
}


#endif
