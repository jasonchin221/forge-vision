#ifndef __FV_STAT_H__
#define __FV_STAT_H__

extern fv_s32 _fv_count_non_zero(fv_mat_t *mat);
extern fv_s32 fv_count_non_zero(fv_arr *arr);
extern fv_s32 fv_cv_count_non_zero(IplImage *cv_img, fv_bool image);

#endif
