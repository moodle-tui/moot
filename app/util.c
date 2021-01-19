#include <stdlib.h>
#include "app.h"

void *xmalloc(size_t size, Error *error) {
    void *data = malloc(size);
    if (!data && size)
        *error = ERR_ALLOCATE;
    return data;
}

