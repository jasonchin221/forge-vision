
#include "fv_types.h"
#include "fv_opencv.h"
#include "fv_core.h"
#include "fv_pyramid.h"
#include "fv_debug.h"
#include "fv_time.h"

#define FV_PYR_NAME     "Pyramid"

static void 
_fv_cv_pyr_down_mine(IplImage *dst, IplImage *src)
{
    fv_image_t      *img;
    fv_image_t      *_dst;
    fv_image_t      *pyr;
    fv_size_t       size;

    img = fv_convert_image(src);
    FV_ASSERT(img != NULL);

    _dst = fv_convert_image(dst);
    FV_ASSERT(_dst != NULL);
    size = fv_get_size(_dst);
    pyr = fv_create_image(size, _dst->ig_depth, _dst->ig_channels);
    FV_ASSERT(pyr != NULL);

    fv_time_meter_set(FV_TIME_METER1);
    fv_pyr_down(pyr, img, 0);
    fv_time_meter_get(FV_TIME_METER1, 0);

    fv_cv_img_to_ipl(dst, pyr);

    fv_release_image(&pyr);
    fv_release_image(&img);
}

fv_s32 
fv_cv_pyr_down(IplImage *cv_img, fv_bool image)
{
    IplImage    *gray;
    IplImage    *dst;
    CvSize      size;
    int         c;

    FV_ASSERT(image);
    size = cvGetSize(cv_img);
    gray = cvCreateImage(size, cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
 
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    size.width /= 2;
    size.height /= 2;
    dst = cvCreateImage(size, gray->depth, gray->nChannels);
    FV_ASSERT(dst != NULL);

    fv_time_meter_set(FV_TIME_METER1);
    cvPyrDown(gray, dst, CV_GAUSSIAN_5x5);
    fv_time_meter_get(FV_TIME_METER1, 0);

    cvNamedWindow(FV_PYR_NAME, 0);  
    cvShowImage(FV_PYR_NAME, dst);  
    c = cvWaitKey(0);  
    printf("c = %d\n", c);

    _fv_cv_pyr_down_mine(dst, gray);
    cvShowImage(FV_PYR_NAME, dst);  
    c = cvWaitKey(0);  
    cvDestroyWindow(FV_PYR_NAME); 

    cvReleaseImage(&dst);
    cvReleaseImage(&gray);

    return FV_OK;

}
