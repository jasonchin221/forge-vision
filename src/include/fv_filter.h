#ifndef __FV_FILTER_H__
#define __FV_FILTER_H__

#include "fv_debug.h"

struct _fv_base_filter_t;
struct _fv_base_row_filter_t;
struct _fv_base_column_filter_t;

typedef void (*fv_filter_2D_func)(void *dst, void *src, fv_s32, 
            fv_s32 , float *, fv_u32 cn, struct _fv_base_filter_t *);

typedef void (*fv_row_filter_func)(double *dst, void *src, fv_s32, 
            float *, struct _fv_base_row_filter_t *);
typedef void (*fv_column_filter_func)(void *dst, double **src, fv_s32, 
            fv_s32 , float *, struct _fv_base_column_filter_t *);

typedef struct _fv_base_filter_t {
    fv_filter_2D_func       bf_filter;
    fv_point_t              bf_anchor;
    fv_size_t               bf_ksize;
} fv_base_filter_t;

typedef struct _fv_base_row_filter_t {
    fv_row_filter_func      br_filter;
    fv_s32                  br_anchor;
    fv_s32                  br_ksize;
    fv_s32                  br_type;
} fv_base_row_filter_t;

typedef struct _fv_base_column_filter_t {
    fv_column_filter_func   bc_filter;
    fv_s32                  bc_anchor;
    fv_s32                  bc_ksize;
    fv_s32                  bc_type;
} fv_base_column_filter_t;

typedef struct _fv_linear_row_filter_t {
    fv_base_row_filter_t    lr_base;
#define lr_filter   lr_base.br_filter   
#define lr_anchor   lr_base.br_anchor   
#define lr_ksize    lr_base.br_ksize    
#define lr_type     lr_base.br_type     
    fv_u32                  lr_nchannels;
} fv_linear_row_filter_t;

typedef struct _fv_linear_column_filter_t {
    fv_base_column_filter_t lc_base;
#define lc_filter   lc_base.bc_filter   
#define lc_anchor   lc_base.bc_anchor   
#define lc_ksize    lc_base.bc_ksize    
#define lc_type     lc_base.bc_type     
} fv_linear_column_filter_t;

typedef struct _fv_filter_engine_t {
    fv_base_filter_t            *fe_filter_2D;
    fv_base_row_filter_t        *fe_row_filter;
    fv_base_column_filter_t     *fe_col_filter;
    fv_bool                     fe_is_separable;
} fv_filter_engine_t;

static inline void 
_fv_normalize_anchor(fv_s32 *anchor, fv_s32 ksize)
{
    if (*anchor < 0) {
        *anchor = ksize >> 1;
    }

    FV_ASSERT(0 <= *anchor && *anchor < ksize);
}

static inline fv_point_t
fv_normalize_anchor(fv_point_t anchor, fv_size_t ksize)
{
    fv_point_t  _anchor = anchor;

    _fv_normalize_anchor(&_anchor.pt_x, ksize.sz_width);
    _fv_normalize_anchor(&_anchor.pt_y, ksize.sz_height);

    return _anchor;
}


extern void fv_sep_filter_proceed(fv_mat_t *dst, fv_mat_t *src,
                fv_mat_t *kernel_x, fv_mat_t *kernel_y, 
                fv_point_t anchor, double delta, fv_s32 border_type, 
                fv_filter_engine_t *filter);
extern void fv_sep_filter2D(fv_mat_t *dst, fv_mat_t *src,
                fv_mat_t *kernel_x, fv_mat_t *kernel_y, 
                fv_point_t anchor, double delta, fv_s32 border_type);
extern void fv_sep_conv_small3_32f(float *dst, fv_s32 dst_step, 
            float *src, fv_s32 src_step, fv_size_t src_size, 
            float *kx, float *ky, float *buffer);
extern void fv_preprocess_2D_kernel(fv_mat_t *kernel,
            fv_point_t **coords, fv_u32 nz);

#endif
