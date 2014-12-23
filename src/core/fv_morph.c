
#include "fv_types.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_imgproc.h"
#include "fv_morph.h"
#include "fv_filter.h"
#include "fv_stat.h"
#include "fv_math.h"
#include "fv_time.h"
#include "fv_mem.h"

static double
fv_morph_op_erode(double v1, double v2)
{
    return fv_min(v1, v2);
}

static double
fv_morph_op_dilate(double v1, double v2)
{
    return fv_max(v1, v2);
}

static fv_morph_op_func fv_morph_op_proc[FV_MOP_MAX] = {
    fv_morph_op_erode,
    fv_morph_op_dilate,
};

static fv_mat_t *
fv_convert_conv_kernel(fv_conv_kernel_t *kernel, fv_point_t *anchor)
{
    fv_mat_t    *mat;
    fv_s32      size;
    fv_s32      i;

    if (kernel == NULL) {
        *anchor = fv_point(1, 1);
        return NULL;
    }

    *anchor = fv_point(kernel->ck_anchor_x, kernel->ck_anchor_y);
    mat = fv_create_mat(kernel->ck_nrows, kernel->ck_ncols, FV_8UC1);
    FV_ASSERT(mat != NULL);

    size = kernel->ck_nrows * kernel->ck_ncols;
    for (i = 0; i < size; i++) {
        mat->mt_data.dt_ptr[i] = (fv_u8)(kernel->ck_values[i] != 0);
    }

    return mat;
}

fv_mat_t *
fv_get_structuring_element(fv_s32 shape, fv_size_t ksize, fv_point_t anchor)
{
    fv_mat_t    *elem = NULL;
    fv_s32      *ptr;
    fv_size_t   size;
    fv_s32      i = 0;
    fv_s32      j = 0;
    fv_s32      j1;
    fv_s32      j2 = 0;
    fv_s32      r = 0;
    fv_s32      c = 0;
    fv_s32      dx = 0;
    fv_s32      dy = 0;
    double      inv_r2 = 0;

    FV_ASSERT(shape == FV_SHAPE_RECT || shape == FV_SHAPE_CROSS ||
            shape == FV_SHAPE_ELLIPSE);

    anchor = fv_normalize_anchor(anchor, ksize);

    size = fv_size(1, 1);
    if (memcmp(&ksize, &size, sizeof(ksize)) == 0) {
        shape = FV_SHAPE_RECT;
    }

    if (shape == FV_SHAPE_ELLIPSE) {
        r = (ksize.sz_height >> 1);
        c = (ksize.sz_width >> 1);
        inv_r2 = r ? 1.0/((double)r*r) : 0;
    }

    elem = fv_create_mat(ksize.sz_height, ksize.sz_width, 
            FV_MAKETYPE(FV_DEPTH_32S, 1)); 
    for (i = 0; i < ksize.sz_height; i++) {
        ptr = (fv_s32 *)(elem->mt_data.dt_ptr + i*elem->mt_step);
        j1 = 0;
        if (shape == FV_SHAPE_RECT || 
                (shape == FV_SHAPE_CROSS && i == anchor.pt_y)) {
            j2 = ksize.sz_width;
        } else if (shape == FV_SHAPE_CROSS) {
            j1 = anchor.pt_x; 
            j2 = j1 + 1;
        } else {
            dy = i - r;
            if (abs(dy) <= r) {
                dx = sqrt((r*r - dy*dy)*inv_r2);
                j1 = fv_max(c - dx, 0);
                j2 = fv_min(c + dx + 1, ksize.sz_width);
            }
        }

        for (j = 0; j < j1; j++) {
            ptr[j] = 0;
        }

        for (; j < j2; j++) {
            ptr[j] = 1;
        }

        for (; j < ksize.sz_width; j++) {
            ptr[j] = 0;
        }
    }

    return elem;
}

