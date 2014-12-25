#include "fv_types.h"
#include "fv_core.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_edge.h"
#include "fv_debug.h"
#include "fv_time.h"

#define FV_SOBEL_WIN_NAME   "sobel"
#define FV_LAPLACE_WIN_NAME "laplace"
#define FV_SOBEL_X_ORDER    1
#define FV_SOBEL_Y_ORDER    0
#define FV_SOBEL_K_SIZE     3
#define FV_LAPLACE_K_SIZE   5
#define FV_CANNY_K_SIZE     3
#define FV_CANNY_THRESHOLD1 50
#define FV_CANNY_THRESHOLD2 10

static void 
_fv_cv_sobel_mine(IplImage *cv_sobel, IplImage *gray, 
                fv_s32 dx, fv_s32 dy, 
                fv_s32 aperture_size, fv_bool opencv)
{
    fv_image_t      *img;
    fv_image_t      *sobel;
    fv_size_t       size;

    if (opencv) {
        fv_time_meter_set(0);
        cvSobel(gray, cv_sobel, dx, dy, aperture_size);
        fv_time_meter_get(0, 0);
        return;
    }

    img = fv_convert_image(gray);
    if (img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    size = fv_get_size(img);
    sobel = fv_create_image(size, cv_sobel->depth, cv_sobel->nChannels);
    if (sobel == NULL) {
        FV_LOG_ERR("Alloc image faield!\n");
    }

    fv_time_meter_set(0);
    fv_sobel(sobel, img, dx, dy, aperture_size);
    fv_time_meter_get(0, 0);

    fv_cv_img_to_ipl(cv_sobel, sobel);

    fv_release_image(&sobel);
    fv_release_image(&img);
}

fv_s32 
fv_cv_sobel(IplImage *cv_img, fv_bool image)
{
    IplImage    *sobel;
    fv_s32      c;

    FV_ASSERT(image);
    sobel = cvCreateImage(cvGetSize(cv_img), cv_img->depth, cv_img->nChannels);
    FV_ASSERT(sobel != NULL);

    cvNamedWindow(FV_SOBEL_WIN_NAME, 0);  
    _fv_cv_sobel_mine(sobel, cv_img, FV_SOBEL_X_ORDER, 
            FV_SOBEL_Y_ORDER, FV_SOBEL_K_SIZE, 0);
    cvShowImage(FV_SOBEL_WIN_NAME, sobel);  
    c = cvWaitKey(0);  
    printf("c = %d\n", c);
    _fv_cv_sobel_mine(sobel, cv_img, FV_SOBEL_X_ORDER, 
            FV_SOBEL_Y_ORDER, FV_SOBEL_K_SIZE, 1);
    cvShowImage(FV_SOBEL_WIN_NAME, sobel);  
    c = cvWaitKey(0);  
    cvDestroyWindow(FV_SOBEL_WIN_NAME); 

    cvReleaseImage(&sobel);

    return FV_OK;
}

fv_s32 
fv_cv_laplace(IplImage *cv_img, fv_bool image)
{
    IplImage        *laplace;
    fv_image_t      *img;
    fv_image_t      *lap;
    fv_size_t       size;
    fv_s32          c;

    FV_ASSERT(image);
    laplace = cvCreateImage(cvGetSize(cv_img), cv_img->depth,
            cv_img->nChannels);
    FV_ASSERT(laplace != NULL);

    img = fv_convert_image(cv_img);
    if (img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    size = fv_get_size(img);
    lap = fv_create_image(size, cv_img->depth, cv_img->nChannels);
    if (lap == NULL) {
        FV_LOG_ERR("Alloc image faield!\n");
    }

    fv_time_meter_set(0);
    fv_laplace(lap, img, FV_LAPLACE_K_SIZE);
    fv_time_meter_get(0, 0);

    fv_cv_img_to_ipl(laplace, lap);
    fv_release_image(&lap);

    cvNamedWindow(FV_LAPLACE_WIN_NAME, 0);
    cvShowImage(FV_LAPLACE_WIN_NAME, laplace);
    c = cvWaitKey(0);
    printf("c = %d\n", c);
    fv_time_meter_set(0);
    cvLaplace(cv_img, laplace, FV_LAPLACE_K_SIZE);
    fv_time_meter_get(0, 0);
    cvShowImage(FV_LAPLACE_WIN_NAME, laplace);
    c = cvWaitKey(0);
    cvDestroyWindow(FV_LAPLACE_WIN_NAME);

    cvReleaseImage(&laplace);

    return FV_OK;
}

fv_s32 
fv_cv_canny(IplImage *cv_img, fv_bool image)
{
    char            *win_name = "canny";
    IplImage        *canny;
    IplImage        *gray;
    fv_image_t      *img;
    fv_image_t      *cy;
    fv_size_t       size;
    fv_s32          c;

    FV_ASSERT(image);
    printf("depth = %d\n", cv_img->depth);

    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    canny = cvCreateImage(cvGetSize(gray), gray->depth,
            gray->nChannels);
    FV_ASSERT(canny != NULL);

    img = fv_convert_image(gray);
    if (img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    size = fv_get_size(img);
    cy = fv_create_image(size, canny->depth, canny->nChannels);
    if (cy == NULL) {
        FV_LOG_ERR("Alloc image faield!\n");
    }

    fv_time_meter_set(0);
    fv_canny(cy, img, FV_CANNY_THRESHOLD1, FV_CANNY_THRESHOLD2,
            FV_CANNY_K_SIZE);
    fv_time_meter_get(0, 0);

    fv_cv_img_to_ipl(canny, cy);
    fv_release_image(&cy);

    cvNamedWindow(win_name, 0);
    cvShowImage(win_name, canny);
    c = cvWaitKey(0);
    printf("c = %d\n", c);
    fv_time_meter_set(0);
    cvCanny(gray, canny, FV_CANNY_THRESHOLD1, 
            FV_CANNY_THRESHOLD2, FV_CANNY_K_SIZE);
    fv_time_meter_get(0, 0);
    cvShowImage(win_name, canny);
    c = cvWaitKey(0);
    cvDestroyWindow(win_name);

    cvReleaseImage(&gray);
    cvReleaseImage(&canny);

    return FV_OK;
}
