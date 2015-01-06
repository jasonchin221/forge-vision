
#include <math.h>
#include "fv_types.h"
#include "fv_math.h"
#include "fv_debug.h"

#define fv_pow_core(dst, src, total, pfunc, p, castop) \
    do { \
        fv_s32      i; \
        for (i = 0; i < total; i++) { \
            dst[i] = castop((pfunc(src[i], p))); \
        } \
    } while(0)

static void 
fv_pow_8u(fv_u8 *dst, fv_u8 *src, fv_s32 total, double p)
{
    fv_pow_core(dst, src, total, pow, p, fv_saturate_cast_8u);
}

static void 
fv_pow_8s(fv_s8 *dst, fv_s8 *src, fv_s32 total, double p)
{
    fv_pow_core(dst, src, total, pow, p, fv_saturate_cast_8s);
}

static void 
fv_pow_16u(fv_u16 *dst, fv_u16 *src, fv_s32 total, double p)
{
    fv_pow_core(dst, src, total, pow, p, fv_saturate_cast_16u);
}

static void 
fv_pow_16s(fv_s16 *dst, fv_s16 *src, fv_s32 total, double p)
{
    fv_pow_core(dst, src, total, pow, p, fv_saturate_cast_16s);
}

static void 
fv_pow_32s(fv_s32 *dst, fv_s32 *src, fv_s32 total, double p)
{
    fv_pow_core(dst, src, total, pow, p, fv_saturate_cast_32s);
}

static void 
fv_pow_32f(float *dst, float *src, fv_s32 total, double p)
{
    fv_pow_core(dst, src, total, powf, p, fv_saturate_cast_32f);
}

static void 
fv_pow_64f(double *dst, double *src, fv_s32 total, double p)
{
    fv_s32      i;

    for (i = 0; i < total; i++) {
        dst[i] = pow(src[i], p);
    }
}

typedef void (*fv_pow_func)(void *, void *, fv_s32, double);

static fv_pow_func fv_pow_tab[] = {
    (fv_pow_func)fv_pow_8u,
    (fv_pow_func)fv_pow_8s,
    (fv_pow_func)fv_pow_16u,
    (fv_pow_func)fv_pow_16s,
    (fv_pow_func)fv_pow_32s,
    (fv_pow_func)fv_pow_32f,
    (fv_pow_func)fv_pow_64f,
};

#define fv_pow_tab_size \
    (sizeof(fv_pow_tab)/sizeof(fv_pow_func))

static fv_pow_func 
fv_get_pow_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_pow_tab_size);

    return fv_pow_tab[depth];
}

#define fv_sqrt_core(dst, src, total, sqrt_func) \
    do { \
        fv_s32      i; \
        for (i = 0; i < total; i++) { \
            dst[i] = sqrt_func(src[i]); \
        } \
    } while(0)

static void 
fv_sqrt_8u(fv_u8 *dst, fv_u8 *src, fv_s32 total)
{
    fv_sqrt_core(dst, src, total, sqrt);
}

static void 
fv_sqrt_8s(fv_s8 *dst, fv_s8 *src, fv_s32 total)
{
    fv_sqrt_core(dst, src, total, sqrt);
}

static void 
fv_sqrt_16u(fv_u16 *dst, fv_u16 *src, fv_s32 total)
{
    fv_sqrt_core(dst, src, total, sqrt);
}

static void 
fv_sqrt_16s(fv_s16 *dst, fv_s16 *src, fv_s32 total)
{
    fv_sqrt_core(dst, src, total, sqrt);
}

static void 
fv_sqrt_32s(fv_s32 *dst, fv_s32 *src, fv_s32 total)
{
    fv_sqrt_core(dst, src, total, sqrt);
}

static void 
fv_sqrt_32f(float *dst, float *src, fv_s32 total)
{
    fv_sqrt_core(dst, src, total, sqrtf);
}

static void 
fv_sqrt_64f(double *dst, double *src, fv_s32 total)
{
    fv_sqrt_core(dst, src, total, sqrt);
}


typedef void (*fv_sqrt_func)(void *, void *, fv_s32);

static fv_sqrt_func fv_sqrt_tab[] = {
    (fv_sqrt_func)fv_sqrt_8u,
    (fv_sqrt_func)fv_sqrt_8s,
    (fv_sqrt_func)fv_sqrt_16u,
    (fv_sqrt_func)fv_sqrt_16s,
    (fv_sqrt_func)fv_sqrt_32s,
    (fv_sqrt_func)fv_sqrt_32f,
    (fv_sqrt_func)fv_sqrt_64f,
};

#define fv_sqrt_tab_size \
    (sizeof(fv_sqrt_tab)/sizeof(fv_sqrt_func))

static fv_sqrt_func 
fv_get_sqrt_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_sqrt_tab_size);

    return fv_sqrt_tab[depth];
}

void
fv_pow(fv_mat_t *dst, fv_mat_t *src, double pow)
{
    fv_pow_func     pow_func;
    fv_sqrt_func    sqrt_func;

    FV_ASSERT(dst->mt_atr == src->mt_atr &&
            dst->mt_total == src->mt_total);

    if (fv_double_eq(pow, 0.5)) {
        sqrt_func = fv_get_sqrt_tab(src->mt_depth);
        sqrt_func(dst->mt_data.dt_ptr, src->mt_data.dt_ptr, src->mt_total);
        FV_ASSERT(sqrt_func != NULL);
        return;
    }
    pow_func = fv_get_pow_tab(src->mt_depth);
    FV_ASSERT(pow_func != NULL);

    pow_func(dst->mt_data.dt_ptr, src->mt_data.dt_ptr, src->mt_total, pow);
}
