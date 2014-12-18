#ifndef __FV_DEBUG_H__
#define __FV_DEBUG_H__

#include <stdio.h>
#include <unistd.h>

#define FV_DEBUG_MAX_FILENAME_LEN       100

#define FV_ASSERT(expr) \
    do {\
        if (!(expr)) { \
            fprintf(stderr, "Error in (%s %s %d)\n", \
                    __FILE__, __FUNCTION__,__LINE__); \
            _exit(0); \
        } \
    } while (0)

enum {
    FV_PIC_FORMAT_JPG,
    FV_PIC_FORMAT_PNG,
    FV_PIC_FORMAT_BMP,
    FV_PIC_FORMAT_MAX,
};

typedef fv_s32 (*fv_save_img_func)(char *file_name, fv_mat_t *mat);

extern void fv_debug_save_img(char *file_name, fv_mat_t *mat);
extern void fv_debug_set_print(fv_save_img_func func);

#endif
