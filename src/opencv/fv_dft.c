
#include "fv_types.h"
#include "fv_core.h"
#include "fv_opencv.h"
#include "fv_log.h"
#include "fv_debug.h"
#include "fv_time.h"
#include "fv_convert.h"
#include "fv_dxt.h"
#include "fv_math.h"

float fv_sin(float theta)
{
    return sin(theta);
}

float fv_cos(float theta)
{
    return cos(theta);
}

static void
fv_zero(fv_mat_t *mat)
{
    fv_u8       *data;
    fv_s32      step;
    fv_s32      i;

    if (mat->mt_cols == 0 || mat->mt_rows == 0) {
        return;
    }

    step = mat->mt_step;
    data = mat->mt_data.dt_ptr;
    for (i = 0; i < mat->mt_rows; i++, data += step) {
        memset(data, 0, mat->mt_cols*FV_MAT_ELEM_SIZE(mat));
    }
}

static void
fv_speedy_preprocess(fv_mat_t *dst, fv_mat_t *dft, fv_mat_t *src, fv_mat_t *tmp)
{
    fv_get_sub_rect(tmp, dft, fv_rect(0, 0, src->mt_cols, src->mt_rows));
    fv_copy_mat(tmp, src);
    fv_get_sub_rect(tmp, dft, fv_rect(src->mt_cols, 0,
                dft->mt_cols - src->mt_cols, src->mt_rows));
    fv_zero(tmp);
    fv_get_sub_rect(tmp, dft, fv_rect(0, src->mt_rows,
                dft->mt_cols, dft->mt_rows - src->mt_rows));
    fv_zero(tmp);
    fv_dft(dst, dft, FV_DXT_FORWARD, dft->mt_rows);
}

static void
fv_speedy_convolution(fv_mat_t *dst, fv_mat_t *src, fv_mat_t *kernel)
{
    fv_mat_t    *dft_a;
    fv_mat_t    *dft_b;
    fv_mat_t    *dft_c;
    fv_mat_t    *dft;
    fv_mat_t    tmp;
    fv_s32      dft_m;
    fv_s32      dft_n;
    fv_s32      row;
    fv_s32      col;

    row = fv_max(src->mt_rows, kernel->mt_rows);
    col = fv_max(src->mt_cols, kernel->mt_cols);
    dft_m = fv_get_optimal_dft_size(row);
    dft_n = fv_get_optimal_dft_size(col);

    dft_a = fv_create_mat(dft_m, dft_n, FV_32FC2);
    FV_ASSERT(dft_a != NULL);
    dft_b = fv_create_mat(dft_m, dft_n, FV_32FC2);
    FV_ASSERT(dft_b != NULL);
    dft = fv_create_mat(dft_m, dft_n, FV_32FC1);
    FV_ASSERT(dft != NULL);
    dft_c = fv_create_mat(dft_m, dft_n, FV_32FC1);
    FV_ASSERT(dft_c != NULL);

    fv_speedy_preprocess(dft_a, dft, src, &tmp);
    fv_speedy_preprocess(dft_b, dft, kernel, &tmp);

    fv_mul_spectrums(dft_a, dft_b, dft_a);
    fv_dft(dft_c, dft_a, FV_DXT_INVERSE, src->mt_rows);

    fv_get_sub_rect(&tmp, dft_c, fv_rect(0, 0, dst->mt_cols, dst->mt_rows));
    fv_copy_mat(dst, &tmp);

    fv_release_mat(&dft);
    fv_release_mat(&dft_c);
    fv_release_mat(&dft_b);
    fv_release_mat(&dft_a);
}

static void
fv_cv_speedy_preprocess(CvMat *dft, CvMat *src, CvMat *tmp)
{
    cvGetSubRect(dft, tmp, cvRect(0, 0, src->cols, src->rows));
    cvCopy(src, tmp, NULL);
    cvGetSubRect(dft, tmp, cvRect(src->cols, 0,
                dft->cols - src->cols, src->rows));
    cvZero(tmp);
    cvDFT(dft, dft, CV_DXT_FORWARD, src->rows);
}

