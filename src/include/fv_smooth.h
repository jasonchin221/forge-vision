#ifndef __FV_SMOOTH_H__
#define __FV_SMOOTH_H__

#include <opencv/cv.h>  
#include <opencv/highgui.h>

#include "fv_filter.h"

typedef struct _fv_sum_row_filter_t {
    fv_base_row_filter_t    sr_base;
#define sr_filter           sr_base.br_filter
#define sr_anchor           sr_base.br_anchor
#define sr_ksize            sr_base.br_ksize
#define sr_type             sr_base.br_type
    fv_u32                  sr_nchannels;
} fv_sum_row_filter_t;

typedef struct _fv_sum_column_filter_t {
    fv_base_column_filter_t sc_base;
#define sc_filter           sc_base.bc_filter
#define sc_anchor           sc_base.bc_anchor
#define sc_ksize            sc_base.bc_ksize
#define sc_type             sc_base.bc_type
    fv_u32                  sc_nchannels;
    double                  *sc_sum;
    fv_u32                  sc_sum_count;
    double                  sc_scale;
} fv_sum_column_filter_t;


extern void fv_box_filter(fv_mat_t *dst, fv_mat_t *src, fv_s32 ddepth,
                fv_size_t ksize, fv_point_t anchor, 
                fv_bool normalize, fv_s32 border_type);
extern void fv_smooth(fv_image_t *dstarr, fv_image_t *srcarr, 
            fv_u32 smooth_type, fv_s32 param1, fv_s32 param2, 
            double param3, double param4);
extern fv_s32 fv_cv_smooth(IplImage *cv_img, fv_bool image);

#endif