fv_conv_kernel_t *
fv_create_structuring_element_ex(fv_s32 cols, fv_s32 rows, fv_s32 anchor_x,
                    fv_s32 anchor_y, fv_s32 shape, fv_s32 *values)
{
    fv_conv_kernel_t    *element;
    fv_mat_t            *elem;
    fv_size_t           ksize = fv_size(cols, rows);
    fv_point_t          anchor = fv_point(anchor_x, anchor_y);
    fv_s32              i;
    fv_s32              size;
    fv_s32              element_size;

    FV_ASSERT(cols > 0 && rows > 0 && 
            (shape != FV_SHAPE_CUSTOM || values != 0));

    size = rows*cols;
    element_size = sizeof(fv_conv_kernel_t) + size*sizeof(fv_s32);
    element = fv_alloc(element_size + 32);

    element->ck_ncols = cols;
    element->ck_nrows = rows;
    element->ck_anchor_x = anchor_x;
    element->ck_anchor_y = anchor_y;
    element->ck_nshift_r = shape < FV_SHAPE_ELLIPSE ? shape : FV_SHAPE_CUSTOM;
    element->ck_values = (fv_s32 *)(element + 1);

    if (shape == FV_SHAPE_CUSTOM) {
        for (i = 0; i < size; i++) {
            element->ck_values[i] = values[i];
        }
    } else {
        elem = fv_get_structuring_element(shape, ksize, anchor);
        for (i = 0; i < size; i++) {
            element->ck_values[i] = elem->mt_data.dt_i[i];
        }
        fv_release_mat(&elem);
    }

    return element;
}

void
fv_release_structuring_element(fv_conv_kernel_t **element)
{
    fv_free(element);
}

#define fv_morph_row_filter_core(dst, src, width, filter) \
    do { \
        fv_morphology_row_filter_t  *row_filter = \
        (fv_morphology_row_filter_t *)filter; \
        fv_morph_op_func            op = row_filter->mr_op; \
        typeof(src)                 s; \
        typeof(*src)                m; \
        fv_s32                      i; \
        fv_s32                      j; \
        fv_s32                      k; \
        fv_s32                      cn = row_filter->mr_nchannels; \
        fv_s32                      ksize = filter->br_ksize*cn; \
                                    \
        if (ksize == cn) { \
            for (i = 0; i < width*cn; i++) {\
                dst[i] = src[i]; \
            } \
            return; \
        } \
        \
        width *= cn; \
        \
        for (k = 0; k < cn; k++, src++, dst++) { \
            for (i = 0; i <= width - cn*2; i += cn*2) { \
                s = src + i; \
                m = s[cn]; \
                for (j = cn*2; j < ksize; j += cn) { \
                    m = op(m, s[j]); \
                } \
                dst[i] = op(m, s[0]); \
                dst[i + cn] = op(m, s[j]); \
            } \
            \
            for (; i < width; i += cn) {\
                s = src + i; \
                m = s[0]; \
                for (j = cn; j < ksize; j += cn) { \
                    m = op(m, s[j]); \
                }\
                dst[i] = m; \
            } \
        } \
    } while(0)

static void
fv_morph_row_filter_8u(double *dst, fv_u8 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_morph_row_filter_core(dst, src, width, filter);
}

static void
fv_morph_row_filter_8s(double *dst, fv_s8 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_morph_row_filter_core(dst, src, width, filter);
}

static void
fv_morph_row_filter_16u(double *dst, fv_u16 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_morph_row_filter_core(dst, src, width, filter);
}

static void
fv_morph_row_filter_16s(double *dst, fv_s16 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_morph_row_filter_core(dst, src, width, filter);
}

static void
fv_morph_row_filter_32s(double *dst, fv_s32 *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_morph_row_filter_core(dst, src, width, filter);
}

static void
fv_morph_row_filter_32f(double *dst, float *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_morph_row_filter_core(dst, src, width, filter);
}

static void
fv_morph_row_filter_64f(double *dst, double *src, fv_s32 width, 
            float *kx_data, fv_base_row_filter_t *filter)
{
    fv_morph_row_filter_core(dst, src, width, filter);
}

static fv_row_filter_func fv_morph_row_filter_tab[] = {
    (fv_row_filter_func)fv_morph_row_filter_8u,
    (fv_row_filter_func)fv_morph_row_filter_8s,
    (fv_row_filter_func)fv_morph_row_filter_16u,
    (fv_row_filter_func)fv_morph_row_filter_16s,
    (fv_row_filter_func)fv_morph_row_filter_32s,
    (fv_row_filter_func)fv_morph_row_filter_32f,
    (fv_row_filter_func)fv_morph_row_filter_64f,
};

