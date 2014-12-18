#include <stdio.h>

#include "fv_types.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_debug.h"

#define FV_KEY_ESC              27
#define FV_KEY_ENTER            13
#define FV_KEY_T                116

#if 0
static void
fv_cv_rectangle_color(IplImage *cv_img, fv_rectangle_t *rect,
            fv_u8 r, fv_u8 g, fv_u8 b)
{
    CvPoint             pt1;
    CvPoint             pt2;

    pt1.x = rect->rt_left;
    pt1.y = cv_img->height - rect->rt_top;
    pt2.x = rect->rt_right;
    pt2.y = cv_img->height - rect->rt_down;

    /* 
     * Draw an white rectangle to mark the location 
     * of the vechicle 
     */
    cvRectangle(cv_img, pt1, pt2, cvScalar(b, g, r, 0), 
            1, 1, 0);
}

static void
fv_cv_rectangle(IplImage *cv_img, fv_rectangle_t *rect)
{
    fv_cv_rectangle_color(cv_img, rect, 0, 0, 255);
}

static void
fv_cv_line(IplImage *cv_img, fv_line_t *line)
{
    CvPoint             pt1;
    CvPoint             pt2;

    pt1.x = line->ln_point[0].pt_x;
    pt1.y = cv_img->height - line->ln_point[0].pt_y;
    pt2.x = line->ln_point[1].pt_x;
    pt2.y = cv_img->height - line->ln_point[1].pt_y;

    cvLine(cv_img, pt1, pt2, cvScalar(0, 255, 0, 0), 
            1, 1, 0);
}
#endif

fv_s32
fv_cv_detect_img(char *im_file, fv_proc_func proc)
{
    IplImage            *cv_img;
    char                *win_name = "Image window";
    char                c;
    fv_s32              ret;

    cv_img = cvLoadImage(im_file, 
            CV_LOAD_IMAGE_ANYDEPTH|CV_LOAD_IMAGE_ANYCOLOR);
    if (cv_img == NULL) {
        FV_LOG_ERR("Can't load Image file %s!\n", im_file);
        return FV_ERROR;
    }

    ret = proc(cv_img, 1);

    cvNamedWindow(win_name, 0);  
    cvShowImage(win_name, cv_img);  
    c = cvWaitKey(0);  
    printf("c = %d\n", c);
    cvDestroyWindow(win_name); 

    cvReleaseImage(&cv_img);

    return ret;
}

fv_s32
fv_cv_detect_video(char *vd_file, fv_proc_func proc)
{
    IplImage            *cv_img;
    char                *win_name = "Video window";
    CvCapture           *capture;
    char                key;
    fv_s32              ret = FV_OK;

    if (vd_file != NULL) {
        capture = cvCreateFileCapture(vd_file);
    } else {
        capture = cvCreateCameraCapture(-1);
    }

    if (capture == NULL) {
        FV_LOG_ERR("Can't create capture for file %s!\n", vd_file);
        return FV_ERROR;
    }

    cvNamedWindow(win_name, 0);  
    while (1) {
        cv_img = cvQueryFrame(capture);
        if (cv_img == NULL) {
            break;
        }

        ret = proc(cv_img, 0);
        if (ret != FV_OK) {
            break;
        }
        cvShowImage(win_name, cv_img);  
        key = cvWaitKey(33);  
        if (key == FV_KEY_ESC) { //Press Esc to quit
            break;
        }

        if (key == FV_KEY_ENTER) { //Press Enter to start or stop debug mode 
        }
    }

    cvDestroyWindow(win_name);  
    cvReleaseCapture(&capture);

    return ret;
}

fv_s32
fv_cv_detect_camera(char *vd_file, fv_proc_func identify)
{
    return fv_cv_detect_video(NULL, identify);
}

fv_s32 
fv_cv_save_img(char *file_name, fv_mat_t *mat)
{
    IplImage    *cv_img; 
    fv_u8       *dst_data;
    fv_u8       *src_data;
    CvSize      size;
    fv_s32      y;
    fv_s32      w;

    size = cvSize(mat->mt_cols, mat->mt_rows);
    cv_img = cvCreateImage(size, FV_MAT_DEPTH(mat), FV_MAT_NCHANNEL(mat));
    if (cv_img == NULL) {
        FV_LOG_ERR("Create cv img failed!\n");
    }

    w = cv_img->widthStep;
    for (y = 0; y < mat->mt_rows; y++) {
        dst_data = (fv_u8 *)cv_img->imageData + y*w;
        src_data = mat->mt_data.dt_ptr + (mat->mt_rows - y - 1)*mat->mt_step;
        memcpy(dst_data, src_data, w);
    }

    cvSaveImage(file_name, cv_img, NULL);
    cvReleaseImage(&cv_img);

    return FV_OK;
}

void
fv_cv_img_to_ipl(IplImage *cv_img, fv_image_t *img)
{
    fv_u32      i;

    FV_ASSERT(img->ig_width == cv_img->width && 
            img->ig_height == cv_img->height && 
            img->ig_width_step == cv_img->widthStep);

    for (i = 0; i < cv_img->height; i++) {
        memcpy(cv_img->imageData + i*cv_img->widthStep, 
                img->ig_image_data + 
                (img->ig_height - i - 1)*img->ig_width_step,
                cv_img->widthStep);
    }
}

