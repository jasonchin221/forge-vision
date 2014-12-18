
#include "fv_types.h"
#include "fv_opencv.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_time.h"
#include "fv_smooth.h"

#define FV_SMOOTH_NAME     "smooth"
#define FV_SMOOTH_TYPE      CV_BLUR_NO_SCALE//CV_BLUR
#define FV_SMOOTH_PARAM1    3
#define FV_SMOOTH_PARAM2    4
#define FV_SMOOTH_PARAM3    0
#define FV_SMOOTH_PARAM4    0

static void 
_fv_cv_smooth(IplImage *dst, IplImage *src)
{
    fv_image_t      *img;
    fv_image_t      *_dst;
    fv_image_t      *sm;
    fv_size_t       size;

    img = fv_convert_image(src);
    FV_ASSERT(img != NULL);

    _dst = fv_convert_image(dst);
    FV_ASSERT(_dst != NULL);
    size = fv_get_size(_dst);
    sm = fv_create_image(size, _dst->ig_depth, _dst->ig_channels);
    FV_ASSERT(sm != NULL);

    fv_time_meter_set(FV_TIME_METER1);
    fv_smooth(sm, img, FV_SMOOTH_TYPE, FV_SMOOTH_PARAM1,
            FV_SMOOTH_PARAM2, FV_SMOOTH_PARAM3, FV_SMOOTH_PARAM4);
    fv_time_meter_get(FV_TIME_METER1, 0);
    fv_cv_img_to_ipl(dst, sm);

    fv_release_image(&sm);
    fv_release_image(&img);
}

fv_s32 
fv_cv_smooth(IplImage *cv_img, fv_bool image)
{
    IplImage    *dst;
    CvSize      size;
    int         c;
    int         depth;

    size = cvGetSize(cv_img);
    FV_ASSERT(image);
    if (FV_SMOOTH_TYPE == CV_BLUR) {
        depth = cv_img->depth;
    } else {
        depth = IPL_DEPTH_16S;
    }
    printf("cn = %d\n", cv_img->nChannels);
    dst = cvCreateImage(size, depth, cv_img->nChannels);
    FV_ASSERT(dst != NULL);

    fv_time_meter_set(FV_TIME_METER1);
    cvSmooth(cv_img, dst, FV_SMOOTH_TYPE, FV_SMOOTH_PARAM1,
            FV_SMOOTH_PARAM2, FV_SMOOTH_PARAM3, FV_SMOOTH_PARAM4);
    fv_time_meter_get(FV_TIME_METER1, 0);

    cvNamedWindow(FV_SMOOTH_NAME, 0);  
    cvShowImage(FV_SMOOTH_NAME, dst);  
    c = cvWaitKey(0);  
    printf("c = %d\n", c);

    _fv_cv_smooth(dst, cv_img);
    cvShowImage(FV_SMOOTH_NAME, dst);  
    c = cvWaitKey(0);  
    cvDestroyWindow(FV_SMOOTH_NAME); 

    cvReleaseImage(&dst);

    return FV_OK;

}
