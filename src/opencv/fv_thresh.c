
#include "fv_types.h"
#include "fv_core.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_thresh.h"
#include "fv_debug.h"
#include "fv_time.h"

#define FV_THRESHOLD_WIN_NAME   "threshold"
#define FV_THRESHOLD_THRESH     200
#define FV_THRESHOLD_MAX_VALUE  200
#define FV_THRESHOLD_TYPE       8

fv_s32 
fv_cv_threshold(IplImage *cv_img, fv_bool image)
{
    IplImage    *dst;
    IplImage    *gray;
    fv_image_t  *_src;
    fv_image_t  *_dst;
    fv_size_t   size;
    fv_s32      c;

    FV_ASSERT(image);
    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    dst = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(dst != NULL);

    fv_time_meter_set(FV_TIME_METER1);
    cvThreshold(gray, dst, FV_THRESHOLD_THRESH, FV_THRESHOLD_MAX_VALUE, 
            FV_THRESHOLD_TYPE);
    fv_time_meter_get(FV_TIME_METER1, 0);
    cvNamedWindow(FV_THRESHOLD_WIN_NAME, 0);  
    cvShowImage(FV_THRESHOLD_WIN_NAME, dst);  
    c = cvWaitKey(0);  

    _src = fv_convert_image(gray);
    FV_ASSERT(_src != NULL);
    size = fv_get_size(_src);
    _dst = fv_create_image(size, _src->ig_depth, _src->ig_channels);
    fv_time_meter_set(FV_TIME_METER1);
    fv_threshold(_dst, _src, FV_THRESHOLD_THRESH, FV_THRESHOLD_MAX_VALUE, 
            FV_THRESHOLD_TYPE);
    fv_time_meter_get(FV_TIME_METER1, 0);
    fv_release_image(&_src);
    fv_cv_img_to_ipl(dst, _dst);
    fv_release_image(&_dst);

    cvShowImage(FV_THRESHOLD_WIN_NAME, dst);  
    c = cvWaitKey(0);  
    printf("c = %d\n", c);
    cvDestroyWindow(FV_THRESHOLD_WIN_NAME); 

    cvReleaseImage(&dst);
    cvReleaseImage(&gray);

    return FV_OK;
}
