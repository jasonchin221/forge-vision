
#include "fv_types.h"
#include "fv_debug.h"
#include "fv_core.h"
#include "fv_math.h"

#define fv_convert_scale_core(dst, src, total, scale, shift, op, cast) \
    do { \
        fv_u32      i; \
        for (i = 0; i < total; i++) { \
            dst[i] = cast(op(src[i], scale, shift)); \
        } \
    } while(0)

#define fv_convert_op_only_shift(data, scale, shift) shift
#define fv_convert_op_only_data(data, scale, shift) data

#define fv_convert_op_no_scale(data, scale, shift) \
    ({ \
        typeof(data)    ret; \
        ret = data + shift; \
        ret; \
     })

#define fv_convert_op_no_shift(data, scale, shift) \
    ({ \
        typeof(data)    ret; \
        ret = data * scale; \
        ret; \
     })

#define fv_convert_op_normal(data, scale, shift) \
    ({ \
        typeof(data)    ret; \
        ret = data * scale + shift; \
        ret; \
     })

#define fv_convert_cast(dst, src, total, scale, shift, cast) \
    do { \
        if (scale == 0) { \
            fv_convert_scale_core(dst, src, total, scale, shift, \
                    fv_convert_op_only_shift, cast); \
        } else if (shift == 0 && scale == 1) { \
            fv_convert_scale_core(dst, src, total, scale, shift, \
                    fv_convert_op_only_data, cast); \
        } else if (scale == 1) { \
            fv_convert_scale_core(dst, src, total, scale, shift, \
                    fv_convert_op_no_scale, cast); \
        } else if (shift == 0) { \
            fv_convert_scale_core(dst, src, total, scale, shift, \
                    fv_convert_op_no_shift, cast); \
        } else { \
            fv_convert_scale_core(dst, src, total, scale, shift, \
                    fv_convert_op_normal, cast); \
        } \
    } while(0) \

/* 8u */
static void
fv_convert_scale_8u_to_8u(fv_u8 *dst, fv_u8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8u_to_8s(fv_s8 *dst, fv_u8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8s);
}

