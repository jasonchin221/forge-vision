
#include "fv_types.h"
#include "fv_opencv.h"
#include "fv_core.h"
#include "fv_matrix.h"
#include "fv_debug.h"
#include "fv_math.h"
#include "fv_time.h"

static fv_s32 
__fv_cv_min_max_loc(IplImage *cv_img, IplImage *mask, char *info)
{
    fv_image_t  *_mask = NULL;
    fv_image_t  *img;
    CvPoint     cv_min_loc;
    CvPoint     cv_max_loc;
    fv_point_t  min_loc1 = {};
    fv_point_t  max_loc1 = {};
    fv_point_t  min_loc2 = {};
    fv_point_t  max_loc2 = {};
    double      cv_min_val;
    double      cv_max_val;
    double      min_val;
    double      max_val;

    fv_time_meter_set(FV_TIME_METER1);
    cvMinMaxLoc(cv_img, &cv_min_val, &cv_max_val,
            &cv_min_loc, &cv_max_loc, mask);
    fv_time_meter_get(FV_TIME_METER1, 0);
    min_loc1.pt_x = cv_min_loc.x;
    min_loc1.pt_y = cv_img->height - cv_min_loc.y - 1;
    max_loc1.pt_x = cv_max_loc.x;
    max_loc1.pt_y = cv_img->height - cv_max_loc.y - 1;
    if (cv_img->roi) {
        min_loc1.pt_x += cv_img->roi->xOffset;
        min_loc1.pt_y -= cv_img->roi->yOffset;
        max_loc1.pt_x += cv_img->roi->xOffset;
        max_loc1.pt_y -= cv_img->roi->yOffset;
    }

    img = fv_convert_image(cv_img);
    FV_ASSERT(img != NULL);
    if (mask != NULL) {
        _mask = fv_convert_image(mask);
        FV_ASSERT(_mask != NULL);
    }
    fv_time_meter_set(FV_TIME_METER1);
    fv_min_max_loc(img, &min_val, &max_val,
            &min_loc2, &max_loc2, _mask);
    fv_time_meter_get(FV_TIME_METER1, 0);
    if (img->ig_roi) {
        min_loc2.pt_x += img->ig_roi->ri_x_offset;
        min_loc2.pt_y += img->ig_roi->ri_y_offset;
        max_loc2.pt_x += img->ig_roi->ri_x_offset;
        max_loc2.pt_y += img->ig_roi->ri_y_offset;
    }

    if (memcmp(&min_loc1, &min_loc2, sizeof(min_loc1)) != 0 ||
            !fv_double_eq(cv_max_val, max_val)) {
        fprintf(stdout, "%s cv min[%d %d] = %f, max[%d %d] = %f\n", info,
                min_loc1.pt_x, min_loc1.pt_y, cv_min_val, 
                max_loc1.pt_x, max_loc1.pt_y, cv_max_val);
        fprintf(stdout, "%s min[%d %d] = %f, max[%d %d] = %f\n", info,
                min_loc2.pt_x, min_loc2.pt_y, min_val, 
                max_loc2.pt_x, max_loc2.pt_y, max_val);

        return FV_ERROR;
    }

    fprintf(stdout, "%s OK!\n", info);

    return FV_OK;
}


static fv_s32 
_fv_cv_min_max_loc(IplImage *cv_img, fv_bool image)
{
    IplImage    *gray;
    IplImage    *mask;
    fv_rect_t   rect = {{200, 300}, {100, 200}};
    fv_s32      ret;

    FV_ASSERT(image);
    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
 
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    ret = __fv_cv_min_max_loc(gray, NULL, "origin");
    FV_ASSERT(ret == FV_OK);
    mask = cvCreateImage(cvGetSize(gray), IPL_DEPTH_8U, 1);
    FV_ASSERT(mask != NULL);
    memset(mask->imageData, 1, 200);
    ret = __fv_cv_min_max_loc(gray, mask, "mask");
    FV_ASSERT(ret == FV_OK);
    cvReleaseImage(&gray);
    cvReleaseImage(&mask);

    cvSetImageROI(cv_img, cvRect(rect.rt_x, 
                cv_img->height - rect.rt_y - rect.rt_height, 
                rect.rt_width, rect.rt_height));

    cv_img->roi->coi = 1;
    ret = __fv_cv_min_max_loc(cv_img, NULL, "roi");
    FV_ASSERT(ret == FV_OK);
    cvResetImageROI(cv_img);

    return FV_OK;
}

fv_s32 
fv_cv_min_max_loc(IplImage *cv_img, fv_bool image)
{
    return _fv_cv_min_max_loc(cv_img, image);
}
