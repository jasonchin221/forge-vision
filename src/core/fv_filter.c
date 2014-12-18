#include <string.h>
#include <stdio.h>

#include "fv_types.h"
#include "fv_imgproc.h"
#include "fv_core.h"
#include "fv_math.h"
#include "fv_debug.h"
#include "fv_time.h"
#include "fv_mem.h"
#include "fv_filter.h"
#include "fv_border.h"

#define FV_SEP_FILTER_OUTPUT_LINE_NUM       4

#define fv_preprocess_2D_kernel_core(data, row, col, coords) \
    do { \
        typeof(data)    krow; \
        fv_u32          i; \
        fv_u32          j; \
        fv_u32          k; \
        for (i = k = 0; i < row; i++) { \
            krow = data + i*col; \
            for( j = 0; j < col; j++) { \
                if (krow[j] == 0) { \
                    continue; \
                } \
                coords[k++] = fv_point(j, i); \
            } \
        } \
    } while(0)

static void
fv_preprocess_2D_kernel_8u(fv_u8 *data, fv_s32 row, fv_s32 col,
                fv_point_t *coords)
{
    fv_preprocess_2D_kernel_core(data, row, col, coords);
}

static void
fv_preprocess_2D_kernel_8s(fv_s8 *data, fv_s32 row, fv_s32 col,
                fv_point_t *coords)
{
    fv_preprocess_2D_kernel_core(data, row, col, coords);
}

static void
fv_preprocess_2D_kernel_16u(fv_u16 *data, fv_s32 row, fv_s32 col,
                fv_point_t *coords)
{
    fv_preprocess_2D_kernel_core(data, row, col, coords);
}

static void
fv_preprocess_2D_kernel_16s(fv_s16 *data, fv_s32 row, fv_s32 col,
                fv_point_t *coords)
{
    fv_preprocess_2D_kernel_core(data, row, col, coords);
}

static void
fv_preprocess_2D_kernel_32s(fv_s32 *data, fv_s32 row, fv_s32 col,
                fv_point_t *coords)
{
    fv_preprocess_2D_kernel_core(data, row, col, coords);
}

static void
fv_preprocess_2D_kernel_32f(float *data, fv_s32 row, fv_s32 col,
                fv_point_t *coords)
{
    fv_preprocess_2D_kernel_core(data, row, col, coords);
}

static void
fv_preprocess_2D_kernel_64f(double *data, fv_s32 row, fv_s32 col,
                fv_point_t *coords)
{
    fv_preprocess_2D_kernel_core(data, row, col, coords);
}

typedef void (*fv_preprocess_kernel_func)(void *, fv_s32, 
                fv_s32, fv_point_t *);

static fv_preprocess_kernel_func fv_preprocess_kernel_tab[] = {
    (fv_preprocess_kernel_func)fv_preprocess_2D_kernel_8u,
    (fv_preprocess_kernel_func)fv_preprocess_2D_kernel_8s,
    (fv_preprocess_kernel_func)fv_preprocess_2D_kernel_16u,
    (fv_preprocess_kernel_func)fv_preprocess_2D_kernel_16s,
    (fv_preprocess_kernel_func)fv_preprocess_2D_kernel_32s,
    (fv_preprocess_kernel_func)fv_preprocess_2D_kernel_32f,
    (fv_preprocess_kernel_func)fv_preprocess_2D_kernel_64f,
};

#define fv_preprocess_kernel_tab_size \
    (sizeof(fv_preprocess_kernel_tab)/sizeof(fv_preprocess_kernel_func))

static fv_preprocess_kernel_func 
fv_get_preprocess_kernel_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_preprocess_kernel_tab_size);

    return fv_preprocess_kernel_tab[depth];
}

void 
fv_preprocess_2D_kernel(fv_mat_t *kernel, fv_point_t **coords, fv_u32 nz)
{
    fv_preprocess_kernel_func   func;
    fv_u32                      depth = kernel->mt_depth;

    if (nz == 0) {
        nz = 1;
    }

    func = fv_get_preprocess_kernel_tab(depth);
    FV_ASSERT(func != NULL);
    *coords = fv_alloc(sizeof(*coords)*nz);
    func(kernel->mt_data.dt_ptr, kernel->mt_rows, kernel->mt_cols, *coords);
}
 
