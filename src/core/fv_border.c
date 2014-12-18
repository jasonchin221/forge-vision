#include <string.h>

#include "fv_types.h"
#include "fv_imgproc.h"
#include "fv_debug.h"
#include "fv_border.h"

static fv_s32 fv_border_replicate_get_value(fv_s32 index, fv_s32 border);
static fv_s32 fv_border_reflect_get_value(fv_s32 index, fv_s32 border);
static fv_s32 fv_border_reflect101_get_value(fv_s32 index, fv_s32 border);
static fv_s32 fv_border_wrap_get_value(fv_s32 index, fv_s32 border);

static fv_border_value_t fv_border_value[FV_BORDER_MAX] = {
    {FV_BORDER_CONSTANT, NULL},
    {
        FV_BORDER_REPLICATE, 
        fv_border_replicate_get_value, 
    },
    {
        FV_BORDER_REFLECT, 
        fv_border_reflect_get_value, 
    },
    {
        FV_BORDER_WRAP, 
        fv_border_wrap_get_value, 
    },
    {
        FV_BORDER_REFLECT_101, 
        fv_border_reflect101_get_value, 
    },
};

static fv_s32
fv_border_replicate_get_value(fv_s32 index, fv_s32 border)
{
    if (index < 0) {
        return 0;
    }

    if (index > border - 1) {
        return border - 1;
    }

    return index;
}

static fv_s32
fv_border_reflect_get_value(fv_s32 index, fv_s32 border)
{
    fv_s32      i;

    i = index;
    if (index < 0) {
        i = -1 - index;
    }

    if (index > border - 1) {
        i = 2*(border) - 1 - index;
    }

    FV_ASSERT(i >= 0 && i < border);

    return i;
}

static fv_s32
fv_border_reflect101_get_value(fv_s32 index, fv_s32 border)
{
    fv_s32      i;

    i = index;
    if (index < 0) {
        i = - index;
    }

    if (index > border - 1) {
        i = 2*(border - 1) - index;
    }

    FV_ASSERT(i >= 0 && i < border);

    return i;
}

static fv_s32
fv_border_wrap_get_value(fv_s32 index, fv_s32 border)
{
    fv_s32      i;

    i = index;
    if (index < 0) {
        i = border - 1 + index;
    }

    if (index > border - 1) {
        i = index - border;
    }

    FV_ASSERT(i >= 0 && i < border);

    return i;
}


fv_s32
fv_border_get_value(fv_u32 border_type, fv_s32 index, fv_s32 border)
{
    fv_border_value_t   *value;

    FV_ASSERT(border_type < FV_BORDER_MAX);

    value = &fv_border_value[border_type];
    FV_ASSERT(value != NULL && value->bv_type == border_type);
    
    return value->bv_get_value(index, border);
}

#define fv_border_make_row_core(dst, src, dst_len, src_step, width, cn, \
        krow, ax, border_type) \
    do { \
        fv_s32      i; \
        fv_s32      k; \
        fv_s32      row; \
        FV_ASSERT(dst_len >= src_step + (krow - 1)*cn); \
        memcpy(dst + ax, src, src_step); \
        if (border_type != FV_BORDER_CONSTANT) { \
            for (k = 0; k < ax; k++) { \
                row = fv_border_get_value(border_type, k - ax, width); \
                for (i = 0; i < cn; i++) { \
                    dst[k + cn - 1] = src[row + cn - 1]; \
                } \
            } \
            dst += (width + ax)*cn; \
            for (k = 0; k < krow - 1 - ax; k++) { \
                row = fv_border_get_value(border_type, width + k, width); \
                for (i = 0; i < cn; i++) { \
                    dst[k + cn - 1] = src[row + cn - 1]; \
                } \
            } \
        } \
    } while(0)


static void
fv_border_make_row_8u(fv_u8 *dst, fv_u8 *src, fv_u32 dst_len, 
            fv_s32 src_step, fv_s32 width, fv_u32 cn, fv_s32 krow, 
            fv_s32 ax, fv_s32 border_type) 
{
    fv_border_make_row_core(dst, src, dst_len, src_step, width, cn,
            krow, ax, border_type);
}

static void
fv_border_make_row_8s(fv_s8 *dst, fv_s8 *src, fv_u32 dst_len, 
            fv_s32 src_step, fv_s32 width, fv_u32 cn, fv_s32 krow, 
            fv_s32 ax, fv_s32 border_type) 
{
    fv_border_make_row_core(dst, src, dst_len, src_step, width, cn,
            krow, ax, border_type);
}

static void
fv_border_make_row_16u(fv_u16 *dst, fv_u16 *src, fv_u32 dst_len, 
            fv_s32 src_step, fv_s32 width, fv_u32 cn, fv_s32 krow, 
            fv_s32 ax, fv_s32 border_type) 
{
    fv_border_make_row_core(dst, src, dst_len, src_step, width, cn,
            krow, ax, border_type);
}

static void
fv_border_make_row_16s(fv_s16 *dst, fv_s16 *src, fv_u32 dst_len, 
            fv_s32 src_step, fv_s32 width, fv_u32 cn, fv_s32 krow, 
            fv_s32 ax, fv_s32 border_type) 
{
    fv_border_make_row_core(dst, src, dst_len, src_step, width, cn,
            krow, ax, border_type);
}

static void
fv_border_make_row_32s(fv_s32 *dst, fv_s32 *src, fv_u32 dst_len, 
            fv_s32 src_step, fv_s32 width, fv_u32 cn, fv_s32 krow, 
            fv_s32 ax, fv_s32 border_type) 
{
    fv_border_make_row_core(dst, src, dst_len, src_step, width, cn,
            krow, ax, border_type);
}

static void
fv_border_make_row_32f(float *dst, float *src, fv_u32 dst_len, 
            fv_s32 src_step, fv_s32 width, fv_u32 cn, fv_s32 krow, 
            fv_s32 ax, fv_s32 border_type) 
{
    fv_border_make_row_core(dst, src, dst_len, src_step, width, cn,
            krow, ax, border_type);
}

static void
fv_border_make_row_64f(double *dst, double *src, fv_u32 dst_len, 
            fv_s32 src_step, fv_s32 width, fv_u32 cn, fv_s32 krow, 
            fv_s32 ax, fv_s32 border_type) 
{
    fv_border_make_row_core(dst, src, dst_len, src_step, width, cn,
            krow, ax, border_type);
}

static fv_border_make_row_func fv_border_make_tab[] = {
    (fv_border_make_row_func)fv_border_make_row_8u,
    (fv_border_make_row_func)fv_border_make_row_8s,
    (fv_border_make_row_func)fv_border_make_row_16u,
    (fv_border_make_row_func)fv_border_make_row_16s,
    (fv_border_make_row_func)fv_border_make_row_32s,
    (fv_border_make_row_func)fv_border_make_row_32f,
    (fv_border_make_row_func)fv_border_make_row_64f,
};

#define fv_border_make_row_tabsize \
    (sizeof(fv_border_make_tab)/sizeof(fv_border_make_row_func))

fv_border_make_row_func 
fv_border_get_func(fv_u32 depth)
{
    FV_ASSERT(depth < fv_border_make_row_tabsize);

    return fv_border_make_tab[depth];
}

