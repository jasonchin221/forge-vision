
#include "fv_types.h"
#include "fv_core.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_debug.h"
#include "fv_time.h"
#include "fv_math.h"
#include "fv_hough.h"
#include "fv_mem.h"

#define FV_HOUGH_WIN_NAME       "hough"
#define FV_HOUGH_LINE_NUM       2000
#define FV_HOUGH_RHO            1
#define FV_HOUGH_THRESHOLD      150
#define FV_HOUGH_PROB_THRESHOLD 15
#define FV_HOUGH_LINE_LENGTH    30
#define FV_HOUGH_LINE_GAP       10
#define FV_HOUGH_DP             2
#define FV_HOUGH_PARAM1         300
#define FV_HOUGH_PARAM2         100 
#define FV_HOUGH_MIN_RADIUS     0
#define FV_HOUGH_MAX_RADIUS     200
#define FV_CANNY_K_SIZE         3
#define FV_CANNY_THRESHOLD1     50
#define FV_CANNY_THRESHOLD2     200

static void
fv_cv_hough_draw_lines2(IplImage *bina, CvSeq *lines)
{
    fv_line_polar_t     *line;
    CvPoint             pt1;
    CvPoint             pt2;
    float               rho;
    float               theta;
    double              a;
    double              b;
    fv_s32              c;
    fv_s32              height;

    height = bina->height;
    for (c = 0; c < lines->total; c++) {
        line = (fv_line_polar_t *)cvGetSeqElem(lines, c);
        rho = line->lp_rho;
        theta = line->lp_angle;
        a = cos(theta);
        b = sin(theta);
        if (fv_float_is_zero(a)) { //pi/2
            pt1.y = pt2.y = cvRound(rho);
            pt1.x = 0;
            pt2.x = bina->width - 1;
        } else if (fv_float_is_zero(b)) { //0
            pt1.x = pt2.x = cvRound(rho);
            pt1.y = 0;
            pt2.y = height;
        } else {
            pt1.x = 0;
            pt1.y = cvRound(rho/b);
            pt2.x = cvRound(rho/a);
            pt2.y = 0;
        }
        cvLine(bina, pt1, pt2, CV_RGB(255, 0, 0), 2, 1, 0);
    }
}


static void
fv_cv_hough_draw_lines(IplImage *bina, void *buf, fv_s32 num)
{
    fv_line_polar_t     *line;
    CvPoint             *p;
    CvPoint             pt1;
    CvPoint             pt2;
    CvPoint             pt;
    float               rho;
    float               theta;
    double              a;
    double              b;
    fv_s32              c;
    fv_s32              height;
    fv_s32              width;

    width = bina->width;
    height = bina->height;
    line = buf;
    for (c = 0; c < num; c++, line++) {
        rho = line->lp_rho;
        theta = line->lp_angle;
        a = cos(theta);
        b = sin(theta);
        if (fv_float_is_zero(a)) { //pi/2
            pt1.y = pt2.y = cvRound(rho);
            pt1.x = 0;
            pt2.x = bina->width - 1;
        } else if (fv_float_is_zero(b)) { //0
            pt1.x = pt2.x = cvRound(rho);
            pt1.y = 0;
            pt2.y = height;
        } else {
            pt.x = rho/a;
            pt.y = 0;
            p = &pt1;
            if (pt.x >= 0 && pt.x < width) {
                *p = pt;
                p = &pt2;
            }
            pt.x = 0;
            pt.y = rho/b;
            if (pt.y >= 0 && pt.y < height) {
                *p = pt;
                p = &pt2;
            }
            pt.x = width - 1;
            pt.y = (rho - a*pt.x)/b;
            if (pt.y >= 0 && pt.y < height) {
                *p = pt;
                p = &pt2;
            }
            pt.y = height - 1;
            pt.x = (rho - b*pt.y)/a;
            if (pt.x >= 0 && pt.x < width) {
                *p = pt;
                p = &pt2;
            }
        }
        pt1.y = height - 1 - pt1.y;
        pt2.y = height - 1 - pt2.y;
        cvLine(bina, pt1, pt2, CV_RGB(255, 0, 0), 2, 1, 0);
    }
}