#define fv_morph_row_filter_tab_size \
    (sizeof(fv_morph_row_filter_tab)/sizeof(fv_row_filter_func))

static fv_row_filter_func 
fv_get_morph_row_filter_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_morph_row_filter_tab_size);

    return fv_morph_row_filter_tab[depth];
}


#define fv_morph_column_filter_core(dst, src, count, width, ky_data, \
           filter) \
    do { \
        fv_morphology_column_filter_t   *col_filter = \
            (fv_morphology_column_filter_t *)filter; \
        fv_morph_op_func                op = col_filter->mc_op; \
        typeof(**src)                   s0; \
        fv_s32                          i; \
        fv_s32                          j; \
        fv_s32                          k; \
        fv_s32                          cn = col_filter->mc_nchannels; \
        fv_s32                          ksize = filter->bc_ksize; \
        fv_s32                          dststep = width*cn; \
                                        \
        width *= cn; \
        for (; ksize > 1 && count > 1; \
                count -= 2, dst += dststep*2, src += 2) { \
            for (j = 0 ; j < cn; j++) { \
                for (i = 0 ; i < width; i += cn) { \
                    s0 = src[1][i + j]; \
                    for (k = 2; k < ksize; k++) { \
                        s0 = op(s0, src[k][i + j]); \
                    } \
                    \
                    dst[i + j] = op(s0, src[0][i + j]); \
                    dst[i + j +dststep] = op(s0, src[k][i + j]); \
                } \
            } \
        } \
        \
        for (; count > 0; count--, dst += dststep, src++) { \
            for (j = 0 ; j < cn; j++) { \
                for (i = 0 ; i < width; i += cn) { \
                    s0 = src[0][i + j]; \
                    for (k = 1; k < ksize; k++) {\
                        s0 = op(s0, src[k][i + j]); \
                    } \
                    dst[i + j] = s0; \
                } \
            } \
        } \
    } while(0)
 
static void
fv_morph_column_filter_8u(fv_u8 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_morph_column_filter_core(dst, src, count, width, ky_data, 
           filter);
}

static void
fv_morph_column_filter_8s(fv_s8 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_morph_column_filter_core(dst, src, count, width, ky_data, 
           filter);
}

static void
fv_morph_column_filter_16u(fv_u16 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_morph_column_filter_core(dst, src, count, width, ky_data, 
           filter);
}

static void
fv_morph_column_filter_16s(fv_s16 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_morph_column_filter_core(dst, src, count, width, ky_data, 
           filter);
}

static void
fv_morph_column_filter_32s(fv_s32 *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_morph_column_filter_core(dst, src, count, width, ky_data, 
           filter);
}

static void
fv_morph_column_filter_32f(float *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_morph_column_filter_core(dst, src, count, width, ky_data, 
           filter);
}

static void
fv_morph_column_filter_64f(double *dst, double **src, 
           fv_s32 count, fv_s32 width, float *ky_data,
           fv_base_column_filter_t *filter)
{
    fv_morph_column_filter_core(dst, src, count, width, ky_data, 
           filter);
}

static fv_column_filter_func fv_morph_column_filter_tab[] = {
    (fv_column_filter_func)fv_morph_column_filter_8u,
    (fv_column_filter_func)fv_morph_column_filter_8s,
    (fv_column_filter_func)fv_morph_column_filter_16u,
    (fv_column_filter_func)fv_morph_column_filter_16s,
    (fv_column_filter_func)fv_morph_column_filter_32s,
    (fv_column_filter_func)fv_morph_column_filter_32f,
    (fv_column_filter_func)fv_morph_column_filter_64f,
};

#define fv_morph_column_filter_tab_size \
    (sizeof(fv_morph_column_filter_tab)/sizeof(fv_column_filter_func))

static fv_column_filter_func 
fv_get_morph_col_filter_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_morph_column_filter_tab_size);

    return fv_morph_column_filter_tab[depth];
}

