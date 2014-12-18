#include "fv_types.h"
#include "fv_core.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_edge.h"
#include "fv_debug.h"
#include "fv_time.h"

#define FV_SOBEL_WIN_NAME   "sobel"
#define FV_SOBEL_X_ORDER    1
#define FV_SOBEL_Y_ORDER    0
#define FV_SOBEL_K_SIZE     3

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
    IplImage    *gray;
    IplImage    *sobel;
    fv_s32      c;

    FV_ASSERT(image);
    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    sobel = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(sobel != NULL);

    cvCvtColor(cv_img, gray, CV_BGR2GRAY);


    cvNamedWindow(FV_SOBEL_WIN_NAME, 0);  
    _fv_cv_sobel_mine(sobel, gray, FV_SOBEL_X_ORDER, 
            FV_SOBEL_Y_ORDER, FV_SOBEL_K_SIZE, 0);
    cvShowImage(FV_SOBEL_WIN_NAME, sobel);  
    c = cvWaitKey(0);  
    printf("c = %d\n", c);
    _fv_cv_sobel_mine(sobel, gray, FV_SOBEL_X_ORDER, 
            FV_SOBEL_Y_ORDER, FV_SOBEL_K_SIZE, 1);
    cvShowImage(FV_SOBEL_WIN_NAME, sobel);  
    c = cvWaitKey(0);  
    cvDestroyWindow(FV_SOBEL_WIN_NAME); 

    cvReleaseImage(&sobel);
    cvReleaseImage(&gray);

    return FV_OK;
}
