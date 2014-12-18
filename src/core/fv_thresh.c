
#include "fv_types.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_log.h"
#include "fv_imgproc.h"
#include "fv_thresh.h"
#include "fv_time.h"

static double fv_thresh_binary(double src_value, double thresh, 
            double max_value);
static double fv_thresh_binary_inv(double src_value, double thresh, 
            double max_value);
static double fv_thresh_trunc(double src_value, double thresh, 
            double max_value);
static double fv_thresh_tozero(double src_value, double thresh, 
            double max_value);
static double fv_thresh_tozero_inv(double src_value, double thresh, 
            double max_value);

static fv_thresh_proc_t fv_thresh_proc[FV_THRESH_MASK] = {
    {FV_THRESH_BINARY, fv_thresh_binary},
    {FV_THRESH_BINARY_INV, fv_thresh_binary_inv},
    {FV_THRESH_TRUNC, fv_thresh_trunc},
    {FV_THRESH_TOZERO, fv_thresh_tozero},
    {FV_THRESH_TOZERO_INV, fv_thresh_tozero_inv},
};

static double 
fv_thresh_binary(double src_value, double thresh, double max_value)
{
    return src_value > thresh ? max_value : 0;
}

static double 
fv_thresh_binary_inv(double src_value, double thresh, double max_value)
{
    return src_value > thresh ? 0 : max_value;
}

static double 
fv_thresh_trunc(double src_value, double thresh, double max_value)
{
    return src_value > thresh ? thresh : src_value;
}

static double 
fv_thresh_tozero(double src_value, double thresh, double max_value)
{
    return src_value > thresh ? src_value : 0;
}

static double 
fv_thresh_tozero_inv(double src_value, double thresh, double max_value)
{
    return src_value > thresh ? 0 : src_value;
}

static double
fv_get_thresh_val_otsu_8u(fv_mat_t *mat)
{
    fv_u8       *src;
    double      dist[FV_GRAY_LEVEL] = {};
    double      ut[FV_GRAY_LEVEL];
    double      u0[FV_GRAY_LEVEL];
    double      u1[FV_GRAY_LEVEL];
    double      w0 = 0;
    double      w1 = 0;
    double      delta;
    double      max_delta = -1;
    double      thresh = 0;
    double      total_num;
    fv_u32      i;
    fv_u32      j;

    FV_ASSERT(mat->mt_atr == FV_8UC1);

    src = mat->mt_data.dt_ptr;
    total_num = mat->mt_total;
    fv_time_meter_set(FV_TIME_METER2);
    for (j = 0; j < total_num; j++) {
        dist[src[j]]++;
    }
    fv_time_meter_get(FV_TIME_METER2, 0);

    for (i = 0; i < FV_GRAY_LEVEL; i++) {
        dist[i] /= total_num;
        ut[i] = i*dist[i];
    }

    for (i = 0; i < FV_GRAY_LEVEL; i++) {
        w0 += dist[i];
        w1 = 1 - w0;
        if (w0 == 0) {
            u0[i] = 0;
            u1[i] = ut[i];
            continue;
        } else if (w1 == 0) {
            u0[i] = ut[i];
            u1[i] = 0;
            continue;
        } else {
            u0[i] = 0;
            u1[i] = 0;
            for (j = 0; j < FV_GRAY_LEVEL; j++) {
                if (j <= i) {
                    u0[i] += j*dist[j]/w0;
                } else {
                    u1[i] += j*dist[j]/w1;
                }
            }
        }
        delta = w0*w1*(u0[i] - u1[i])*(u0[i] - u1[i]);
        if (delta > max_delta) {
            max_delta = delta;
            thresh = i;
        }
    }

    return thresh;
}

#define fv_threshold_core(dst, src, total_num, thresh, max_value, get_value) \
    do {\
        fv_s32  i; \
        for (i = 0; i < total_num; i++) { \
            dst[i] = get_value(src[i], thresh, max_value); \
        } \
    } while(0)

static void
fv_threshold_8u(fv_u8 *dst, fv_u8 *src, fv_s32 total_num, 
        double thresh, double maxval, fv_u32 type, 
        double (*get_value)(double , double, double))
{
    fv_s32  i;
    fv_u8   tab[256];

    switch(type) {
        case FV_THRESH_BINARY:
            for (i = 0; i <= thresh; i++) {
                tab[i] = 0;
            }
            for (; i < 256; i++) {
                tab[i] = maxval;
            }
            break;
        case FV_THRESH_BINARY_INV:
            for (i = 0; i <= thresh; i++) {
                tab[i] = maxval;
            }
            for (; i < 256; i++) {
                tab[i] = 0;
            }
            break;
        case FV_THRESH_TRUNC:
            for (i = 0; i <= thresh; i++) {
                tab[i] = (fv_u8)i;
            }
            for (; i < 256; i++) {
                tab[i] = thresh;
            }
            break;
        case FV_THRESH_TOZERO:
            for (i = 0; i <= thresh; i++) {
                tab[i] = 0;
            }
            for (; i < 256; i++) {
                tab[i] = (fv_u8)i;
            }
            break;
        case FV_THRESH_TOZERO_INV:
            for (i = 0; i <= thresh; i++) {
                tab[i] = (fv_u8)i;
            }
            for (; i < 256; i++) {
                tab[i] = 0;
            }
            break;
        default:
            FV_LOG_ERR("Unknown threshold type\n");
    }

    for (i = 0; i < total_num; i++) {
        dst[i] = tab[src[i]];
    }
}