static void
fv_create_morph_filter(fv_u32 op, fv_morphology_row_filter_t *row_filter, 
        fv_morphology_column_filter_t *col_filter, fv_mat_t *src,
        fv_s32 ddepth, fv_s32 sdepth, fv_size_t ksize, 
        fv_point_t anchor)
{
    fv_morph_op_func    func;
    fv_u32              cn;

    func = fv_morph_op_proc[op];
    FV_ASSERT(op < FV_MOP_MAX && func != NULL);
    cn = src->mt_nchannel;
    row_filter->mr_filter = fv_get_morph_row_filter_tab(sdepth);
    row_filter->mr_ksize = ksize.sz_width;
    row_filter->mr_nchannels = cn;
    row_filter->mr_op = func;
    col_filter->mc_filter = fv_get_morph_col_filter_tab(ddepth);
    col_filter->mc_ksize = ksize.sz_height;
    col_filter->mc_nchannels = cn;
    col_filter->mc_op = func;
}

#define fv_morph_filter_2D_core(dst, src, count, width, cn, filter) \
    do { \
        fv_morphology_filter_2D_t   *f = \
            (fv_morphology_filter_2D_t *)filter; \
        fv_morph_op_func                op = f->mf_op; \
        typeof(src)                     kp; \
        typeof(**src)                   s0; \
        fv_point_t                      *pt; \
        fv_u32                          nz; \
        fv_u32                          i; \
        fv_u32                          k; \
                                        \
        nz = f->mf_nz; \
        pt = f->mf_coords; \
        kp = f->mf_ptrs; \
        \
        width *= cn; \
        for (; count > 0; count--, dst += width, src++) { \
            for (k = 0; k < nz; k++) { \
                kp[k] = src[pt[k].pt_y] + pt[k].pt_x*cn; \
            } \
                \
            for (i = 0; i < width; i++) { \
                s0 = kp[0][i]; \
                for( k = 1; k < nz; k++ ) \
                    s0 = op(s0, kp[k][i]); \
                dst[i] = s0; \
            } \
        } \
    } while(0)
 
static void
fv_morph_filter_2D_8u(fv_u8 *dst, fv_u8 **src, 
           fv_s32 count, fv_s32 width, float *k_data,
           fv_u32 cn, fv_base_filter_t *filter)
{
    fv_morph_filter_2D_core(dst, src, count, width, cn, filter);
}

static void
fv_morph_filter_2D_8s(fv_s8 *dst, fv_s8 **src, 
           fv_s32 count, fv_s32 width, float *k_data,
           fv_u32 cn, fv_base_filter_t *filter)
{
    fv_morph_filter_2D_core(dst, src, count, width, cn, filter);
}

static void
fv_morph_filter_2D_16u(fv_u16 *dst, fv_u16 **src, 
           fv_s32 count, fv_s32 width, float *k_data,
           fv_u32 cn, fv_base_filter_t *filter)
{
    fv_morph_filter_2D_core(dst, src, count, width, cn, filter);
}

static void
fv_morph_filter_2D_16s(fv_s16 *dst, fv_s16 **src, 
           fv_s32 count, fv_s32 width, float *k_data,
           fv_u32 cn, fv_base_filter_t *filter)
{
    fv_morph_filter_2D_core(dst, src, count, width, cn, filter);
}

static void
fv_morph_filter_2D_32s(fv_s32 *dst, fv_s32 **src, 
           fv_s32 count, fv_s32 width, float *k_data,
           fv_u32 cn, fv_base_filter_t *filter)
{
    fv_morph_filter_2D_core(dst, src, count, width, cn, filter);
}

static void
fv_morph_filter_2D_32f(float *dst, float **src, 
           fv_s32 count, fv_s32 width, float *k_data,
           fv_u32 cn, fv_base_filter_t *filter)
{
    fv_morph_filter_2D_core(dst, src, count, width, cn, filter);
}

static void
fv_morph_filter_2D_64f(double *dst, double **src, 
           fv_s32 count, fv_s32 width, float *k_data,
           fv_u32 cn, fv_base_filter_t *filter)
{
    fv_morph_filter_2D_core(dst, src, count, width, cn, filter);
}