static void
fv_cv_hough_draw_lines_prob(IplImage *dst, CvSeq *lines)
{
    CvPoint             *line;
    fv_s32              c;

    printf("num = %d\n", lines->total);
    for (c = 0; c < lines->total; c++) {
        line = (CvPoint*)cvGetSeqElem(lines, c);
        cvLine(dst, line[0], line[1], CV_RGB(255,0,0), 3, 8, 0);
    }
}
 
static void
fv_cv_hough_draw_lines_prob2(IplImage *dst, void *buf, fv_s32 num)
{
    CvPoint             *pt;
    fv_s32              c;

    pt = buf;
    printf("num2 = %d\n", num);
    for (c = 0; c < num; c++, pt += 2) {
        pt[0].y = dst->height - 1 - pt[0].y;
        pt[1].y = dst->height - 1 - pt[1].y;
        cvLine(dst, pt[0], pt[1], CV_RGB(255, 0, 0), 3, 8, 0);
    }
}

static void
fv_cv_hough_draw_circle(IplImage *dst, void *buf, fv_s32 num)
{
    fv_circle_t         *cl;
    CvPoint             pt;
    fv_s32              c;

    cl = buf;
    for (c = 0; c < num; c++, cl++) {
        pt.x = cl->cl_center.pt_x;
        pt.y = dst->height - 1 - cl->cl_center.pt_y;
        printf("circle %d, center[%d %d], r = %f\n", c, pt.x, pt.y, cl->cl_radius);
        cvCircle(dst, pt, cl->cl_radius, CV_RGB(0, 0, 255), 3, 8, 0);
    }
}
 
static void
fv_cv_hough_draw_circle2(IplImage *dst, CvSeq *circle)
{
    CvPoint             pt;
    float               *p;
    fv_s32              c;

    printf("total = %d\n", circle->total);
    for (c = 0; c < circle->total; c++) {
        p = (float *)cvGetSeqElem(circle, c);
        pt.x = cvRound(p[0]);
        pt.y = cvRound(p[1]);
        printf("circle %d, center[%d %d], r = %d\n", c, pt.x, pt.y, cvRound(p[2]));
        cvCircle(dst, pt, cvRound(p[2]), CV_RGB(0, 0, 255), 3, 8, 0);
    }
}
 