static void
fv_threshold_8s(fv_s8 *dst, fv_s8 *src, fv_s32 total_num, 
        double thresh, double max_value, fv_u32 type, 
        double (*get_value)(double , double, double))
{
    fv_threshold_core(dst, src, total_num, thresh, max_value, get_value);
}

static void
fv_threshold_16u(fv_u16 *dst, fv_u16 *src, fv_s32 total_num, 
        double thresh, double max_value, fv_u32 type, 
        double (*get_value)(double , double, double))
{
    fv_threshold_core(dst, src, total_num, thresh, max_value, get_value);
}

static void
fv_threshold_16s(fv_s16 *dst, fv_s16 *src, fv_s32 total_num, 
        double thresh, double max_value, fv_u32 type, 
        double (*get_value)(double , double, double))
{
    fv_threshold_core(dst, src, total_num, thresh, max_value, get_value);
}

static void
fv_threshold_32s(fv_s32 *dst, fv_s32 *src, fv_s32 total_num, 
        double thresh, double max_value, fv_u32 type, 
        double (*get_value)(double , double, double))
{
    fv_threshold_core(dst, src, total_num, thresh, max_value, get_value);
}

static void
fv_threshold_32f(float *dst, float *src, fv_s32 total_num, 
        double thresh, double max_value, fv_u32 type, 
        double (*get_value)(double , double, double))
{
    fv_threshold_core(dst, src, total_num, thresh, max_value, get_value);
}

static void
fv_threshold_64f(double *dst, double *src, fv_s32 total_num, 
        double thresh, double max_value, fv_u32 type, 
        double (*get_value)(double , double, double))
{
    fv_threshold_core(dst, src, total_num, thresh, max_value, get_value);
}

typedef void (*fv_threshold_func)(void *, void *, fv_s32, double, double, 
        fv_u32, double (*get_value)(double , double, double));

static fv_threshold_func fv_threshold_tab[] = {
    (fv_threshold_func)fv_threshold_8u,
    (fv_threshold_func)fv_threshold_8s,
    (fv_threshold_func)fv_threshold_16u,
    (fv_threshold_func)fv_threshold_16s,
    (fv_threshold_func)fv_threshold_32s,
    (fv_threshold_func)fv_threshold_32f,
    (fv_threshold_func)fv_threshold_64f,
};

#define fv_threshold_tab_size (sizeof(fv_threshold_tab)/sizeof(fv_threshold_func))

static fv_threshold_func 
fv_get_threshold_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_threshold_tab_size);

    return fv_threshold_tab[depth];
}

void 
_fv_threshold(fv_mat_t *dst, fv_mat_t *src, double thresh,
        double max_value, fv_u32 type)
{
    fv_threshold_func   func;
    fv_s32              total;
    fv_bool             use_otsu;

    FV_ASSERT(dst->mt_rows == src->mt_rows &&
            dst->mt_cols == src->mt_cols &&
            FV_MAT_NCHANNEL(dst) == FV_MAT_NCHANNEL(src) &&
            (FV_MAT_DEPTH(dst) == FV_MAT_DEPTH(src) || 
             FV_MAT_DEPTH(dst) == FV_DEPTH_8U));

    use_otsu = (type & FV_THRESH_OTSU) != 0;
    type &= FV_THRESH_MASK;
    if (use_otsu) {
        thresh = fv_get_thresh_val_otsu_8u(src);
    }
    
    FV_ASSERT(fv_thresh_proc[type].tp_get_value != NULL);
    func = fv_get_threshold_tab(src->mt_depth);
    FV_ASSERT(func != NULL);

    total = dst->mt_total*FV_MAT_NCHANNEL(dst);
    func(dst->mt_data.dt_ptr, src->mt_data.dt_ptr, total, 
        thresh, max_value, type,
        fv_thresh_proc[type].tp_get_value);
}

void 
fv_threshold(fv_image_t *dst, fv_image_t *src, double thresh,
        double max_value, fv_u32 type)
{
    fv_mat_t    _dst;
    fv_mat_t    _src;

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);
    _fv_threshold(&_dst, &_src, thresh, max_value, type);
}
