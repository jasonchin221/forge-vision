#include "fv_types.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_track.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_time.h"

#define FV_CV_MAX_CORNERS   500

static IplImage *fv_cv_prev_img;
static CvPoint2D32f fv_cv_corners_a[FV_CV_MAX_CORNERS];
static CvPoint2D32f fv_cv_corners_b[FV_CV_MAX_CORNERS];
static CvPoint2D32f fv_cv_corners_c[FV_CV_MAX_CORNERS];
static fv_s8 fv_cv_features_found[FV_CV_MAX_CORNERS];
static float fv_cv_feature_errors[FV_CV_MAX_CORNERS];
static fv_bool fv_track_points = 0;

static void
fv_track_label_points(CvPoint2D32f *corners_b, CvPoint2D32f *corners_a, 
            fv_s32 corner_count, fv_s8 *features_found)
{   
    fv_u32      i;

    printf("corner count = %d\n", corner_count);
    for (i = 0; i < corner_count; i++) {
        corners_b[i] = corners_a[i];
        corners_b[i].y += 10;
    }
    memset(features_found, 1, corner_count);
}

/*
 * fv_lk_optical_flow_image:
 * @prev:
 * @curr:
 * @corners_a:
 * @corners_b:
 * @features_found:
 * @feature_errors:
 * @corner_max_num: the max number of corners_a and corners_b
 * return: the number of track points
 */
static fv_s32 
fv_lk_optical_flow_image(IplImage *prev, IplImage *curr, 
            CvPoint2D32f *corners_a, CvPoint2D32f *corners_b, 
            fv_s8 *features_found, float *feature_errors, 
            fv_s32 corner_max_num)
{
    fv_image_t      *prev_img;
    fv_image_t      *curr_img;
    fv_s32          height;
    fv_s32          i;
    fv_s32          corner_count = corner_max_num;

    prev_img = fv_convert_image(prev);
    if (prev_img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    curr_img = fv_convert_image(curr);
    if (curr_img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    height = prev_img->ig_height;
    fv_time_meter_set(FV_TIME_METER2);
    fv_good_features_to_track(prev_img, (fv_point_2D32f_t *)corners_a,
            &corner_count, FV_TRACK_QUALITY_LEVEL, FV_TRACK_MIN_DISTANCE,
            FV_TRACK_BLOCK_SIZE, NULL, FV_TRACK_USE_HARRIS, FV_TRACK_HARRIS_K);
    fv_time_meter_get(FV_TIME_METER2, 0);
    if (fv_track_points) {
        fv_track_label_points(corners_b, corners_a, 
                corner_count, features_found);
        goto out;
    }

    fv_find_corner_sub_pix(prev_img, (fv_point_2D32f_t *)corners_a, 
            corner_count, fv_size(FV_TRACK_WIN_SIZE, FV_TRACK_WIN_SIZE), 
            fv_size(-1, -1), fv_term_criteria(FV_TERMCRIT_ITER|FV_TERMCRIT_EPS,
                FV_TRACK_MAX_ITER, FV_TRACK_EPSISON));

    fv_time_meter_set(FV_TIME_METER2);
    fv_calc_optical_flow_pyr_lk(prev_img, curr_img,
                (fv_point_2D32f_t *)corners_a, (fv_point_2D32f_t *)corners_b,
                corner_count, fv_size(FV_TRACK_WIN_SIZE, FV_TRACK_WIN_SIZE), 
                FV_TRACK_PYR_LEVEL, features_found, feature_errors,
                fv_term_criteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 
                    FV_TRACK_MAX_ITER, FV_TRACK_EPSISON), 0);
    fv_time_meter_get(FV_TIME_METER2, 0);

out:
    fv_release_image(&curr_img);
    fv_release_image(&prev_img);

    for (i = 0; i < corner_count; i++) {
        corners_a[i].y = height - 1 - corners_a[i].y;
        corners_b[i].y = height - 1 - corners_b[i].y;
    }

    return corner_count;
}

static fv_s32 
_fv_cv_lk_optical_flow_image(IplImage *prev, IplImage *curr, 
            CvPoint2D32f *corners_a, CvPoint2D32f *corners_b, 
            fv_s8 *features_found, float *feature_errors, 
            fv_s32 corner_max_num)
{
    fv_s32          corner_count = corner_max_num;
    
    fv_time_meter_set(FV_TIME_METER2);
    cvGoodFeaturesToTrack(prev, NULL, NULL, corners_a,
                &corner_count, FV_TRACK_QUALITY_LEVEL, FV_TRACK_MIN_DISTANCE,
                NULL, FV_TRACK_BLOCK_SIZE, FV_TRACK_USE_HARRIS, 
                FV_TRACK_HARRIS_K);
   fv_time_meter_get(FV_TIME_METER2, 0);
    if (fv_track_points) {
        fv_track_label_points(corners_b, corners_a, 
                corner_count, features_found);
        goto out;
    }

    cvFindCornerSubPix(prev, corners_a, corner_count, 
                cvSize(FV_TRACK_WIN_SIZE, FV_TRACK_WIN_SIZE), 
                cvSize(-1, -1),
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 
                    FV_TRACK_MAX_ITER, FV_TRACK_EPSISON));
 
    fv_time_meter_set(FV_TIME_METER2);
    cvCalcOpticalFlowPyrLK(prev, curr, NULL, NULL,
                corners_a, corners_b, corner_count, 
                cvSize(FV_TRACK_WIN_SIZE, FV_TRACK_WIN_SIZE), 
                FV_TRACK_PYR_LEVEL, features_found, feature_errors,
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 
                    FV_TRACK_MAX_ITER, FV_TRACK_EPSISON), 0);
    fv_time_meter_get(FV_TIME_METER2, 0);

