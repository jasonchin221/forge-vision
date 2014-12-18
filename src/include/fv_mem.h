#ifndef __FV_MEM_H__
#define __FV_MEM_H__


extern void *fv_alloc(size_t size);
extern void *fv_calloc(size_t size);
extern void _fv_free(void *ptr);

#if 1
#define fv_free(ptr) (_fv_free(*(ptr)), *(ptr) = NULL)
#else
#define fv_free(ptr) \
    do { \
        fprintf(stdout, "free %p in %s %d\n", *ptr, __FUNCTION__,__LINE__); \
        _fv_free(*(ptr));  \
        *(ptr) = NULL; \
    } while(0)
#endif

#endif
