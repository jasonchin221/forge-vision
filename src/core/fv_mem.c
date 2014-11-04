#include <stdlib.h>

void *
fv_alloc(size_t size)
{
    return malloc(size);
}

void
_fv_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    free(ptr);
}
