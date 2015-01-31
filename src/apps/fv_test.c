#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "fv_types.h"
#include "fv_app.h"
#include "fv_track.h"
#include "fv_log.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_opencv.h"
#include "fv_morph.h"
#include "fv_time.h"
#include "fv_math.h"
#include "fv_convert.h"
#include "fv_dxt.h"
#include "fv_dft.h"
#include "fv_mem.h"

static void
_fv_dft_1D_real(float *r, float *i, float *src, float width,
            float pi2, fv_s32 x, fv_s32 step)
{
    float       theta;
    float       w;

    *r = *i = 0;
    for (w = 0; w < width; w++, src += step) {
        theta = pi2*x*w/width;
        *r += *src * fv_cos(theta);
        *i -= *src * fv_sin(theta);
    }
}

static void
fv_dft_bow(float r1, float i1, float r2, float i2, float theta, 
        float *or1, float *oi1, float *or2, float *oi2)
{
    float       r;
    float       i;

    fv_complex_multiply(r2, i2, fv_cos(theta), -fv_sin(theta), r, i);

    *or1 = r1 + r;
    *oi1 = i1 + i;
    *or2 = r1 - r;
    *oi2 = i1 - i;
}

static void
fv_dft_bow_pair(fv_u32 k, fv_u32 t, fv_u32 base, fv_s32 *one,
        fv_s32 *tow)
{
    *one = 0;
    *tow = 1;
}

void
fv_dft_1D_real(float *dst, float *src, fv_s32 n, fv_s32 m,float pi2, 
            float scale, float *buf)
{
    float       *b;
    float       *f1;
    float       *f2;
    float       theta;
    fv_s32      base;
    fv_s32      x;
    fv_s32      y;
    fv_s32      i;
    fv_s32      j;
    fv_s32      j2;
    fv_s32      l;
    fv_s32      k;
    fv_s32      one;
    fv_s32      tow;

    l = (n >> m);
    base = (1 << m);
    for (x = 0; x < l; x++, dst += 2) {
        /* 初始化输入数据 */
        for (k = 0; k < base; k++) {
            b = &buf[2*k];
            i = fv_num_map(k, m);
            _fv_dft_1D_real(b, b + 1, src + i, l, pi2, x, base);
        }
        theta = pi2*x/n;
        /* m级蝶形运算 */
        for (k = 0; k < m; k++) {
            /* 1 << (m - 1)次蝶形运算 */
            for (y = 0; y < (1 << (m - 1)); y++) {
                fv_dft_bow_pair(k, y, (1 << (m - 1)), &one, &tow);
                f1 = buf + (one << 1);
                f2 = buf + (tow << 1);
                fv_dft_bow(f1[0], f1[1], f2[0], f2[1], theta, f1, f1 + 1,
                        f2, f2 + 1);
            }
        }
        /* 获得输出 */
        for (j = 0; j < base; j++) {
            j2 = (j << 1);
            dst[j2*l] = buf[j2] * scale;
            dst[j2*l + 1] = buf[j2 + 1] * scale;
        }
    }
}

typedef struct _fv_test_proc_t {
    char            *tp_name;
    fv_bool         tp_need_file;
    fv_proc_func    tp_func;
} fv_test_proc_t;

static fv_s32 fv_test_mat(IplImage *, fv_bool);
static fv_s32 fv_test_selem(IplImage *, fv_bool);
static fv_s32 fv_test_sort(IplImage *, fv_bool);
static fv_s32 fv_test_dft(IplImage *, fv_bool);

static fv_test_proc_t fv_test_algorithm[] = {
    {"mat", 0, fv_test_mat},
    {"selem", 0, fv_test_selem},
    {"sort", 0, fv_test_sort},
    {"dft", 1, fv_test_dft},
};

#define fv_test_alg_num (sizeof(fv_test_algorithm)/sizeof(fv_test_proc_t))

static const char *
fv_program_version = "1.0.0";//PACKAGE_STRING;

