
#include "fv_types.h"
#include "fv_smooth.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_filter.h"
#include "fv_mem.h"
#include "fv_math.h"

#define fv_box_row_filter_core(dst, src, width, kx_data, filter) \
    do { \
        fv_sum_row_filter_t     *row_filter = (fv_sum_row_filter_t *)filter; \
        fv_s32                  kx_row = filter->br_ksize;\
        fv_s32                  cn = row_filter->sr_nchannels;\
        fv_s32                  i; \
        fv_s32                  k; \
        fv_s32                  ksz_cn = kx_row*cn; \
        double                  s; \
                        \
        width = (width - 1)*cn; \
        for (k = 0; k < cn; k++, src++, dst++) { \
            s = 0; \
            for (i = 0; i < ksz_cn; i += cn) { \
                s += src[i]; \
            } \
            dst[0] = s; \
            for (i = 0; i < width; i += cn) { \
                s += src[i + ksz_cn] - src[i]; \
                dst[i + cn] = s; \
            } \
        } \
    } while(0) \

static void
fv_box_row_filter_8u(double *dst, fv_u8 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_box_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_box_row_filter_8s(double *dst, fv_s8 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_box_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_box_row_filter_16u(double *dst, fv_u16 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_box_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_box_row_filter_16s(double *dst, fv_s16 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_box_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_box_row_filter_32s(double *dst, fv_s32 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_box_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_box_row_filter_32f(double *dst, float *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_box_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_box_row_filter_64f(double *dst, double *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_box_row_filter_core(dst, src, width, kx_data, filter);
}

static fv_row_filter_func fv_box_row_filter_tab[] = {
    (fv_row_filter_func)fv_box_row_filter_8u,
    (fv_row_filter_func)fv_box_row_filter_8s,
    (fv_row_filter_func)fv_box_row_filter_16u,
    (fv_row_filter_func)fv_box_row_filter_16s,
    (fv_row_filter_func)fv_box_row_filter_32s,
    (fv_row_filter_func)fv_box_row_filter_32f,
    (fv_row_filter_func)fv_box_row_filter_64f,
};

#define fv_box_row_filter_tab_size \
    (sizeof(fv_box_row_filter_tab)/sizeof(fv_row_filter_func))

static fv_row_filter_func 
fv_get_box_row_filter_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_box_row_filter_tab_size);

    return fv_box_row_filter_tab[depth];
}

#define fv_box_column_filter_core(dst, src, count, width, ky_data, \
           filter, cast) \
    do { \
        fv_sum_column_filter_t  *col_filter = (fv_sum_column_filter_t *)filter; \
        double                  *sum = col_filter->sc_sum; \
        double                  *sp; \
        double                  *sm; \
        double                  **src_data; \
        fv_s32                  sum_count = col_filter->sc_sum_count; \
        fv_s32                  cn = col_filter->sc_nchannels; \
        fv_s32                  ksize = filter->bc_ksize; \
        fv_s32                  i; \
        fv_s32                  k; \
        double                  scale = col_filter->sc_scale; \
        double                  s0; \
        fv_bool                 have_scale = col_filter->sc_scale != 1; \
                                \
        width *= cn; \
        if (sum_count == 0) { \
            memset(sum, 0, width*sizeof(*sum)); \
            src_data = src; \
            for (k = 0; k < cn; k++) { \
                src = src_data; \
                for (sum_count = 0; sum_count < ksize - 1; \
                        sum_count++, src++) { \
                    sp = src[0]; \
                    for (i = 0; i < width; i += cn) { \
                        sum[i + k] += sp[i + k]; \
                    } \
                } \
            } \
            col_filter->sc_sum_count = sum_count; \
        } else { \
            FV_ASSERT(sum_count == ksize - 1); \
            src += ksize - 1; \
        } \
            \
        for (; count--; src++) { \
            sp = src[0]; \
            sm = src[1 - ksize]; \
            if (have_scale) { \
                for (k = 0; k < cn; k++) { \
                    for (i = 0; i < width; i += cn) { \
                        s0 = sum[i + k] + sp[i + k]; \
                        dst[i + k] = cast(s0*scale); \
                        sum[i + k] = s0 - sm[i + k]; \
                    } \
                } \
            } else { \
                for (k = 0; k < cn; k++) { \
                    for (i = 0; i < width; i += cn) { \
                        s0 = sum[i + k] + sp[i + k]; \
                        dst[i + k] = cast(s0); \
                        sum[i + k] = s0 - sm[i + k]; \
                    } \
                } \
            } \
                \
            dst += width; \
        } \
    } while(0)

static void
fv_box_column_filter_8u(fv_u8 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_box_column_filter_core(dst, src, count, width, ky_data, 
           filter, fv_saturate_cast_8u);
}

static void
fv_box_column_filter_8s(fv_s8 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_box_column_filter_core(dst, src, count, width, ky_data, 
           filter, fv_saturate_cast_8s);
}

static void
fv_box_column_filter_16u(fv_u16 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_box_column_filter_core(dst, src, count, width, ky_data, 
           filter, fv_saturate_cast_16u);
}

static void
fv_box_column_filter_16s(fv_s16 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_box_column_filter_core(dst, src, count, width, ky_data, 
           filter, fv_saturate_cast_16s);
}

static void
fv_box_column_filter_32s(fv_s32 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_box_column_filter_core(dst, src, count, width, ky_data, 
           filter, fv_saturate_cast_32s);
}

static void
fv_box_column_filter_32f(float *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_box_column_filter_core(dst, src, count, width, ky_data, 
           filter, fv_saturate_cast_32f);
}

static void
fv_box_column_filter_64f(double *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_sum_column_filter_t  *col_filter = (fv_sum_column_filter_t *)filter;
    double                  *sum = col_filter->sc_sum;
    double                  *sp;
    double                  *sm;
    fv_s32                  sum_count = col_filter->sc_sum_count;
    fv_s32                  ksize = filter->bc_ksize;
    fv_s32                  i;
    double                  scale = col_filter->sc_scale;
    double                  s0;
    fv_bool                 have_scale = col_filter->sc_scale != 1;

    if (sum_count == 0) {
        memset(sum, 0, width*sizeof(*sum));
        for (; sum_count < ksize - 1; sum_count++, src++) {
            sp = src[0];
            for (i = 0; i < width; i++) {
                sum[i] += sp[i];
            }
        }
        col_filter->sc_sum_count = sum_count;
    } else {
        FV_ASSERT(sum_count == ksize - 1);
        src += ksize - 1;
    }

    for (; count--; src++) {
        sp = src[0];
        sm = src[1 - ksize];
        if (have_scale) {
            for (i = 0; i < width; i++) {
                s0 = sum[i] + sp[i];
                dst[i] = s0*scale;
                sum[i] = s0 - sm[i];
            }
        } else {
            i = 0;
            for (; i < width; i++) {
                s0 = sum[i] + sp[i];
                dst[i] = s0;
                sum[i] = s0 - sm[i];
            }
        }

        dst += width;
    } 
}

static fv_column_filter_func fv_box_column_filter_tab[] = {
    (fv_column_filter_func)fv_box_column_filter_8u,
    (fv_column_filter_func)fv_box_column_filter_8s,
    (fv_column_filter_func)fv_box_column_filter_16u,
    (fv_column_filter_func)fv_box_column_filter_16s,
    (fv_column_filter_func)fv_box_column_filter_32s,
    (fv_column_filter_func)fv_box_column_filter_32f,
    (fv_column_filter_func)fv_box_column_filter_64f,
};

#define fv_box_column_filter_tab_size \
    (sizeof(fv_box_column_filter_tab)/sizeof(fv_column_filter_func))

static fv_column_filter_func 
fv_get_box_col_filter_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_box_column_filter_tab_size);

    return fv_box_column_filter_tab[depth];
}

static void
fv_create_box_filter(fv_sum_row_filter_t *row_filter, 
        fv_sum_column_filter_t *col_filter, fv_mat_t *src,
        fv_s32 ddepth, fv_s32 sdepth, fv_size_t ksize, 
        fv_point_t anchor, fv_bool normalize)
{
    fv_u32      cn;

    cn = src->mt_nchannel;
    row_filter->sr_filter = fv_get_box_row_filter_tab(sdepth);
    row_filter->sr_ksize = ksize.sz_width;
    row_filter->sr_nchannels = cn;
    col_filter->sc_filter = fv_get_box_col_filter_tab(ddepth);
    col_filter->sc_ksize = ksize.sz_height;
    col_filter->sc_nchannels = cn;
    col_filter->sc_scale =
        normalize ? 1.0/(ksize.sz_width*ksize.sz_height) : 1;
    col_filter->sc_sum = 
        fv_alloc(sizeof(*(col_filter->sc_sum))*src->mt_cols*cn); 
    FV_ASSERT(col_filter->sc_sum != NULL);
    col_filter->sc_sum_count = 0;
}

static void
fv_release_box_filter(fv_sum_row_filter_t *row_filter, 
        fv_sum_column_filter_t *col_filter)
{
    fv_free(&col_filter->sc_sum);
}

void 
fv_box_filter(fv_mat_t *dst, fv_mat_t *src, fv_s32 ddepth, fv_size_t ksize, 
        fv_point_t anchor, fv_bool normalize, fv_s32 border_type)
{
    fv_mat_t                    kernel = {};
    fv_filter_engine_t          filter = {};
    fv_sum_row_filter_t         row_filter = {};
    fv_sum_column_filter_t      col_filter = {};
    fv_u32                      sdepth;

    FV_ASSERT(dst->mt_nchannel == src->mt_nchannel);

    sdepth = src->mt_depth;
    ddepth = dst->mt_depth;
    if (ddepth < 0) {
        ddepth = sdepth;
    }

    if (border_type != FV_BORDER_CONSTANT && normalize) {
        if (src->mt_rows == 1) {
            ksize.sz_height = 1;
        }
        if (src->mt_cols == 1) {
            ksize.sz_width = 1;
        }
    }

    kernel.mt_nchannel = 1;
    kernel.mt_rows = ksize.sz_height;
    kernel.mt_cols = ksize.sz_width;

    if (anchor.pt_x < 0) {
        anchor.pt_x = (ksize.sz_width >> 1);
    }

    if (anchor.pt_y < 0) {
        anchor.pt_y = (ksize.sz_height >> 1);
    }

    fv_create_box_filter(&row_filter, &col_filter, src, 
            ddepth, sdepth, ksize, anchor, normalize);

    filter.fe_row_filter = &row_filter.sr_base;
    filter.fe_col_filter = &col_filter.sc_base;
    filter.fe_is_separable = 1;
    fv_sep_filter_proceed(dst, src, NULL, &kernel, &kernel, 
            anchor, 0, border_type, &filter);

    fv_release_box_filter(&row_filter, &col_filter);
}

void 
fv_gaussian_blur(fv_mat_t *dst, fv_mat_t *src, fv_size_t ksize, 
        double param3, double param4, fv_s32 border_type)
{
}

void 
fv_median_blur(fv_mat_t *dst, fv_mat_t *src, double param1)
{
}

void 
fv_bilateral_filter(fv_mat_t *dst, fv_mat_t *src, double param1,
        double param3, double param4, fv_s32 border_type)
{
}

void
fv_smooth(fv_image_t *dstarr, fv_image_t *srcarr, fv_u32 smooth_type,
          fv_s32 param1, fv_s32 param2, double param3, double param4)
{
    fv_mat_t    dst;
    fv_mat_t    src;

    dst = fv_image_to_mat(dstarr);
    src = fv_image_to_mat(srcarr);
 
    FV_ASSERT(dst.mt_total == src.mt_total &&
        (smooth_type == FV_BLUR_NO_SCALE || 
        (dst.mt_rows == src.mt_rows && dst.mt_cols == src.mt_cols)));

    if (param2 <= 0) {
        param2 = param1;
    }

    if (smooth_type == FV_BLUR || smooth_type == FV_BLUR_NO_SCALE) {
        fv_box_filter(&dst, &src, dst.mt_depth, fv_size(param1, param2),
                fv_point(-1,-1), smooth_type == FV_BLUR,
                FV_BORDER_REPLICATE);
    } else if (smooth_type == FV_GAUSSIAN) {
        fv_gaussian_blur(&dst, &src, fv_size(param1, param2), param3, param4,
                FV_BORDER_REPLICATE);
    } else if (smooth_type == FV_MEDIAN) {
        fv_median_blur(&dst, &src, param1);
    } else {
        fv_bilateral_filter(&dst, &src, param1, param3, param4, 
                FV_BORDER_REPLICATE);
    }
}