static fv_filter_2D_func fv_morph_filter_2D_tab[] = {
    (fv_filter_2D_func)fv_morph_filter_2D_8u,
    (fv_filter_2D_func)fv_morph_filter_2D_8s,
    (fv_filter_2D_func)fv_morph_filter_2D_16u,
    (fv_filter_2D_func)fv_morph_filter_2D_16s,
    (fv_filter_2D_func)fv_morph_filter_2D_32s,
    (fv_filter_2D_func)fv_morph_filter_2D_32f,
    (fv_filter_2D_func)fv_morph_filter_2D_64f,
};

#define fv_morph_filter_2D_tab_size \
    (sizeof(fv_morph_filter_2D_tab)/sizeof(fv_filter_2D_func))

static fv_filter_2D_func 
fv_get_morph_filter_2D_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_morph_filter_2D_tab_size);

    return fv_morph_filter_2D_tab[depth];
}

static void
fv_create_morph_filter_2D(fv_u32 op, fv_morphology_filter_2D_t *filter,
        fv_mat_t *src, fv_s32 depth, fv_u32 nz, fv_size_t ksize,
        fv_mat_t *kernel, fv_point_t anchor)
{
    fv_morph_op_func    func;
    fv_u32              cn;

    func = fv_morph_op_proc[op];
    FV_ASSERT(op < FV_MOP_MAX && func != NULL);

    cn = src->mt_nchannel;
    filter->mf_filter = fv_get_morph_filter_2D_tab(depth);
    filter->mf_ksize = ksize;
    filter->mf_anchor = anchor;
    filter->mf_nchannels = cn;
    filter->mf_op = func;
    fv_preprocess_2D_kernel(kernel, &filter->mf_coords, &filter->mf_coeffs, nz);
    filter->mf_nz = nz;
    filter->mf_ptrs = fv_alloc(nz*sizeof(void *));
    FV_ASSERT(filter->mf_ptrs != NULL);
}

static void
fv_release_morph_filter_2D(fv_filter_engine_t *filter)
{
    fv_morphology_filter_2D_t   *f = 
        (fv_morphology_filter_2D_t *)filter->fe_filter_2D;

    if (!filter->fe_is_separable) {
        fv_free(&f->mf_coords);
        fv_free(&f->mf_coeffs);
        fv_free(&f->mf_ptrs);
    }
}

static void 
fv_morph_op_iterate(fv_s32 op, fv_mat_t *dst, fv_mat_t *src, fv_mat_t *kernel,
        fv_point_t anchor, fv_s32 iterations, fv_u32 border_type)
{
    fv_filter_engine_t              filter = {};
    fv_morphology_filter_2D_t       filter_2D = {};
    fv_morphology_row_filter_t      row_filter = {};
    fv_morphology_column_filter_t   col_filter = {};
    fv_size_t                       ksize;
    fv_u32                          i;
    fv_u32                          nz;

    ksize = kernel != NULL && kernel->mt_data.dt_ptr != NULL? 
        fv_size(kernel->mt_cols, kernel->mt_rows) : fv_size(3, 3);
    nz = _fv_count_non_zero(kernel);
    if (nz == kernel->mt_rows*kernel->mt_cols) {
        fv_create_morph_filter(op, &row_filter, &col_filter, src, dst->mt_depth, 
                src->mt_depth, ksize, anchor);
        filter.fe_row_filter = &row_filter.mr_base;
        filter.fe_col_filter = &col_filter.mc_base;
        filter.fe_is_separable = 1;
    } else {
        fv_create_morph_filter_2D(op, &filter_2D, src, dst->mt_depth, 
                nz, ksize, kernel, anchor);
        filter.fe_filter_2D = &filter_2D.mf_base;
        filter.fe_is_separable = 0;
    }

    fv_sep_filter_proceed(dst, src, kernel, kernel, kernel, 
            anchor, 0, border_type, &filter);

    for (i = 1; i < iterations; i++) {
        fv_sep_filter_proceed(dst, dst, kernel, kernel, kernel,
                anchor, 0, border_type, &filter);
    }

    fv_release_morph_filter_2D(&filter);
}

