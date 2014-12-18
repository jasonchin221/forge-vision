#ifndef __FV_THRESH_H__
#define __FV_THRESH_H__

typedef struct _fv_thresh_proc_t {
    fv_u32      tp_type;
    double      (*tp_get_value)(double , double, double);
} fv_thresh_proc_t;

extern void fv_threshold(fv_image_t *dst, fv_image_t *src, double thresh,
        double max_value, fv_u32 type);
extern void _fv_threshold(fv_mat_t *dst, fv_mat_t *src, double thresh,
        double max_value, fv_u32 type);
extern fv_s32 fv_cv_threshold(IplImage *cv_img, fv_bool image);

#endif
