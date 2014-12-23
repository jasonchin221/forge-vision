#ifndef __FV_MORPH_H__
#define __FV_MORPH_H__

#include "fv_filter.h"

/* Morphological operations */
enum {
    FV_MOP_ERODE        = 0,
    FV_MOP_DILATE       = 1,
    FV_MOP_OPEN         = 2,
    FV_MOP_CLOSE        = 3,
    FV_MOP_GRADIENT     = 4,
    FV_MOP_TOPHAT       = 5,
    FV_MOP_BLACKHAT     = 6,
    FV_MOP_MAX,
};

//! shape of the structuring element
enum { 
    FV_SHAPE_RECT = 0, 
    FV_SHAPE_CROSS, 
    FV_SHAPE_ELLIPSE,
    FV_SHAPE_CUSTOM = 100,
};

typedef double (*fv_morph_op_func)(double, double);

typedef struct _fv_morphology_filter_2D_t {
    fv_base_filter_t        mf_base;
#define mf_filter           mf_base.bf_filter
#define mf_anchor           mf_base.bf_anchor
#define mf_ksize            mf_base.bf_ksize
    fv_u32                  mf_nchannels;
    fv_u32                  mf_nz;
    fv_morph_op_func        mf_op;
    fv_point_t              *mf_coords;
    double                  *mf_coeffs;
    void                    *mf_ptrs;
} fv_morphology_filter_2D_t;

typedef struct _fv_morphology_row_filter_t {
    fv_base_row_filter_t    mr_base;
#define mr_filter           mr_base.br_filter
#define mr_anchor           mr_base.br_anchor
#define mr_ksize            mr_base.br_ksize
#define mr_type             mr_base.br_type
    fv_u32                  mr_nchannels;
    fv_morph_op_func        mr_op;
} fv_morphology_row_filter_t;

typedef struct _fv_morphology_column_filter_t {
    fv_base_column_filter_t mc_base;
#define mc_filter           mc_base.bc_filter
#define mc_anchor           mc_base.bc_anchor
#define mc_ksize            mc_base.bc_ksize
#define mc_type             mc_base.bc_type
    fv_u32                  mc_nchannels;
    fv_morph_op_func        mc_op;
} fv_morphology_column_filter_t;


extern void _fv_dilate(fv_mat_t *dst, fv_mat_t *src, fv_mat_t *kernel, 
            fv_point_t anchor, fv_s32 iterations, fv_u32 border_type);
extern void fv_dilate(fv_image_t *dst, fv_image_t *src,
            fv_conv_kernel_t *element, fv_s32 iterations);
extern void fv_erode(fv_image_t *dst, fv_image_t *src, 
            fv_conv_kernel_t *element, fv_s32 iterations);
extern void  fv_dilate_default(fv_mat_t *dst, fv_mat_t *src, 
             fv_mat_t *kernel);
extern fv_s32 fv_cv_dilate(IplImage *cv_img, fv_bool image);
extern fv_s32 fv_cv_erode(IplImage *cv_img, fv_bool image);
extern fv_conv_kernel_t *fv_create_structuring_element_ex(fv_s32 cols,
            fv_s32 rows, fv_s32 anchor_x, fv_s32 anchor_y,
            fv_s32 shape, fv_s32 *values);
extern void fv_release_structuring_element(fv_conv_kernel_t **element);

#endif