out:
    return corner_count;
}

static fv_s32 
fv_cv_lk_optical_flow_image(IplImage *cv_img, CvPoint2D32f *corners_a, 
            CvPoint2D32f *corners_b, fv_s8 *features_found,
            float *feature_errors, fv_s32 corner_max_num, fv_bool opencv)
{
    IplImage    *img_a;
    IplImage    *img_b;
    fv_s32      ret;

    img_a = cvLoadImage("./A.png", CV_LOAD_IMAGE_GRAYSCALE);
    if (img_a == NULL) {
        FV_LOG_ERR("Load Image A failed!\n");
        return FV_ERROR;
    }

    img_b = cvLoadImage("./B.png", CV_LOAD_IMAGE_GRAYSCALE);
    if (img_b == NULL) {
        FV_LOG_ERR("Load Image B failed!\n");
        cvReleaseImage(&img_a);
        return FV_ERROR;
    }
    
    if (opencv) {
        ret = _fv_cv_lk_optical_flow_image(img_a, img_b, corners_a, corners_b,
                features_found, feature_errors, corner_max_num);
    } else {
        ret = fv_lk_optical_flow_image(img_a, img_b, corners_a, corners_b,
                features_found, feature_errors, corner_max_num);
    }

    cvReleaseImage(&img_a);
    cvReleaseImage(&img_b);

    return ret;
}

static fv_s32 
fv_cv_lk_optical_flow_video(IplImage *cv_img, CvPoint2D32f *corners_a, 
            CvPoint2D32f *corners_b, fv_s8 *features_found,
            float *feature_errors, fv_s32 corner_max_num, fv_bool opencv)
{
    IplImage    *curr;
    fv_s32      ret;

    curr = cvCreateImage(cvGetSize(cv_img), IPL_DEPTH_8U, 1);
    if (curr == NULL) {
        FV_LOG_ERR("Create image failed!\n");
        return FV_ERROR;
    }

    cvCvtColor(cv_img, curr, CV_BGR2GRAY);

    if (fv_cv_prev_img == NULL) {
        fv_cv_prev_img = curr;
        return FV_OK;
    }

    if (opencv) {
        ret = _fv_cv_lk_optical_flow_image(fv_cv_prev_img, curr, corners_a, 
                corners_b, features_found, feature_errors, corner_max_num);
    } else {
        ret = fv_lk_optical_flow_image(fv_cv_prev_img, curr, corners_a, 
                corners_b, features_found, feature_errors, corner_max_num);
    }
    cvReleaseImage(&fv_cv_prev_img);
    fv_cv_prev_img = curr;

    return ret;
}

static fv_s32 
__fv_cv_lk_optical_flow(IplImage *cv_img, fv_bool image, fv_bool opencv)
{
    CvPoint         start;
    CvPoint         end;
    fv_s32          i;
    fv_s32          corner_count = FV_CV_MAX_CORNERS;

    if (image) {
        corner_count = fv_cv_lk_optical_flow_image(cv_img, fv_cv_corners_a,
                fv_cv_corners_b, fv_cv_features_found, fv_cv_feature_errors,
                FV_CV_MAX_CORNERS, opencv);
    } else {
        corner_count = fv_cv_lk_optical_flow_video(cv_img, fv_cv_corners_a,
                fv_cv_corners_b, fv_cv_features_found, fv_cv_feature_errors,
                FV_CV_MAX_CORNERS, opencv);
    }

    for (i = 0; i < corner_count; i++) {
        if (fv_cv_features_found[i] == 0 || fv_cv_feature_errors[i] > 500) {
            FV_LOG_PRINT("Error is %f\n", fv_cv_feature_errors[i]);
            continue;
        }
        start = cvPoint(cvRound(fv_cv_corners_a[i].x), 
                    cvRound(fv_cv_corners_a[i].y));
        end = cvPoint(cvRound(fv_cv_corners_b[i].x), 
                    cvRound(fv_cv_corners_b[i].y));
        cvLine(cv_img, start, end, CV_RGB(255, 0, 0), 1, 1, 0);
    }

    return FV_OK;
}

