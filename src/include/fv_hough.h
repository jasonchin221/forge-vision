#ifndef __FV_HOUGH_H__
#define __FV_HOUGH_H__

#include <opencv/cv.h>  
#include <opencv/highgui.h>

enum {
    FV_HOUGH_STANDARD,
    FV_HOUGH_PROBABILISTIC,
    FV_HOUGH_MULTI_SCALE,
    FV_HOUGH_GRADIENT,
};

extern fv_s32 fv_hough_lines(fv_image_t *image, void *line_storage, fv_s32 len,
                fv_s32 method, double rho, double theta, fv_s32 threshold, 
                double param1, double param2);
extern fv_s32 fv_hough_circles(fv_image_t *image, void *circle_storage,
                fv_s32 len, fv_s32 method, double dp,
                double min_dist, double param1, double param2,
                fv_s32 min_radius, fv_s32 max_radius);
extern fv_s32 fv_cv_hough(IplImage *cv_img, fv_bool image);

#endif
