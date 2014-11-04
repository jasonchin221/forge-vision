#ifndef __FV_MEM_H__
#define __FV_MEM_H__


extern void *fv_alloc(size_t size);
extern void _fv_free(void *ptr);

#define fv_free(ptr) (_fv_free(*(ptr)), *(ptr) = 0)

#endif