/* lightweight convolution with 3x3 kernel */
void 
fv_sep_conv_small3_32f(float *dst, fv_s32 dst_step, 
            float *src, fv_s32 src_step, fv_size_t src_size, 
            float *kx, float *ky, float *buffer)
{
    float       *src2;
    float       *src3;
    fv_s32      dst_width;
    fv_s32      buffer_step = 0;
    fv_s32      x;
    fv_s32      y;
    fv_bool     fast_kx = 1;
    fv_bool     fast_ky = 1;

    FV_ASSERT(src && dst && src_size.sz_width > 2 && 
            src_size.sz_height > 2 &&
            (src_step & 3) == 0 && (dst_step & 3) == 0 &&
            (kx || ky) && (buffer || !kx || !ky));

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    dst_width = src_size.sz_width - 2;

    if (!kx) {
        /* set vars, so that vertical convolution
           will write results into destination ROI and
           horizontal convolution won't run */
        src_size.sz_width = dst_width;
        buffer_step = dst_step;
        buffer = dst;
        dst_width = 0;
    } else {
        fast_kx = kx[1] == 0 && kx[0] == -kx[2] && kx[0] == -1.0;
    }

    FV_ASSERT(src_step >= src_size.sz_width && dst_step >= dst_width);

    src_size.sz_height -= 2;
    if (!ky) {
        /* set vars, so that vertical convolution won't run and
           horizontal convolution will write results into destination ROI */
        src_size.sz_height += 2;
        buffer_step = src_step;
        buffer = src;
        src_size.sz_width = 0;
    } else {
        fast_ky = ky[1] == 0 && ky[0] == -ky[2] && ky[0] == -1.0;
    }

    for (y = 0; y < src_size.sz_height; 
            y++, src += src_step, dst += dst_step, buffer += buffer_step) {
        src2 = src + src_step;
        src3 = src + src_step*2;
        if (fast_ky) {
            for (x = 0; x < src_size.sz_width; x++) {
                buffer[x] = (float)(src3[x] - src[x]);
            }
        } else {
            for(x = 0; x < src_size.sz_width; x++) {
                buffer[x] = (float)(ky[0]*src[x] + ky[1]*src2[x] + ky[2]*src3[x]);
            }
        }

        if (fast_kx) {
            for (x = 0; x < dst_width; x++) {
                dst[x] = (float)(buffer[x+2] - buffer[x]);
            }
        } else {
            for (x = 0; x < dst_width; x++) {
                dst[x] = (float)(kx[0]*buffer[x] + kx[1]*buffer[x+1] + kx[2]*buffer[x+2]);
            }
        }
    }
}

static fv_s32 
fv_get_kernel_type(fv_mat_t *kernel, fv_point_t anchor)
{
    fv_s32      i;
    fv_s32      type;
    fv_s32      sz = kernel->mt_total;
    double      sum = 0;
    double      a;
    double      b;
    double      *coeffs;

    FV_ASSERT(kernel->mt_nchannel == 1);

    type = FV_KERNEL_SMOOTH + FV_KERNEL_INTEGER;
    if ((kernel->mt_rows == 1 || kernel->mt_cols == 1) &&
        anchor.pt_x*2 + 1 == kernel->mt_cols &&
        anchor.pt_y*2 + 1 == kernel->mt_rows ) {
        type |= (FV_KERNEL_SYMMETRICAL + FV_KERNEL_ASYMMETRICAL);
    }

    coeffs = kernel->mt_data.dt_db;
    for (i = 0; i < sz; i++) {
        a = coeffs[i], b = coeffs[sz - i - 1];
        if (a != b) {
            type &= ~FV_KERNEL_SYMMETRICAL;
        }
        if (a != -b) {
            type &= ~FV_KERNEL_ASYMMETRICAL;
        }
        if (a < 0) {
            type &= ~FV_KERNEL_SMOOTH;
        }
        if (a != (fv_s32)(a)) {
            type &= ~FV_KERNEL_INTEGER;
        }
        sum += a;
    }

    if (fabs(sum - 1) > FLT_EPSILON*(fabs(sum) + 1)) {
        type &= ~FV_KERNEL_SMOOTH;
    }

    return type;
}