static const struct option 
fv_long_opts[] = {
	{"help", 0, 0, 'H'},
	{"debug", 0, 0, 'D'},
	{"list", 0, 0, 'L'},
	{"save", 0, 0, 'S'},
	{"image_file", 1, 0, 'f'},
	{"output_dir", 1, 0, 'o'},
	{"mode", 1, 0, 'm'},
	{"name", 1, 0, 'n'},

	{0, 0, 0, 0}
};

static const char *
fv_options[] = {
	"--debug        -D	enable debug mode\n",	
	"--save         -S	save image\n",	
	"--list         -L	list algorithm name\n",	
	"--file         -f	file location\n",
	"--mode         -m	mode of algorithm, 0 for forgevision, 1 for opencv\n",	
	"--nname        -n	the name of test function\n",	
	"--help         -H	Print help information\n",	
};

static void 
fv_help()
{
	int i;

	fprintf(stdout, "Version: %s\n", fv_program_version);

	fprintf(stdout, "\nOptions:\n");
	for(i = 0; i < sizeof(fv_options)/sizeof(char *); i ++) {
		fprintf(stdout, "  %s", fv_options[i]);
	}

	return;
}

static const char *
fv_optstring = "HDLSf:o:n:m:";

int main(int argc, char **argv)  
{
    int                         c;
    char                        *im_file = NULL;
    char                        *name = NULL;
    fv_test_proc_t              *alg = NULL;
    fv_bool                     list = 0;
    fv_u32                      i;

    while((c = getopt_long(argc, argv, 
                    fv_optstring,  fv_long_opts, NULL)) != -1) {
        switch(c) {
            case 'H':
                fv_help();
                return FV_ERROR;

            case 'D':
                fv_log_enable();
                break;

            case 'S':
                fv_debug_set_print(fv_cv_save_img);
                break;

            case 'n':
                name = optarg;
                break;

            case 'L':
                list = 1;
                break;

            case 'f':
                im_file = optarg;
                break;

            default:
                fv_help();
                return FV_ERROR;
        }
    }

    if (list) {
        for (i = 0; i < fv_test_alg_num; i++) {
            fprintf(stdout, "%s\n", fv_test_algorithm[i].tp_name);
        }
        return FV_OK;
    }

    if (name == NULL) {
        fprintf(stderr, "Please input algorithm name with -n!\n");
        return FV_ERROR;
    }

    for (i = 0; i < fv_test_alg_num; i++) {
        if (strcmp(name, fv_test_algorithm[i].tp_name) == 0) {
            alg = &fv_test_algorithm[i];
            break;
        }
    }

    if (alg == NULL) {
        fprintf(stderr, "Unknow algorithm name %s!\n", name);
        return FV_ERROR;
    }

    if (alg->tp_need_file) {
        if (im_file == NULL) {
            fprintf(stderr, "Please input file name with -f!\n");
            return FV_ERROR;
        }
        return fv_cv_detect_img(im_file, alg->tp_func);
    }

    return alg->tp_func(NULL, 0);
}

static fv_s32 
fv_test_mat(IplImage *img, fv_bool image)
{
    fv_mat_t    *mat;
    fv_s32      row = 3;
    fv_s32      col = 4;
    double      v;

    mat = fv_create_mat(5, 8, FV_MAKETYPE(FV_DEPTH_8U, 1));

    fv_mset(mat, row, col, 1, 34);
    v = fv_mget(mat, row, col, 1, FV_BORDER_CONSTANT);
    printf("value = %f\n", v);

    fv_release_mat(&mat);
    return FV_OK;
}

#define FV_STRUCT_ELEM_COL      3
#define FV_STRUCT_ELEM_ROW      3
#define FV_STRUCT_ANCHOR_X      1
#define FV_STRUCT_ANCHOR_Y      1
#define FV_STRUCT_SHAPE1        CV_SHAPE_RECT
#define FV_STRUCT_SHAPE2        CV_SHAPE_CROSS
#define FV_STRUCT_SHAPE3        CV_SHAPE_ELLIPSE
#define FV_STRUCT_SHAPE4        CV_SHAPE_CUSTOM

