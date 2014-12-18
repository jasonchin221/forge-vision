#ifndef __FV_PYRAMID_H__
#define __FV_PYRAMID_H__

extern void fv_pyr_down(fv_image_t *dst, fv_image_t *src, fv_u32 filter);
extern void _fv_pyr_down(fv_mat_t *dst, fv_mat_t *src, fv_u32 filter);
extern fv_s32 fv_cv_pyr_down(IplImage *cv_img, fv_bool image);

#endif