#define fv_row_filter_core(dst, src, width, kx_data, filter) \
    do {\
        fv_s32      kx_row = filter->br_ksize; \
        fv_s32      i; \
        fv_s32      k; \
                        \
        for (i = 0; i < width; i++) { \
            dst[i] = src[i]*kx_data[0]; \
            for (k = 1; k < kx_row; k++) { \
                dst[i] += src[i + k]*kx_data[k]; \
            } \
        } \
    } while(0)

static void
fv_row_filter_8u(double *dst, fv_u8 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_row_filter_8s(double *dst, fv_s8 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_row_filter_16u(double *dst, fv_u16 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_row_filter_16s(double *dst, fv_s16 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_row_filter_32s(double *dst, fv_s32 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_row_filter_core(dst, src, width, kx_data, filter);
}


static void
fv_row_filter_32f(double *dst, float *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_row_filter_core(dst, src, width, kx_data, filter);
}

static void
fv_row_filter_64f(double *dst, double *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_row_filter_core(dst, src, width, kx_data, filter);
}

static fv_row_filter_func fv_row_filter_tab[] = {
    (fv_row_filter_func)fv_row_filter_8u,
    (fv_row_filter_func)fv_row_filter_8s,
    (fv_row_filter_func)fv_row_filter_16u,
    (fv_row_filter_func)fv_row_filter_16s,
    (fv_row_filter_func)fv_row_filter_32s,
    (fv_row_filter_func)fv_row_filter_32f,
    (fv_row_filter_func)fv_row_filter_64f,
};

#define fv_row_filter_tab_size \
    (sizeof(fv_row_filter_tab)/sizeof(fv_row_filter_func))

static fv_row_filter_func 
fv_get_row_filter_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_row_filter_tab_size);

    return fv_row_filter_tab[depth];
}

#define fv_column_filter_core(dst, src, count, width, ky_data, \
        filter, cast) \
    do { \
        fv_linear_column_filter_t   *col_filter = \
            (fv_linear_column_filter_t *)filter;\
        double          v; \
        fv_s32          ky_size = filter->bc_ksize; \
        fv_s32          ay = filter->bc_anchor; \
        fv_s32          i; \
        fv_s32          j; \
        fv_s32          k; \
                        \
        ky_data += ky_size; \
        if (col_filter->lc_type & FV_KERNEL_SYMMETRICAL) { \
            for (j = 0; j < count; j++, dst += width) { \
                for (i = 0; i < width; i++) { \
                    v = ky_data[0]*src[j + ay][i]; \
                    for (k = 1; k <= ky_size; k++) { \
                        v += ky_data[k]*(src[j + ay + k][i] + \
                                src[j + ay - k][i]); \
                    } \
                    dst[i] = cast(v); \
                } \
            } \
        } else { \
            for (j = 0; j < count; j++, dst += width) { \
                for (i = 0; i < width; i++) { \
                    v = 0; \
                    for (k = 1; k <= ky_size; k++) { \
                        v += ky_data[k]*(src[j + ay - k][i] - \
                                src[j + ay + k][i]); \
                    } \
                    dst[i] = cast(v); \
                } \
            } \
        } \
    } while(0)

static void
fv_column_filter_8u(fv_u8 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_column_filter_core(dst, src, count, width, ky_data,
        filter, fv_saturate_cast_8u);
}

static void
fv_column_filter_8s(fv_s8 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_column_filter_core(dst, src, count, width, ky_data,
        filter, fv_saturate_cast_8s);
}

static void
fv_column_filter_16u(fv_u16 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_column_filter_core(dst, src, count, width, ky_data,
        filter, fv_saturate_cast_16u);
}

static void
fv_column_filter_16s(fv_s16 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_column_filter_core(dst, src, count, width, ky_data,
        filter, fv_saturate_cast_16s);
}

static void
fv_column_filter_32s(fv_s32 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_column_filter_core(dst, src, count, width, ky_data,
        filter, fv_saturate_cast_32s);
}

static void
fv_column_filter_32f(float *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_column_filter_core(dst, src, count, width, ky_data,
        filter, fv_saturate_cast_32f);
}

static void
fv_column_filter_64f(double *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_s32      ky_size = filter->bc_ksize;
    fv_s32      ay = filter->bc_anchor;
    fv_s32      i;
    fv_s32      j;
    fv_s32      k;
    
    ky_data += ky_size;
    if (filter->bc_type & FV_KERNEL_SYMMETRICAL) {
        for (j = 0; j < count; j++, dst += width) {
            for (i = 0; i < width; i++) {
                dst[i] = ky_data[0]*src[j + ay][i];
                for (k = 1; k <= ky_size; k++) {
                    dst[i] += ky_data[k]*(src[j + ay + k][i] + 
                            src[j + ay - k][i]);
                }
            }
        }
    } else {
        for (j = 0; j < count; j++, dst += width) {
            for (i = 0; i < width; i++) {
                dst[i] = 0;
                for (k = 1; k <= ky_size; k++) {
                    dst[i] += ky_data[k]*(src[j + ay - k][i] - 
                            src[j + ay + k][i]);
                }
            }
        }
    }
}

static fv_column_filter_func fv_column_filter_tab[] = {
    (fv_column_filter_func)fv_column_filter_8u,
    (fv_column_filter_func)fv_column_filter_8s,
    (fv_column_filter_func)fv_column_filter_16u,
    (fv_column_filter_func)fv_column_filter_16s,
    (fv_column_filter_func)fv_column_filter_32s,
    (fv_column_filter_func)fv_column_filter_32f,
    (fv_column_filter_func)fv_column_filter_64f,
};

#define fv_column_filter_tab_size \
    (sizeof(fv_column_filter_tab)/sizeof(fv_column_filter_func))

static fv_column_filter_func 
fv_get_column_filter_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_column_filter_tab_size);

    return fv_column_filter_tab[depth];
}