fv_s32 
_fv_cv_lk_optical_flow(IplImage *cv_img, fv_bool image)
{
    return __fv_cv_lk_optical_flow(cv_img, image, 1);
}

fv_s32 
fv_cv_lk_optical_flow(IplImage *cv_img, fv_bool image)
{
    return __fv_cv_lk_optical_flow(cv_img, image, 0);
}

fv_s32 
_fv_cv_track_points(IplImage *cv_img, fv_bool image)
{
    fv_track_points = 1;
    return __fv_cv_lk_optical_flow(cv_img, image, 1);
}

fv_s32 
fv_cv_track_points(IplImage *cv_img, fv_bool image)
{
    fv_track_points = 1;
    return __fv_cv_lk_optical_flow(cv_img, image, 0);
}

fv_s32 
_fv_cv_sub_pix(IplImage *cv_img, fv_bool image)
{
    fv_image_t      *img;
    IplImage        *gray;
    fv_s32          i;
    fv_s32          corner_count = FV_CV_MAX_CORNERS;
    float           y;
    fv_s32          ret = FV_OK;

    FV_ASSERT(image);

    gray = cvCreateImage(cvGetSize(cv_img), IPL_DEPTH_8U, 1);
    if (gray == NULL) {
        FV_LOG_ERR("Create image failed!\n");
        return FV_ERROR;
    }

    cvCvtColor(cv_img, gray, CV_BGR2GRAY);
    cvGoodFeaturesToTrack(gray, NULL, NULL, fv_cv_corners_a,
                &corner_count, FV_TRACK_QUALITY_LEVEL, FV_TRACK_MIN_DISTANCE,
                NULL, FV_TRACK_BLOCK_SIZE, FV_TRACK_USE_HARRIS, 
                FV_TRACK_HARRIS_K);

    memcpy(fv_cv_corners_b, fv_cv_corners_a, 
            sizeof(*fv_cv_corners_b)*corner_count);

    cvFindCornerSubPix(gray, fv_cv_corners_b, corner_count, 
                cvSize(FV_TRACK_WIN_SIZE, FV_TRACK_WIN_SIZE), 
                cvSize(-1, -1),
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 
                    FV_TRACK_MAX_ITER, FV_TRACK_EPSISON));

    for (i = 0; i < corner_count; i++) {
        fv_cv_corners_c[i] = fv_cv_corners_a[i];
        fv_cv_corners_c[i].y = cv_img->height - 1 - fv_cv_corners_a[i].y;
    }
    img = fv_convert_image(gray);
    if (img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    cvReleaseImage(&gray);

    fv_find_corner_sub_pix(img, (fv_point_2D32f_t *)fv_cv_corners_c, 
            corner_count, fv_size(FV_TRACK_WIN_SIZE, FV_TRACK_WIN_SIZE), 
            fv_size(-1, -1), fv_term_criteria(FV_TERMCRIT_ITER|FV_TERMCRIT_EPS,
                FV_TRACK_MAX_ITER, FV_TRACK_EPSISON));

    for (i = 0; i < corner_count; i++) {
        y = cv_img->height - 1 - fv_cv_corners_c[i].y;
        if (fabs(fv_cv_corners_b[i].x - fv_cv_corners_c[i].x) > 
                FV_TRACK_EPSISON ||
                fabs(fv_cv_corners_b[i].y - y) > FV_TRACK_EPSISON) {
            fprintf(stdout, "a[%d]=[%f, %f], b=[%f, %f], c=[%f, %f]\n", i,
                    fv_cv_corners_a[i].x, fv_cv_corners_a[i].y,
                    fv_cv_corners_b[i].x, fv_cv_corners_b[i].y,
                    fv_cv_corners_c[i].x, y);
            ret = FV_ERROR;
            break;
        }
    }

    fv_release_image(&img);

    return ret;
}

fv_s32 
fv_cv_sub_pix(IplImage *cv_img, fv_bool image)
{
    return _fv_cv_sub_pix(cv_img, image);
}
