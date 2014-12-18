
#include "fv_types.h"
#include "fv_core.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_debug.h"
#include "fv_stat.h"
#include "fv_time.h"

#define FV_THRESHOLD_THRESH     200
#define FV_THRESHOLD_MAX_VALUE  200
#define FV_THRESHOLD_TYPE       8

static fv_s32 
__fv_cv_count_non_zero(IplImage *cv_img, fv_bool image, fv_bool opencv)
{
    IplImage    *gray;
    IplImage    *bina;
    fv_image_t  *src;
    fv_s32      num;

    FV_ASSERT(image);
    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);
    bina = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(bina != NULL);

    cvThreshold(gray, bina, FV_THRESHOLD_THRESH, FV_THRESHOLD_MAX_VALUE, 
            FV_THRESHOLD_TYPE);
    cvReleaseImage(&gray);

    if (opencv) {
        fv_time_meter_set(FV_TIME_METER1);
        num = cvCountNonZero(bina);
        fv_time_meter_get(FV_TIME_METER1, 0);
    } else {
        src = fv_convert_image(bina);
        FV_ASSERT(src != NULL);
        fv_time_meter_set(FV_TIME_METER1);
        num = fv_count_non_zero(src);
        fv_time_meter_get(FV_TIME_METER1, 0);
        fv_release_image(&src);
    }

    cvReleaseImage(&bina);

    fprintf(stdout, "Non zero num is %d\n", num);

    return FV_OK;
}

static fv_s32 
_fv_cv_count_non_zero(IplImage *cv_img, fv_bool image)
{
    fv_s32  num1;
    fv_s32  num2;

    num1 = __fv_cv_count_non_zero(cv_img, image, 0);
    num2 = __fv_cv_count_non_zero(cv_img, image, 1);
    if (num1 != num2) {
        fprintf(stderr, "Error!\n");
        return FV_ERROR;
    }

    fprintf(stdout, "OK!\n");

    return FV_OK;
}

fv_s32 
fv_cv_count_non_zero(IplImage *cv_img, fv_bool image)
{
    return _fv_cv_count_non_zero(cv_img, image);
}

