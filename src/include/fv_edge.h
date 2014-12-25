#ifndef __FV_EDGE_H__
#define __FV_EDGE_H__

extern void _fv_sobel(fv_mat_t *dst, fv_mat_t *src, fv_s32 ddepth, 
                fv_s32 dx, fv_s32 dy, fv_s32 ksize, 
                double scale, double delta, fv_s32 border_type);
extern void fv_sobel(fv_image_t *dst, fv_image_t *src, fv_s32 dx, 
                fv_s32 dy, fv_s32 aperture_size);
extern void _fv_scharr(fv_mat_t *dst, fv_mat_t *src, fv_s32 ddepth, 
                fv_s32 dx, fv_s32 dy, double scale, 
                double delta, fv_s32 border_type);
extern void fv_laplace(fv_image_t *dst, fv_image_t *src, fv_s32 aperture_size);
extern void fv_canny(fv_image_t *dst, fv_image_t *src, double thresh1,
            double thresh2, fv_s32 aperture_size);
extern fv_s32 fv_cv_sobel(IplImage *cv_img, fv_bool image);
extern fv_s32 fv_cv_laplace(IplImage *cv_img, fv_bool image);
extern fv_s32 fv_cv_canny(IplImage *cv_img, fv_bool image);

#endif