static fv_s32 
_fv_test_selem(fv_s32 col, fv_s32 row, fv_s32 anchor_x, fv_s32 anchor_y,
            fv_s32 shape, fv_s32 *data)
{
    IplConvKernel       *elem;
    fv_conv_kernel_t    *kernel;
    int                 *v1;
    int                 *v2;
    fv_s32              i;
    fv_s32              j;
    fv_s32              ret = FV_OK;
    
    elem = cvCreateStructuringElementEx(col, row, anchor_x, anchor_y, 
                shape, data);

    kernel = fv_create_structuring_element_ex(col, row, anchor_x, anchor_y,
                shape, data);

    v1 = elem->values;
    v2 = kernel->ck_values;
    for (i = 0; i < row; i++) {
        for (j = 0; j < col; j++, v1++, v2++) {
            if (*v1 != *v2) {
                printf("%d %d ", *v1, *v2);
                ret = FV_ERROR;
            }
        }
    }

    fv_release_structuring_element(&kernel);
    cvReleaseStructuringElement(&elem);

    return ret;
}
static fv_s32 
fv_test_selem(IplImage *img, fv_bool image)
{
    fv_s32              ker[FV_STRUCT_ELEM_COL*FV_STRUCT_ELEM_ROW] = 
                {0, 1, 0, 1, 1, 0};
    fv_s32              ret;
    
    ret = _fv_test_selem(FV_STRUCT_ELEM_COL, FV_STRUCT_ELEM_ROW, 
            FV_STRUCT_ANCHOR_X, FV_STRUCT_ANCHOR_Y, 
            FV_STRUCT_SHAPE1, NULL);

    if (ret != FV_OK) {
        fprintf(stderr, "Error!\n");
        return ret;
    }

    fprintf(stdout, "OK!\n");

    ret = _fv_test_selem(FV_STRUCT_ELEM_COL, FV_STRUCT_ELEM_ROW, 
            FV_STRUCT_ANCHOR_X, FV_STRUCT_ANCHOR_Y, 
            FV_STRUCT_SHAPE2, NULL);

    if (ret != FV_OK) {
        fprintf(stderr, "Error!\n");
        return ret;
    }

    fprintf(stdout, "OK!\n");

    ret = _fv_test_selem(FV_STRUCT_ELEM_COL, FV_STRUCT_ELEM_ROW, 
            FV_STRUCT_ANCHOR_X, FV_STRUCT_ANCHOR_Y, 
            FV_STRUCT_SHAPE3, NULL);

    if (ret != FV_OK) {
        fprintf(stderr, "Error!\n");
        return ret;
    }

    fprintf(stdout, "OK!\n");

    ret = _fv_test_selem(FV_STRUCT_ELEM_COL, FV_STRUCT_ELEM_ROW, 
            FV_STRUCT_ANCHOR_X, FV_STRUCT_ANCHOR_Y, 
            FV_STRUCT_SHAPE4, ker);

    if (ret != FV_OK) {
        fprintf(stderr, "Error!\n");
        return ret;
    }

    fprintf(stdout, "OK!\n");

    return FV_OK;
}

#define FV_TEST_SORT_NUM    10000

int fv_test_sort_data[FV_TEST_SORT_NUM];
int fv_test_sort_buf[FV_TEST_SORT_NUM];

void 
fv_insert_sort(int *sort_buf, int base, int total, int *accum)
{
    fv_s32      i;
    fv_s32      j;

    if (total == 0) {
        sort_buf[0] = base;
        return;
    }

    for (i = 0; i < total; i++) {
        if (accum[base] > accum[sort_buf[i]]) {
            for (j = total - 1; j >= i; j--) {
                sort_buf[j + 1] = sort_buf[j];
            }
            break;
        }
    }
    sort_buf[i] = base;
}

