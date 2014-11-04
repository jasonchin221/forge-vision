#include "fv_types.h"
#include "fv_opencv.h"
#include "fv_log.h"

#define FV_CV_MAX_CORNERS   500

static IplImage *fv_cv_prev_img;
static CvPoint2D32f fv_cv_corners_a[FV_CV_MAX_CORNERS];
static CvPoint2D32f fv_cv_corners_b[FV_CV_MAX_CORNERS];
static fv_s8 fv_cv_features_found[FV_CV_MAX_CORNERS];
static float fv_cv_feature_errors[FV_CV_MAX_CORNERS];

static fv_s32 
_fv_cv_lk_optical_flow_image(IplImage *prev, IplImage *curr)
{
    IplImage        *eig_img;
    IplImage        *tmp_img;
    IplImage        *pyr_prev;
    IplImage        *pyr_curr;
    CvSize          img_sz;
    CvSize          pyr_sz;
    fv_u8           win_size = 5;
    fv_s32          corner_count = FV_CV_MAX_CORNERS;
    
    img_sz = cvGetSize(prev);
    eig_img = cvCreateImage(img_sz, IPL_DEPTH_8U, 1);
    if (eig_img == NULL) {
        FV_LOG_ERR("Create image failed!\n");
    }

    tmp_img = cvCreateImage(img_sz, IPL_DEPTH_8U, 1);
    if (tmp_img == NULL) {
        FV_LOG_ERR("Create image failed!\n");
    }

    cvGoodFeaturesToTrack(prev, eig_img, tmp_img, fv_cv_corners_a,
                &corner_count, 0.01, 5.0, 0, 3, 0, 0.04);
    cvFindCornerSubPix(prev, fv_cv_corners_a, corner_count, 
                cvSize(win_size, win_size), cvSize(-1, -1),
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03));
    pyr_sz = cvSize(prev->width + 8, curr->height/3);
    pyr_prev = cvCreateImage(pyr_sz, IPL_DEPTH_8U, 1);
    if (pyr_prev == NULL) {
        FV_LOG_ERR("Create image failed!\n");
    }

    pyr_curr = cvCreateImage(pyr_sz, IPL_DEPTH_8U, 1);
    if (pyr_curr == NULL) {
        FV_LOG_ERR("Create image failed!\n");
    }

    cvCalcOpticalFlowPyrLK(prev, curr, pyr_prev, pyr_curr,
                fv_cv_corners_a, fv_cv_corners_b, corner_count, 
                cvSize(win_size, win_size), 5, fv_cv_features_found,
                fv_cv_feature_errors,
                cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03), 0);
    cvReleaseImage(&pyr_curr);
    cvReleaseImage(&pyr_prev);
    cvReleaseImage(&tmp_img);
    cvReleaseImage(&eig_img);

    return corner_count;
}

static fv_s32 
fv_cv_lk_optical_flow_image(IplImage *cv_img)
{
    IplImage    *img_a;
    IplImage    *img_b;
    fv_s32      ret;

    printf("%s %d\n", __FUNCTION__, __LINE__);
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
    ret = _fv_cv_lk_optical_flow_image(img_a, img_b);

    cvReleaseImage(&img_a);
    cvReleaseImage(&img_b);

    return ret;
}

static fv_s32 
fv_cv_lk_optical_flow_video(IplImage *cv_img)
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

    ret = _fv_cv_lk_optical_flow_image(fv_cv_prev_img, curr);
    cvReleaseImage(&curr);
    cvReleaseImage(&fv_cv_prev_img);

    return ret;
}

fv_s32 
fv_cv_lk_optical_flow(IplImage *cv_img, fv_bool image)
{
    CvPoint         start;
    CvPoint         end;
    fv_s32          i;
    fv_s32          corner_count = FV_CV_MAX_CORNERS;

    if (image) {
        corner_count = fv_cv_lk_optical_flow_image(cv_img);
    } else {
        corner_count = fv_cv_lk_optical_flow_video(cv_img);
    }

    if (corner_count < 0) {
        return FV_ERROR;
    }

    for (i = 0; i < corner_count; i++) {
        if (fv_cv_features_found[i] == 0 || fv_cv_feature_errors[i] > 500) {
            FV_LOG_PRINT("Error is %f\n", fv_cv_feature_errors[i]);
            continue;
        }
        if (abs(fv_cv_corners_a[i].x - fv_cv_corners_b[i].x) > 140 || 
                abs(fv_cv_corners_a[i].y - fv_cv_corners_b[i].y) > 140) {
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