static void
fv_convert_scale_8u_to_16u(fv_u16 *dst, fv_u8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8u_to_16s(fv_s16 *dst, fv_u8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8u_to_32s(fv_s32 *dst, fv_u8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8u_to_32f(float *dst, fv_u8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8u_to_64f(double *dst, fv_u8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

/* 8s */
static void
fv_convert_scale_8s_to_8u(fv_u8 *dst, fv_s8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8u);
}

static void
fv_convert_scale_8s_to_8s(fv_s8 *dst, fv_s8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8s_to_16u(fv_u16 *dst, fv_s8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16u_min);
}

static void
fv_convert_scale_8s_to_16s(fv_s16 *dst, fv_s8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8s_to_32s(fv_s32 *dst, fv_s8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8s_to_32f(float *dst, fv_s8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_8s_to_64f(double *dst, fv_s8 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

/* 16u */
static void
fv_convert_scale_16u_to_8u(fv_u8 *dst, fv_u16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8u);
}

static void
fv_convert_scale_16u_to_8s(fv_s8 *dst, fv_u16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8s);
}

static void
fv_convert_scale_16u_to_16u(fv_u16 *dst, fv_u16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_16u_to_16s(fv_s16 *dst, fv_u16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16s);
}

static void
fv_convert_scale_16u_to_32s(fv_s32 *dst, fv_u16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_16u_to_32f(float *dst, fv_u16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_16u_to_64f(double *dst, fv_u16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

/* 16s */
static void
fv_convert_scale_16s_to_8u(fv_u8 *dst, fv_s16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8u);
}

static void
fv_convert_scale_16s_to_8s(fv_s8 *dst, fv_s16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8s);
}

static void
fv_convert_scale_16s_to_16u(fv_u16 *dst, fv_s16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16u);
}

static void
fv_convert_scale_16s_to_16s(fv_s16 *dst, fv_s16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_16s_to_32s(fv_s32 *dst, fv_s16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_16s_to_32f(float *dst, fv_s16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_16s_to_64f(double *dst, fv_s16 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

/* 32s */
static void
fv_convert_scale_32s_to_8u(fv_u8 *dst, fv_s32 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8u);
}

static void
fv_convert_scale_32s_to_8s(fv_s8 *dst, fv_s32 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8s);
}

static void
fv_convert_scale_32s_to_16u(fv_u16 *dst, fv_s32 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16u);
}

static void
fv_convert_scale_32s_to_16s(fv_s16 *dst, fv_s32 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16s);
}

static void
fv_convert_scale_32s_to_32s(fv_s32 *dst, fv_s32 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_32s_to_32f(float *dst, fv_s32 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_32s_to_64f(double *dst, fv_s32 *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

/* 32f */
static void
fv_convert_scale_32f_to_8u(fv_u8 *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8u);
}

static void
fv_convert_scale_32f_to_8s(fv_s8 *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8s);
}

static void
fv_convert_scale_32f_to_16u(fv_u16 *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16u);
}

static void
fv_convert_scale_32f_to_16s(fv_s16 *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16s);
}

static void
fv_convert_scale_32f_to_32s(fv_s32 *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_32f_to_32f(float *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

static void
fv_convert_scale_32f_to_64f(double *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

/* 64f */
static void
fv_convert_scale_64f_to_8u(fv_u8 *dst, double *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8u);
}

static void
fv_convert_scale_64f_to_8s(fv_s8 *dst, double *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_8s);
}

static void
fv_convert_scale_64f_to_16u(fv_u16 *dst, double *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16u);
}

static void
fv_convert_scale_64f_to_16s(fv_s16 *dst, double *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_16s);
}

static void
fv_convert_scale_64f_to_32s(fv_s32 *dst, double *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_32s);
}

static void
fv_convert_scale_64f_to_32f(float *dst, double *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_cast_32f);
}

static void
fv_convert_scale_64f_to_64f(double *dst, float *src, fv_u32 total, 
            double scale, double shift)
{
    fv_convert_cast(dst, src, total, scale, shift, fv_saturate_no_cast);
}

typedef void (*fv_convert_scale_func)(void *, void *, fv_u32, double, double);

static fv_convert_scale_func fv_convert_scale_tab[][FV_DEPTH_NUM] = {
    {
        (fv_convert_scale_func)fv_convert_scale_8u_to_8u,
        (fv_convert_scale_func)fv_convert_scale_8u_to_8s,
        (fv_convert_scale_func)fv_convert_scale_8u_to_16u,
        (fv_convert_scale_func)fv_convert_scale_8u_to_16s,
        (fv_convert_scale_func)fv_convert_scale_8u_to_32s,
        (fv_convert_scale_func)fv_convert_scale_8u_to_32f,
        (fv_convert_scale_func)fv_convert_scale_8u_to_64f,
    },
    {
        (fv_convert_scale_func)fv_convert_scale_8s_to_8u,
        (fv_convert_scale_func)fv_convert_scale_8s_to_8s,
        (fv_convert_scale_func)fv_convert_scale_8s_to_16u,
        (fv_convert_scale_func)fv_convert_scale_8s_to_16s,
        (fv_convert_scale_func)fv_convert_scale_8s_to_32s,
        (fv_convert_scale_func)fv_convert_scale_8s_to_32f,
        (fv_convert_scale_func)fv_convert_scale_8s_to_64f,
    },
    {
        (fv_convert_scale_func)fv_convert_scale_16u_to_8u,
        (fv_convert_scale_func)fv_convert_scale_16u_to_8s,
        (fv_convert_scale_func)fv_convert_scale_16u_to_16u,
        (fv_convert_scale_func)fv_convert_scale_16u_to_16s,
        (fv_convert_scale_func)fv_convert_scale_16u_to_32s,
        (fv_convert_scale_func)fv_convert_scale_16u_to_32f,
        (fv_convert_scale_func)fv_convert_scale_16u_to_64f,
    },
    {
        (fv_convert_scale_func)fv_convert_scale_16s_to_8u,
        (fv_convert_scale_func)fv_convert_scale_16s_to_8s,
        (fv_convert_scale_func)fv_convert_scale_16s_to_16u,
        (fv_convert_scale_func)fv_convert_scale_16s_to_16s,
        (fv_convert_scale_func)fv_convert_scale_16s_to_32s,
        (fv_convert_scale_func)fv_convert_scale_16s_to_32f,
        (fv_convert_scale_func)fv_convert_scale_16s_to_64f,
    },
    {
        (fv_convert_scale_func)fv_convert_scale_32s_to_8u,
        (fv_convert_scale_func)fv_convert_scale_32s_to_8s,
        (fv_convert_scale_func)fv_convert_scale_32s_to_16u,
        (fv_convert_scale_func)fv_convert_scale_32s_to_16s,
        (fv_convert_scale_func)fv_convert_scale_32s_to_32s,
        (fv_convert_scale_func)fv_convert_scale_32s_to_32f,
        (fv_convert_scale_func)fv_convert_scale_32s_to_64f,
    },
    {
        (fv_convert_scale_func)fv_convert_scale_32f_to_8u,
        (fv_convert_scale_func)fv_convert_scale_32f_to_8s,
        (fv_convert_scale_func)fv_convert_scale_32f_to_16u,
        (fv_convert_scale_func)fv_convert_scale_32f_to_16s,
        (fv_convert_scale_func)fv_convert_scale_32f_to_32s,
        (fv_convert_scale_func)fv_convert_scale_32f_to_32f,
        (fv_convert_scale_func)fv_convert_scale_32f_to_64f,
    },
    {
        (fv_convert_scale_func)fv_convert_scale_64f_to_8u,
        (fv_convert_scale_func)fv_convert_scale_64f_to_8s,
        (fv_convert_scale_func)fv_convert_scale_64f_to_16u,
        (fv_convert_scale_func)fv_convert_scale_64f_to_16s,
        (fv_convert_scale_func)fv_convert_scale_64f_to_32s,
        (fv_convert_scale_func)fv_convert_scale_64f_to_32f,
        (fv_convert_scale_func)fv_convert_scale_64f_to_64f,
    },
};

static fv_convert_scale_func 
fv_get_convert_scale(fv_u32 ddepth, fv_u32 sdepth)
{
    FV_ASSERT(ddepth < FV_DEPTH_NUM && sdepth < FV_DEPTH_NUM);

    return fv_convert_scale_tab[sdepth][ddepth];
}

void
_fv_convert_scale(fv_mat_t *dst, fv_mat_t *src, double scale, double shift)
{
    fv_convert_scale_func   func;

    if (scale == 0 && shift == 0) {
        memset(dst->mt_data.dt_ptr, 0, dst->mt_total_size);
        return;
    }

    FV_ASSERT(src->mt_total == dst->mt_total && src->mt_nchannel == 
            dst->mt_nchannel);

    func = fv_get_convert_scale(dst->mt_depth, src->mt_depth);
    func(dst->mt_data.dt_ptr, src->mt_data.dt_ptr, dst->mt_total, 
            scale, shift);
}

void
fv_convert_scale(fv_image_t *dst, fv_image_t *src, double scale, double shift)
{
    fv_mat_t    _dst;
    fv_mat_t    _src;

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);

    _fv_convert_scale(&_dst, &_src, scale, shift);
}
