#ifndef __FV_OPENCV_H__
#define __FV_OPENCV_H__

#include <opencv/cv.h>  
#include <opencv/highgui.h>

typedef fv_s32 (*fv_proc_func)(IplImage *, fv_bool);

extern fv_s32 fv_cv_detect_img(char *im_file, fv_proc_func proc);
extern fv_s32 fv_cv_detect_video(char *vd_file, fv_proc_func proc);
extern fv_s32 fv_cv_detect_camera(char *vd_file, fv_proc_func proc);
extern fv_s32 fv_cv_save_img(char *file_name, fv_mat_t *mat);
extern void fv_cv_img_to_ipl(IplImage *cv_img, fv_image_t *img);

#endif
