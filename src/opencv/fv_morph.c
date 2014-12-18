
#include "fv_types.h"
#include "fv_core.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_morph.h"
#include "fv_debug.h"
#include "fv_time.h"

#define FV_MORPH_SHAPE_DEFAULT  0
#define FV_MORPH_WIN_NAME       "morph"
#define FV_MORPH_ITERATIONS     1
#define FV_THRESHOLD_THRESH     200
#define FV_THRESHOLD_MAX_VALUE  200
#define FV_THRESHOLD_TYPE       8

#define FV_STRUCT_ELEM_COL      3
#define FV_STRUCT_ELEM_ROW      3
#define FV_STRUCT_ANCHOR_X      1
#define FV_STRUCT_ANCHOR_Y      1
#define FV_STRUCT_SHAPE        CV_SHAPE_CROSS

static fv_s32 
_fv_cv_morph(IplImage *cv_img, fv_bool image, fv_bool dilate)
{
    IplConvKernel       *elem;
    fv_conv_kernel_t    *kernel;
    IplImage            *dst;
    IplImage            *gray;
    IplImage            *bina;
    fv_image_t          *_src;
    fv_image_t          *_dst;
    fv_size_t           size;
    fv_s32              c;

    elem = cvCreateStructuringElementEx(FV_STRUCT_ELEM_COL,
                FV_STRUCT_ELEM_ROW, FV_STRUCT_ANCHOR_X,
                FV_STRUCT_ANCHOR_Y, FV_STRUCT_SHAPE, NULL);

    kernel = fv_create_structuring_element_ex(FV_STRUCT_ELEM_COL,
                FV_STRUCT_ELEM_ROW, FV_STRUCT_ANCHOR_X,
                FV_STRUCT_ANCHOR_Y, FV_STRUCT_SHAPE, NULL);

    FV_ASSERT(image);
    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    bina = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(bina != NULL);

    cvThreshold(gray, bina, FV_THRESHOLD_THRESH, FV_THRESHOLD_MAX_VALUE, 
            FV_THRESHOLD_TYPE);
    cvReleaseImage(&gray);

    dst = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(dst != NULL);

    fv_time_meter_set(FV_TIME_METER1);
#if FV_MORPH_SHAPE_DEFAULT  
    if (dilate) {
        cvDilate(bina, dst, NULL, FV_MORPH_ITERATIONS);
    } else {
        cvErode(bina, dst, NULL, FV_MORPH_ITERATIONS);
    }
#else
    if (dilate) {
        cvDilate(bina, dst, elem, FV_MORPH_ITERATIONS);
    } else {
        cvErode(bina, dst, elem, FV_MORPH_ITERATIONS);
    }
#endif
    fv_time_meter_get(FV_TIME_METER1, 0);

    cvNamedWindow(FV_MORPH_WIN_NAME, 0);  
    cvShowImage(FV_MORPH_WIN_NAME, bina);  
    c = cvWaitKey(0);  
    cvShowImage(FV_MORPH_WIN_NAME, dst);  
    c = cvWaitKey(0);  

    _src = fv_convert_image(bina);
    FV_ASSERT(_src != NULL);
    size = fv_get_size(_src);
    _dst = fv_create_image(size, _src->ig_depth, _src->ig_channels);
    fv_time_meter_set(FV_TIME_METER1);
#if FV_MORPH_SHAPE_DEFAULT  
    if (dilate) {
        fv_dilate(_dst, _src, NULL, FV_MORPH_ITERATIONS);
    } else {
        fv_erode(_dst, _src, NULL, FV_MORPH_ITERATIONS);
    }
#else
    if (dilate) {
        fv_dilate(_dst, _src, kernel, FV_MORPH_ITERATIONS);
    } else {
        fv_erode(_dst, _src, kernel, FV_MORPH_ITERATIONS);
    }
#endif
    fv_time_meter_get(FV_TIME_METER1, 0);
    fv_release_image(&_src);
    fv_cv_img_to_ipl(dst, _dst);
    fv_release_image(&_dst);

    cvShowImage(FV_MORPH_WIN_NAME, dst);  
    c = cvWaitKey(0);  
    printf("c = %d\n", c);
    cvDestroyWindow(FV_MORPH_WIN_NAME); 

    cvReleaseImage(&dst);
    cvReleaseImage(&bina);

    fv_release_structuring_element(&kernel);
    cvReleaseStructuringElement(&elem);

    return FV_OK;
}

fv_s32 
fv_cv_dilate(IplImage *cv_img, fv_bool image)
{
    return _fv_cv_morph(cv_img, image, 1);
}

fv_s32 
fv_cv_erode(IplImage *cv_img, fv_bool image)
{
    return _fv_cv_morph(cv_img, image, 0);
}

