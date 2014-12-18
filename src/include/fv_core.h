#ifndef __FV_CORE_H__
#define __FV_CORE_H__

#include <opencv/cv.h>  
#include <opencv/highgui.h>

#include "fv_imgproc.h"

extern fv_size_t fv_get_size(fv_arr *arr);
extern fv_s32 fv_inc_ref_data(fv_arr *arr);
extern fv_image_t *fv_create_image(fv_size_t size, 
                fv_s32 depth, fv_s32 channels);
extern void fv_release_image(fv_image_t **image);
extern fv_image_t *fv_convert_image(IplImage *cv_img);
extern fv_mat_t *fv_create_mat(fv_s32 height, fv_s32 width, fv_u32 atr);
extern void fv_copy_mat(fv_mat_t *dst, fv_mat_t *src);
extern void _fv_release_mat(fv_mat_t *array);
extern void fv_release_mat(fv_mat_t **array);
extern fv_mat_t fv_image_to_mat(fv_image_t *image);
extern double fv_mget(fv_mat_t *mat, fv_s32 row, fv_s32 col, 
            fv_u16 channel, fv_u32 border_type);
extern void fv_mset(fv_mat_t *mat, fv_s32 row, fv_s32 col, 
            fv_u16 channel, double value);
extern fv_mat_t *fv_mat_extract_image_coi(fv_image_t *image);
extern void fv_convert_mat(fv_mat_t *dst, fv_mat_t *src);

#endif