void 
fv_sep_filter_proceed(fv_mat_t *dst, fv_mat_t *src, fv_mat_t *kernel_x, 
                fv_mat_t *kernel_y, fv_point_t anchor, double delta, 
                fv_s32 border_type, fv_filter_engine_t *filter)
{
    fv_base_filter_t            *filter_2D;
    fv_base_row_filter_t        *row_filter;
    fv_base_column_filter_t     *col_filter;
    fv_border_make_row_func     make_border; 
    fv_u8                       *dst_data;
    fv_u8                       *src_data;
    double                      **buf;
    void                        *tmp;
    void                        *src_buf;
    void                        *row;
    fv_u32                      src_buf_len;
    fv_u32                      cn;
    fv_s32                      kx_row;
    fv_s32                      ky_row;
    fv_s32                      buf_row;
    fv_s32                      buf_col;
    fv_s32                      src_step;
    fv_s32                      dst_step;
    fv_s32                      col;
    fv_s32                      row_num;
    fv_s32                      width;
    fv_s32                      height;
    fv_s32                      ax;
    fv_s32                      ay;
    fv_s32                      s;
    fv_s32                      i;
    fv_s32                      j;
    fv_s32                      k;
    fv_s32                      h;
    fv_bool                     first;
    fv_bool                     is_separable; 

    filter_2D = filter->fe_filter_2D;
    row_filter = filter->fe_row_filter;
    col_filter = filter->fe_col_filter;

    FV_ASSERT((row_filter != NULL && col_filter != NULL) ||
            filter_2D != NULL); 

    kx_row = kernel_x->mt_rows;
    ky_row = kernel_y->mt_rows;
    
    ax = anchor.pt_x;
    ay = anchor.pt_y;

    dst_data = dst->mt_data.dt_ptr;
    src_data = src->mt_data.dt_ptr;

    width = dst->mt_cols;
    height = dst->mt_rows;
    row_num = FV_SEP_FILTER_OUTPUT_LINE_NUM;
    buf_row = row_num + 2*ky_row - 2 - ay;
    cn = src->mt_nchannel;
    buf_col = (width + kx_row)*cn;

    dst_step = dst->mt_step;
    src_step = src->mt_step;
    src_buf_len = src_step + (kx_row - 1)*src_step/width;
    src_buf = fv_calloc(src_buf_len);
    FV_ASSERT(src_buf != NULL);
    buf = fv_alloc(buf_row*sizeof(*buf));
    FV_ASSERT(buf != NULL);

    s = buf_col*sizeof(**buf);
    for (i = 0; i < buf_row; i++) {
        buf[i] = fv_alloc(s);
        FV_ASSERT(buf[i] != NULL);
    }

    is_separable = filter->fe_is_separable;
    make_border = fv_border_get_func(src->mt_depth);
    FV_ASSERT(make_border != NULL);
    for (h = 0, j = 0; j < ky_row - 1 - ay; j++, src_data += src_step, h++) {
        row = is_separable ? src_buf:buf[ay + j];
        make_border(row, src_data, src_buf_len, src_step, width, 
                cn, kx_row, ax, border_type);
        if (is_separable) {
            row_filter->br_filter(buf[ay + j], row, width, 
                    kernel_x->mt_data.dt_fl, row_filter);
        }
    }

    first = 1;
    for (; h < height; dst_data += dst_step*row_num) {
        for (j = 0; j < row_num && h < height; 
                j++, src_data += src_step, h++) {
            row = is_separable ? src_buf:buf[ky_row - 1 + j];
            make_border(row, src_data, src_buf_len, src_step, width,
                    cn, kx_row, ax, border_type);
            if (is_separable) {
                row_filter->br_filter(buf[ky_row - 1 + j], row, width,
                        kernel_x->mt_data.dt_fl, row_filter);
            }
        }
        if (first) {
            for (k = 0; k < ay; k++) {
                if (border_type == FV_BORDER_CONSTANT) {
                    memset(buf[k], 0, s);
                } else {
                    col = fv_border_get_value(border_type, k - ay, height);
                    memcpy(buf[k], buf[ay + col], s);
                }
            }
            first = 0;
        }
        if (h == height) {
            for (k = 0; k < ky_row - 1 - ay; k++) {
                if (border_type == FV_BORDER_CONSTANT) {
                    memset(buf[ky_row - 1 + j + k], 0, s);
                } else {
                    col = fv_border_get_value(border_type, 
                            height - 1 + k, height);
                    memcpy(buf[ky_row - 1 + j + k], 
                            buf[ky_row - 1 + j + col - height], s);
                }
            }
            j += ky_row - 1 - ay;
        }

        if (is_separable) {
            col_filter->bc_filter(dst_data, buf, j, width, 
                    kernel_y->mt_data.dt_fl, col_filter);
        } else {
            filter_2D->bf_filter(dst_data, buf, j, width, 
                    kernel_y->mt_data.dt_fl, cn, filter_2D);
        }
        for (k = 0; k < ky_row - 1; k++) {
            tmp = buf[row_num + k];
            buf[row_num + k] = buf[k];
            buf[k] = tmp;
        }
    }

    for (i = 0; i < buf_row; i++) {
        fv_free(&buf[i]);
    }

    fv_free(&buf);
    fv_free(&src_buf);
}