void
fv_bubble_sort(int *sort_buf, int total, int *accum)
{
    fv_s32      i;
    fv_s32      j;

    for (i = 0; i < total - 1; i++) {
        for (j = 0; j < total - 1 - i; j++) {
            if (accum[sort_buf[j]] < accum[sort_buf[j + 1]]) {
                fv_swap(sort_buf[j], sort_buf[j + 1]);
            }
        }
    }
}

#define hough_cmp_gt(l1,l2) (aux[l1] > aux[l2])

static FV_IMPLEMENT_QSORT_EX(hough_sort, int, hough_cmp_gt, const int* )

static fv_s32 
fv_test_sort(IplImage *img, fv_bool image)
{
    int     i;

    for (i = 0; i < FV_TEST_SORT_NUM; i++) {
        fv_test_sort_data[i] = random() % FV_TEST_SORT_NUM;
        fv_test_sort_buf[i] = i;
    }

    fv_time_meter_set(0);
    for (i = 0; i < FV_TEST_SORT_NUM; i++) {
        fv_insert_sort(fv_test_sort_buf, i, i, fv_test_sort_data);
    };
    fv_time_meter_get(0, 0);

    for (i = 0; i < 20; i++) {
        fprintf(stdout, "%d ", fv_test_sort_data[fv_test_sort_buf[i]]);
    }
    fprintf(stdout, "Insert sort\n");

    for (i = 0; i < FV_TEST_SORT_NUM; i++) {
        fv_test_sort_buf[i] = i;
    }

    fv_time_meter_set(0);
    fv_bubble_sort(fv_test_sort_buf, FV_TEST_SORT_NUM, fv_test_sort_data);
    fv_time_meter_get(0, 0);
    for (i = 0; i < 20; i++) {
        fprintf(stdout, "%d ", fv_test_sort_data[fv_test_sort_buf[i]]);
    }
    fprintf(stdout, "Bubble sort\n");

    for (i = 0; i < FV_TEST_SORT_NUM; i++) {
        fv_test_sort_buf[i] = i;
    }

    fv_time_meter_set(0);
    hough_sort(fv_test_sort_buf, FV_TEST_SORT_NUM, fv_test_sort_data);
    for (i = 0; i < 20; i++) {
        fprintf(stdout, "%d ", fv_test_sort_data[fv_test_sort_buf[i]]);
    }
    fprintf(stdout, "hough sort\n");
    fv_time_meter_get(0, 0);

    return FV_OK;
}

static void
fv_test_dft_cmp(fv_mat_t *dst, fv_mat_t *src) 
{
    float       *dst_data;
    float       *src_data;
    float       row;
    float       col;
    float       width;
    float       height;

    width = dst->mt_cols;
    height = dst->mt_rows;

    dst_data = dst->mt_data.dt_fl;
    src_data = src->mt_data.dt_fl;
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++, dst_data++, src_data++) {
            if (fabs(*dst_data - *src_data) > 0.001) {
                printf("Error! %f\n", fabs(*dst_data - *src_data));
            }
        }
    }
}

#define fv_buf_size 16384
static float s[fv_buf_size] = {
    1, 2, 3, 4, 
    2, 3, 4, 5, 
    3, 4, 5, 6, 
    4, 5, 6, 7};
static float sk[fv_buf_size] = {
    1, 2, 3, 4, 
    1, 1, 1, 1, 
    1, 1, 1, 1, 
    1, 1, 1, 1};
static float k[fv_buf_size] = {
    -1, 0, 0, 0,
    1, 0, 0, 0};

static float d1[fv_buf_size*2];
static float d2[fv_buf_size*2];
static float buf[fv_buf_size*2];

extern void fv_fft_inverse_row(float *dst, float *src, fv_s32 n, fv_s32 m, float pi2);

