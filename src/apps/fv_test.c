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

typedef struct _fv_test_proc_t {
    char            *tp_name;
    fv_bool         tp_need_file;
    fv_proc_func    tp_func;
} fv_test_proc_t;

static fv_s32 fv_test_mat(IplImage *, fv_bool);
static fv_s32 fv_test_selem(IplImage *, fv_bool);
static fv_test_proc_t fv_test_algorithm[] = {
    {"mat", 0, fv_test_mat},
    {"selem", 0, fv_test_selem},
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