static void
fv_cv_speedy_convolution(CvMat *dst, CvMat *src, CvMat *kernel)
{
    CvMat       *dft_a;
    CvMat       *dft_b;
    CvMat       tmp;
    fv_s32      dft_m;
    fv_s32      dft_n;

    dft_m = cvGetOptimalDFTSize(src->rows + kernel->rows - 1);
    dft_n = cvGetOptimalDFTSize(src->cols + kernel->cols - 1);

    dft_a = cvCreateMat(dft_m, dft_n, src->type);
    dft_b = cvCreateMat(dft_m, dft_n, kernel->type);
    fv_cv_speedy_preprocess(dft_a, src, &tmp);
    fv_cv_speedy_preprocess(dft_b, kernel, &tmp);

    cvMulSpectrums(dft_a, dft_b, dft_a, 0);
    cvDFT(dft_a, dft_a, CV_DXT_INV_SCALE, dst->rows);
    cvGetSubRect(dft_a, &tmp, cvRect(0, 0, dst->cols, dst->rows));
    cvCopy(&tmp, dst, NULL);
    cvReleaseMat(&dft_a);
    cvReleaseMat(&dft_b);
}

#define fv_dft_rows 3
#define fv_dft_cols 3
fv_s32 
fv_cv_dft(IplImage *cv_img, fv_bool image)
{
    IplImage        *gray;
    IplImage        *D;
    CvMat           *A;
    CvMat           *B;
    CvMat           *C;
    fv_image_t      *img;
    fv_image_t      *dst;
    char            *win_name = "dft";
    fv_mat_t        _dst;
    fv_mat_t        *src;
    fv_mat_t        *d;
    fv_mat_t        mat;
    fv_mat_t        kernel;
    float           filter[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
    fv_s32          c;

    FV_ASSERT(image);

    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    A = cvCreateMat(gray->height, gray->width, CV_32FC1);
    FV_ASSERT(A != NULL);
    cvConvert(gray, A);

    B = cvCreateMat(fv_dft_rows, fv_dft_cols, CV_32FC1);
    FV_ASSERT(B != NULL);

    memcpy(B->data.ptr, filter, B->rows*B->step);
    C = cvCreateMat(cv_img->height + B->rows - 1, cv_img->width + B->cols - 1, 
            CV_32FC1);
    FV_ASSERT(C != NULL);

    D = cvCreateImage(cvGetSize(gray), gray->depth, 1);
    FV_ASSERT(D != NULL);

    img = fv_convert_image(gray);
    if (img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    cvReleaseImage(&gray);
    mat = fv_image_to_mat(img);
    kernel = fv_mat(fv_dft_rows, fv_dft_cols, FV_32FC1, filter);
    src = fv_create_mat(mat.mt_rows, mat.mt_cols, FV_32FC1);
    FV_ASSERT(src != NULL);

    d = fv_create_mat(mat.mt_rows, mat.mt_cols, FV_32FC1);
    FV_ASSERT(d != NULL);

    dst = fv_convert_image(D);
    if (dst == NULL) {
        FV_LOG_ERR("Alloc image faield!\n");
    }

    _dst = fv_image_to_mat(dst);
    _fv_convert(src, &mat);
    fv_time_meter_set(0);
    fv_speedy_convolution(d, src, &kernel);
    fv_time_meter_get(0, 0);

    _fv_convert(&_dst, d);

    fv_cv_img_to_ipl(D, dst);
    cvNamedWindow(win_name, 0);
    cvShowImage(win_name, D);
    c = cvWaitKey(0);
    printf("c = %d\n", c);
    fv_time_meter_set(0);
    fv_cv_speedy_convolution(C, A, B);
    fv_time_meter_get(0, 0);
    cvShowImage(win_name, C);
    c = cvWaitKey(0);
    cvDestroyWindow(win_name);

    fv_release_image(&img);
    fv_release_image(&dst);
    fv_release_mat(&src);
    cvReleaseImage(&D);
    cvReleaseImage(&gray);
    cvReleaseMat(&C);
    cvReleaseMat(&B);
    cvReleaseMat(&A);

    return FV_OK;
}
