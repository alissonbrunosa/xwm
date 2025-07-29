#ifndef XALLOC_H
#define XALLOC_H

#include <stdlib.h>

#include "logger.h"

// Memory allocation functions with error checking

void* xcalloc(size_t nmemb, size_t size);

#endif // XALLOC_H
