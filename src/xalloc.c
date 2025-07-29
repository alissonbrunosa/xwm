#include <stdlib.h>

#include "wm_logger.h"

void* xcalloc(size_t nmemb, size_t size) {
    void* ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        FATAL("Memory allocation failed: %zu elements of size %zu", nmemb, size);
        exit(1);
    }

    return ptr;
}