static fv_s32 
fv_test_dft(IplImage *cv_img, fv_bool image)
{
    IplImage        *gray;
    fv_image_t      *img;
    fv_image_t      *dst;
    fv_mat_t        _dst;
    fv_mat_t        mat;
    fv_mat_t        *src;
    fv_mat_t        *dft_a;
    fv_mat_t        d;
    fv_mat_t        s2;
    int m;
    int n;
    int i;
    int c;

    FV_ASSERT(image);

    gray = cvCreateImage(cvGetSize(cv_img), cv_img->depth, 1);
    FV_ASSERT(gray != NULL);
    cvCvtColor(cv_img, gray, CV_BGR2GRAY);

    img = fv_convert_image(gray);
    if (img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    n = 4;
    m = 1;
    d = fv_mat(n, n, FV_32FC2, d1);
    s2 = fv_mat(n, n, FV_32FC1, s);

    fv_dft(&d, &s2, FV_DXT_FORWARD, s2.mt_rows);

    for (c = 0; c < n; c++) {
        for (i = 0; i < n; i++) {
            printf("[%f %f], ", d1[c*2*n + 2*i], d1[c*2*n + 2*i + 1]);
        }
        printf("\n");
    }

    d = fv_mat(m, n, FV_32FC2, d1);
    s2 = fv_mat(m, n, FV_32FC1, k);

    fv_dft(&d, &s2, FV_DXT_FORWARD, s2.mt_rows);

    for (c = 0; c < m; c++) {
        for (i = 0; i < n; i++) {
            printf("[%f %f], ", d1[c*2*n + 2*i], d1[c*2*n + 2*i + 1]);
        }
        printf("\n");
    }

    d = fv_mat(n, n, FV_32FC2, d1);
    s2 = fv_mat(n, n, FV_32FC1, sk);

    fv_dft(&d, &s2, FV_DXT_FORWARD, s2.mt_rows);

    for (c = 0; c < n; c++) {
        for (i = 0; i < n; i++) {
            printf("[%f %f], ", d1[c*2*n + 2*i], d1[c*2*n + 2*i + 1]);
        }
        printf("\n");
    }

    fv_dft_1D_real(d1, s, n, 1, 2*fv_pi, 
            1.0/n, buf);
    fv_fft_real(d2, s, n, log(n)/log(2), 2*fv_pi);

    c = 0;
    for (i = 0; i < n; i++) {
        if (fabs(d1[2*i] - d2[2*i]) > 0.001 ||
                fabs(d1[2*i + 1] - d2[2*i + 1]) > 0.001) {
              printf("[%f %f] ", fabs(d1[2*i] - d2[2*i]), 
                        fabs(d1[2*i + 1] - d2[2*i + 1]));
            printf("diff\n");
            c++;
        }
    }

    if (c > 0) {
        for (i = 0; i < n; i++) {
            printf("[%f %f] ", d1[2*i], d1[2*i + 1]);
        }
        printf("test\n");
        for (i = 0; i < n; i++) {
            printf("[%f %f] ", d2[2*i], d2[2*i + 1]);
        }
        printf("fft\n");

        printf("Error! num is %d\n", c);
        return -1;
    }
    mat = fv_image_to_mat(img);
    src = fv_create_mat(mat.mt_rows, mat.mt_cols, FV_32FC1);
    FV_ASSERT(src != NULL);

    dst = fv_create_image(fv_size(mat.mt_cols, mat.mt_rows), FV_DEPTH_32F, 1);
    if (dst == NULL) {
        FV_LOG_ERR("Alloc image faield!\n");
    }

    _dst = fv_image_to_mat(dst);
    _fv_convert(src, &mat);
 
    dft_a = fv_create_mat(src->mt_rows, src->mt_cols, FV_32FC2);
    FV_ASSERT(dft_a != NULL);

    fv_dft(dft_a, src, FV_DXT_FORWARD, src->mt_rows);

    fv_dft(&_dst, dft_a, FV_DXT_INVERSE, src->mt_rows);

    fv_test_dft_cmp(&_dst, src);
    fv_release_image(&img);
    fv_release_image(&dst);
    fv_release_mat(&src);
    fv_release_mat(&dft_a);
    cvReleaseImage(&gray);

    return FV_OK;
}