void 
fv_sep_filter2D(fv_mat_t *dst, fv_mat_t *src,
                fv_mat_t *kernel_x, fv_mat_t *kernel_y, 
                fv_point_t anchor, double delta, fv_s32 border_type)
{
    fv_mat_t                    *kernel;
    fv_filter_engine_t          filter = {};
    fv_linear_row_filter_t      row_filter = {};
    fv_linear_column_filter_t   col_filter = {};
    fv_s32                      ay;

    if (anchor.pt_x < 0) {
        anchor.pt_x = ((kernel_x->mt_rows - 1) >> 1);
    }

    if (anchor.pt_y < 0) {
        anchor.pt_y = ((kernel_y->mt_rows - 1) >> 1);
    }

    ay = anchor.pt_y;
    row_filter.lr_filter = fv_get_row_filter_tab(src->mt_depth);
    row_filter.lr_anchor = anchor.pt_x;
    row_filter.lr_ksize = kernel_x->mt_rows;
    col_filter.lc_filter = fv_get_column_filter_tab(dst->mt_depth);
    col_filter.lc_ksize = (kernel_y->mt_rows >> 1);
    col_filter.lc_anchor = ay;

    kernel = fv_create_mat(kernel_y->mt_rows, kernel_y->mt_cols, 
            FV_MAKETYPE(FV_DEPTH_64F, 1));
    FV_ASSERT(kernel != NULL);
    fv_convert_mat(kernel, kernel_y);
    col_filter.lc_type = 
        fv_get_kernel_type(kernel, kernel_y->mt_rows == 1 ? 
            fv_point(ay, 0):fv_point(0, ay));
    fv_release_mat(&kernel);

    filter.fe_row_filter = &row_filter.lr_base;
    filter.fe_col_filter = &col_filter.lc_base;
    filter.fe_is_separable = 1;
    fv_sep_filter_proceed(dst, src, kernel_x, kernel_y, anchor, delta, 
            border_type, &filter);
}