fv_s32 
fv_cv_hough(IplImage *cv_img, fv_bool image)
{
    CvMemStorage        *storage = NULL;  
    void                *mem;
    IplImage            *gray;
    IplImage            *bina;
    IplImage            *dst;
    IplImage            *prob;
    IplImage            *prob2;
    IplImage            *circle;
    IplImage            *circle2;
    CvSeq               *lines;
    fv_image_t          *src;
    fv_image_t          *_src;
    fv_s32              num;
    fv_s32              c;
    fv_s32              min_dist;

    FV_ASSERT(image);
    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    bina = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(bina != NULL);

    dst = cvCloneImage(cv_img);
    FV_ASSERT(dst != NULL);

    prob = cvCloneImage(cv_img);
    FV_ASSERT(prob != NULL);

    prob2 = cvCloneImage(cv_img);
    FV_ASSERT(prob2 != NULL);

    circle = cvCloneImage(cv_img);
    FV_ASSERT(circle != NULL);

    circle2 = cvCloneImage(cv_img);
    FV_ASSERT(circle2 != NULL);

    cvCanny(gray, bina, FV_CANNY_THRESHOLD1, 
            FV_CANNY_THRESHOLD2, FV_CANNY_K_SIZE);

    cvNamedWindow(FV_HOUGH_WIN_NAME, 0);  
    cvShowImage(FV_HOUGH_WIN_NAME, bina);  

    c = cvWaitKey(0);  
    src = fv_convert_image(bina);
    FV_ASSERT(src != NULL);

    num = FV_HOUGH_LINE_NUM*sizeof(fv_line_t);
    mem = fv_alloc(num);
    FV_ASSERT(mem != NULL);
    fv_time_meter_set(FV_TIME_METER1);
    c = fv_hough_lines(src, mem, num, FV_HOUGH_STANDARD, 
            FV_HOUGH_RHO, fv_pi/180, FV_HOUGH_THRESHOLD, 0, 0);
    fv_time_meter_get(FV_TIME_METER1, 0);
    FV_ASSERT(c >= 0);
    fv_cv_hough_draw_lines(cv_img, mem, c);
    cvShowImage(FV_HOUGH_WIN_NAME, cv_img);  
    c = cvWaitKey(0);  
    storage = cvCreateMemStorage(0);
    fv_time_meter_set(FV_TIME_METER1);
    lines = cvHoughLines2(bina, storage, CV_HOUGH_STANDARD, FV_HOUGH_RHO,
            CV_PI/180, FV_HOUGH_THRESHOLD, 0, 0);
    fv_time_meter_get(FV_TIME_METER1, 0);
    fv_cv_hough_draw_lines2(dst, lines);
    cvShowImage(FV_HOUGH_WIN_NAME, dst);  
    c = cvWaitKey(0);  

    fv_time_meter_set(FV_TIME_METER1);
    c = fv_hough_lines(src, mem, num/2, FV_HOUGH_PROBABILISTIC, 
            FV_HOUGH_RHO, fv_pi/180, FV_HOUGH_PROB_THRESHOLD,
            FV_HOUGH_LINE_LENGTH, FV_HOUGH_LINE_GAP);
    fv_time_meter_get(FV_TIME_METER1, 0);
    FV_ASSERT(c >= 0);
    fv_cv_hough_draw_lines_prob2(prob2, mem, c);
    cvShowImage(FV_HOUGH_WIN_NAME, prob2);  
    c = cvWaitKey(0);  
 
    fv_time_meter_set(FV_TIME_METER1);
    lines = cvHoughLines2(bina, storage, CV_HOUGH_PROBABILISTIC, FV_HOUGH_RHO,
            CV_PI/180, FV_HOUGH_PROB_THRESHOLD, 
            FV_HOUGH_LINE_LENGTH, FV_HOUGH_LINE_GAP);
    fv_time_meter_get(FV_TIME_METER1, 0);
    fv_cv_hough_draw_lines_prob(prob, lines);
    cvShowImage(FV_HOUGH_WIN_NAME, prob);  
    c = cvWaitKey(0);  

    min_dist = gray->width/3;

    _src = fv_convert_image(gray);
    FV_ASSERT(_src != NULL);
    fv_time_meter_set(FV_TIME_METER1);
    c = fv_hough_circles(_src, mem, num, FV_HOUGH_GRADIENT,
                FV_HOUGH_DP, min_dist, FV_HOUGH_PARAM1,
                FV_HOUGH_PARAM2, FV_HOUGH_MIN_RADIUS, FV_HOUGH_MAX_RADIUS);
    fv_time_meter_get(FV_TIME_METER1, 0);
    fv_cv_hough_draw_circle(circle, mem, c);
    cvShowImage(FV_HOUGH_WIN_NAME, circle);  
    c = cvWaitKey(0);  

    fv_time_meter_set(FV_TIME_METER1);
    lines = cvHoughCircles(gray, storage, CV_HOUGH_GRADIENT, 
            FV_HOUGH_DP, min_dist, FV_HOUGH_PARAM1,
            FV_HOUGH_PARAM2, FV_HOUGH_MIN_RADIUS, FV_HOUGH_MAX_RADIUS);
    fv_time_meter_get(FV_TIME_METER1, 0);

    fv_cv_hough_draw_circle2(circle2, lines);
    cvShowImage(FV_HOUGH_WIN_NAME, circle2);  
    c = cvWaitKey(0);  

    cvDestroyWindow(FV_HOUGH_WIN_NAME); 

    fv_free(&mem);
    cvReleaseImage(&gray);
    cvReleaseImage(&circle2);
    cvReleaseImage(&circle);
    fv_release_image(&_src);
    fv_release_image(&src);
    cvReleaseImage(&prob2);
    cvReleaseImage(&prob);
    cvReleaseImage(&dst);
    cvReleaseMemStorage(&storage);
    cvReleaseImage(&bina);

    return FV_OK;
}