static void
fv_morph_op(fv_u32 op, fv_mat_t *dst, fv_mat_t *src, 
        fv_mat_t *kernel, fv_point_t anchor,
        fv_s32 iterations, fv_u32 border_type)
{
    fv_mat_t            *_ker = kernel;
    fv_size_t           ksize;
    fv_bool             kernel_set;

    FV_ASSERT(op < FV_MOP_MAX);

    kernel_set = kernel != NULL && kernel->mt_data.dt_ptr != NULL;
    ksize = kernel_set ? 
        fv_size(kernel->mt_cols, kernel->mt_rows) : fv_size(3, 3);
    anchor = fv_normalize_anchor(anchor, ksize);

    if (iterations == 0 || 
            (kernel != NULL && kernel->mt_rows*kernel->mt_cols == 1)) {
        fv_copy_mat(dst, src);
        return;
    }

    if (!kernel_set) {
        _ker = fv_get_structuring_element(FV_SHAPE_RECT, 
                fv_size(1 + iterations*2, 1 + iterations*2), fv_point(-1, -1));
        anchor = fv_point(iterations, iterations);
        iterations = 1;
    } else if (iterations > 1 && 
            _fv_count_non_zero(kernel) == kernel->mt_rows*kernel->mt_cols) {
        anchor = fv_point(anchor.pt_x*iterations, anchor.pt_y*iterations);
        _ker = fv_get_structuring_element(FV_SHAPE_RECT, 
                fv_size(ksize.sz_width + (iterations - 1)*(ksize.sz_width - 1),
                    ksize.sz_height + (iterations - 1)*(ksize.sz_height - 1)),
                anchor);
        iterations = 1;
    }

    fv_time_meter_set(FV_TIME_METER2);
    fv_morph_op_iterate(op, dst, src, _ker, anchor, iterations, border_type);
    fv_time_meter_get(FV_TIME_METER2, 0);

    if (_ker != kernel) {
        fv_release_mat(&_ker);
    }
}

void 
_fv_dilate(fv_mat_t *dst, fv_mat_t *src, fv_mat_t *kernel, fv_point_t anchor,
        fv_s32 iterations, fv_u32 border_type)
{
    fv_morph_op(FV_MOP_DILATE, dst, src, kernel, anchor, 
                iterations, border_type);
}

void 
fv_dilate_default(fv_mat_t *dst, fv_mat_t *src, fv_mat_t *kernel)
{
    _fv_dilate(dst, src, kernel, fv_point(-1, -1), 1, FV_BORDER_DEFAULT);
}

void 
fv_dilate(fv_image_t *dst, fv_image_t *src, fv_conv_kernel_t *element,
            fv_s32 iterations)
{
    fv_point_t  anchor;
    fv_mat_t    _dst;
    fv_mat_t    _src;
    fv_mat_t    *kernel;

    FV_ASSERT(dst->ig_image_size == src->ig_image_size &&
            dst->ig_depth == src->ig_depth && 
            dst->ig_channels == src->ig_channels);

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);
    kernel = fv_convert_conv_kernel(element, &anchor);
    _fv_dilate(&_dst, &_src, kernel, anchor, iterations, FV_BORDER_REPLICATE);
    fv_release_mat(&kernel);
}

void 
_fv_erode(fv_mat_t *dst, fv_mat_t *src, fv_mat_t *kernel, fv_point_t anchor,
        fv_s32 iterations, fv_u32 border_type)
{
    fv_morph_op(FV_MOP_ERODE, dst, src, kernel, anchor, 
                iterations, border_type);
}

void 
fv_erode(fv_image_t *dst, fv_image_t *src, fv_conv_kernel_t *element,
            fv_s32 iterations)
{
    fv_point_t  anchor;
    fv_mat_t    _dst;
    fv_mat_t    _src;
    fv_mat_t    *kernel;

    FV_ASSERT(dst->ig_image_size == src->ig_image_size &&
            dst->ig_depth == src->ig_depth && 
            dst->ig_channels == src->ig_channels);

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);
    kernel = fv_convert_conv_kernel(element, &anchor);
    _fv_erode(&_dst, &_src, kernel, anchor, iterations, FV_BORDER_REPLICATE);
    fv_release_mat(&kernel);
}
