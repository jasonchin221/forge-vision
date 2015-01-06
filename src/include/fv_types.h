#ifndef __FV_TYPES_H__
#define __FV_TYPES_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define FV_OK                           0
#define FV_ERROR                        (-1)
#define FV_ORIGIN_TL                    0   /* Top Left is origin */
#define FV_ORIGIN_BL                    1   /* Bottom Left is origin */
#define FV_DEFAULT_IMAGE_ROW_ALIGN      4   /* default image row align (in bytes) */
#define FV_GRAY_LEVEL                   256

#define FV_CN_SHIFT                     16
#define FV_DEPTH_MAX                    64
#define FV_DEPTH_SIGN                   (1 << (FV_CN_SHIFT - 1))

#define FV_8U                           0
#define FV_8S                           1
#define FV_16U                          2
#define FV_16S                          3
#define FV_32S                          4
#define FV_32F                          5
#define FV_64F                          6
#define FV_DEPTH_NUM                    7

#define FV_DEPTH_1U                     1
#define FV_DEPTH_8U                     8
#define FV_DEPTH_16U                    16
#define FV_DEPTH_32F                    32

#define FV_DEPTH_8S                     (FV_DEPTH_SIGN| 8)
#define FV_DEPTH_16S                    (FV_DEPTH_SIGN|16)
#define FV_DEPTH_32S                    (FV_DEPTH_SIGN|32)
#define FV_DEPTH_64F                    64

#define FV_MAX_CHANNEL                  512

#define FV_DATA_ORDER_PIXEL             0
#define FV_DATA_ORDER_PLANE             1

#define fv_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/*
 * fv_container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 */
#define fv_container_of(ptr, type, member) ({          \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef unsigned long           fv_ulong;
typedef unsigned long long      fv_u64;
typedef unsigned int            fv_u32;
typedef unsigned short          fv_u16;
typedef unsigned char           fv_u8;

typedef long                    fv_long;
typedef long long               fv_s64;
typedef int                     fv_s32;
typedef short                   fv_s16;
typedef char                    fv_s8;

typedef bool                    fv_bool;

/* fv_arr* is used to pass arbitrary
 *  array-like data structures
 *  into functions where the particular
 *  array type is recognized at runtime:
 */
typedef void fv_arr;      

typedef enum _fv_base_type_m {
    FV_BASE_TYPE_IMAGE = 0,
    FV_BASE_TYPE_MAT = 1,
    FV_BASE_TYPE_MAX,
} fv_base_type_m;

/* Image smooth methods */
enum {
    FV_BLUR_NO_SCALE,
    FV_BLUR,
    FV_GAUSSIAN,
    FV_MEDIAN,
    FV_BILATERAL,
};

typedef struct _fv_point_t {
    fv_s32          pt_x;
    fv_s32          pt_y;
} fv_point_t;

typedef struct _fv_size_t {
    fv_s32          sz_width;
    fv_s32          sz_height;
} fv_size_t;

typedef struct _fv_line_polar_t {
    float           lp_rho;
    float           lp_angle;
} fv_line_polar_t;

/*
 * @rt_point: left bottom
 */
typedef struct _fv_rect_t {
    fv_point_t      rt_point;
    fv_size_t       rt_size;
#define rt_x rt_point.pt_x
#define rt_y rt_point.pt_y
#define rt_width rt_size.sz_width
#define rt_height rt_size.sz_height
} fv_rect_t;

typedef struct _fv_size_2D32f_t {
    float       sf_width;
    float       sf_height;
} fv_size_2D32f_t;

typedef struct _fv_point_2D32f_t {
    float       pf_x;
    float       pf_y;
} fv_point_2D32f_t;

typedef struct _fv_line_t {
    fv_point_2D32f_t    ln_p1;
    fv_point_2D32f_t    ln_p2;
} fv_line_t;

typedef struct _fv_circle_t {
    fv_point_t  cl_center;
    float       cl_radius;
} fv_circle_t;

typedef struct _fv_roi_t {
    fv_s32      ri_coi; /* -1 - no COI (all channels are selected), 0 - 0th channel is selected ...*/
    fv_s32      ri_x_offset;
    fv_s32      ri_y_offset;
    fv_s32      ri_width;
    fv_s32      ri_height;
} fv_roi_t;

typedef struct _fv_conv_kernel_t {
    fv_s32      ck_ncols;
    fv_s32      ck_nrows;
    fv_s32      ck_anchor_x;
    fv_s32      ck_anchor_y;
    fv_s32      *ck_values;
    fv_s32      ck_nshift_r;
} fv_conv_kernel_t;

typedef struct _fv_base_operation_t {
    fv_s32      (*bo_create_data)(fv_arr *);
    void        (*bo_release_data)(fv_arr *);
    fv_size_t   (*bo_get_size)(fv_arr *);
    fv_s32      (*bo_inc_ref)(fv_arr *);
    void        (*bo_dec_ref)(fv_arr *);
} fv_base_operation_t;

typedef struct _fv_image_t {
    fv_u32      ig_type;        /* Must be FV_BASE_TYPE_IMAGE */
    fv_u32      ig_channels;    /* Most of OpenCV functions support 1,2,3 or 4 channels */
    fv_u32      ig_depth;       /* Pixel depth in bits:
                                   FV_DEPTH_8U, FV_DEPTH_8S, FV_DEPTH_16S,
                                   FV_DEPTH_32S, FV_DEPTH_32F and FV_DEPTH_64F are supported.  */
    fv_u32      ig_depth2;
    fv_s32      ig_width;        /* Image width in pixels.                           */
    fv_s32      ig_height;       /* Image height in pixels.                          */
    fv_s32      ig_total;        /* The total number of pixels.                      */
    fv_s32      ig_refcount;
    fv_roi_t    *ig_roi;         /* Image ROI. If NULL, the whole image is selected. */
    fv_u32      ig_image_size;         /* Image data size in bytes
                                            (==image->height*image->widthStep
                                            in case of interleaved data)*/
    fv_s32      ig_width_step;         /* Size of aligned image row in bytes.    */
    union {
        fv_s8   *ig_image_data;        /* Pointer to aligned image data.         */
        void    *ig_data_ptr;
    };
    fv_s8       *ig_image_data_origin;  /* Pointer to very origin of image data
                                            (not necessarily aligned) -
                                            needed for correct deallocation */
} fv_image_t;

#define FV_DEPTH(depth) (depth & ~FV_DEPTH_SIGN)
#define FV_MAKETYPE(depth, cn) (FV_DEPTH(depth) + ((cn) << FV_CN_SHIFT))
#define FV_IMAGE_GET_TYPE(img) \
    FV_MAKETYPE(img->ig_depth, img->ig_channels)

#define FV_8UC1     FV_MAKETYPE(FV_DEPTH_8U, 1)
#define FV_8UC2     FV_MAKETYPE(FV_DEPTH_8U, 2)
#define FV_8UC3     FV_MAKETYPE(FV_DEPTH_8U, 3)
#define FV_8UC4     FV_MAKETYPE(FV_DEPTH_8U, 4)
#define FV_8UC(n)   FV_MAKETYPE(FV_DEPTH_8U, n)

#define FV_16SC1     FV_MAKETYPE(FV_DEPTH_16S, 1)
#define FV_16SC2     FV_MAKETYPE(FV_DEPTH_16S, 2)
#define FV_16SC3     FV_MAKETYPE(FV_DEPTH_16S, 3)
#define FV_16SC4     FV_MAKETYPE(FV_DEPTH_16S, 4)
#define FV_16SC(n)   FV_MAKETYPE(FV_DEPTH_16S, n)

#define FV_32SC1    FV_MAKETYPE(FV_DEPTH_32S, 1)
#define FV_32SC2    FV_MAKETYPE(FV_DEPTH_32S, 2)
#define FV_32SC3    FV_MAKETYPE(FV_DEPTH_32S, 3)
#define FV_32SC4    FV_MAKETYPE(FV_DEPTH_32S, 4)
#define FV_32SC(n)  FV_MAKETYPE(FV_DEPTH_32S, n)

#define FV_32FC1    FV_MAKETYPE(FV_DEPTH_32F, 1)
#define FV_32FC2    FV_MAKETYPE(FV_DEPTH_32F, 2)
#define FV_32FC3    FV_MAKETYPE(FV_DEPTH_32F, 3)
#define FV_32FC4    FV_MAKETYPE(FV_DEPTH_32F, 4)
#define FV_32FC(n)  FV_MAKETYPE(FV_DEPTH_32F, n)

#define FV_64FC1    FV_MAKETYPE(FV_DEPTH_64F, 1)
#define FV_64FC2    FV_MAKETYPE(FV_DEPTH_64F, 2)
#define FV_64FC3    FV_MAKETYPE(FV_DEPTH_64F, 3)
#define FV_64FC4    FV_MAKETYPE(FV_DEPTH_64F, 4)
#define FV_64FC(n)  FV_MAKETYPE(FV_DEPTH_64F, n)


typedef struct _fv_mat_t {
    fv_u32      mt_type;        /* Must be FV_BASE_TYPE_MAT */
    union {
        fv_u32      mt_atr;
        struct {
            fv_u16  mt_idepth;
            fv_u16  mt_nchannel;
        };
    };
    fv_s32      mt_step;
    fv_s32      mt_total;
    fv_s32      mt_total_size;

    /* for internal use only */
    fv_s32      *mt_refcount;
    fv_u32      mt_depth;
    fv_s32      mt_hdr_refcount;

    union {
        fv_u8   *dt_ptr;
        fv_s16  *dt_s;
        fv_s32  *dt_i;
        float   *dt_fl;
        double  *dt_db;
    } mt_data;

    union {
        fv_s32      mt_rows;
        fv_s32      mt_height;
    };
    union {
        fv_s32      mt_cols;
        fv_s32      mt_width;
    };
} fv_mat_t;

#define FV_MAT_DEPTH(mat) ((mat)->mt_idepth)
#define FV_DEPTH_ELEM_SIZE(depth) (((depth) + 7) >> 3)
#define FV_MAT_ELEM_SIZE(mat) FV_DEPTH_ELEM_SIZE(FV_MAT_DEPTH(mat))
#define FV_MAT_NCHANNEL(mat) ((mat)->mt_nchannel)
#define FV_ELEM_NCHANNEL(atr) (atr >> FV_CN_SHIFT)
#define _FV_ELEM_SIZE(atr) FV_DEPTH_ELEM_SIZE(atr & 0xFFFF)
#define FV_ELEM_SIZE(atr) ((FV_ELEM_NCHANNEL(atr))*_FV_ELEM_SIZE(atr))

// Initializes CvMat header, allocated by the user
static inline void
fv_init_mat_header(fv_mat_t *arr, fv_s32 rows, fv_s32 cols,
                 fv_s32 atr, void *data, fv_s32 step)
{
    arr->mt_step = step;
    arr->mt_type = FV_BASE_TYPE_MAT;
    arr->mt_atr = atr;
    arr->mt_rows = rows;
    arr->mt_cols = cols;
    arr->mt_total = rows*cols;
    arr->mt_data.dt_ptr = data;
    arr->mt_refcount = NULL;
    arr->mt_hdr_refcount = 1;

    switch (arr->mt_idepth) {
        case FV_DEPTH_1U:
        case FV_DEPTH_8U:
            arr->mt_depth = FV_8U;
            break;
        case FV_DEPTH_8S:
            arr->mt_depth = FV_8S;
            break;
        case FV_DEPTH_16U:
            arr->mt_depth = FV_16U;
            break;
        case FV_DEPTH_16S:
            arr->mt_depth = FV_16S;
            break;
        case FV_DEPTH_32F:
            arr->mt_depth = FV_32F;
            break;
        case FV_DEPTH_32S:
            arr->mt_depth = FV_32S;
            break;
        case FV_DEPTH_64F:
            arr->mt_depth = FV_64F;
            break;
        default:
            fprintf(stderr,"Unknow depth type %d!\n", arr->mt_idepth);
    } 
}

/* Inline constructor. No data is allocated internally!!!
 * (Use together with cvCreateData, or use cvCreateMat instead to
 * get a matrix with allocated data):
 */
static inline fv_mat_t 
fv_mat(fv_s32 rows, fv_s32 cols, fv_s32 atr, void *data)
{
    fv_mat_t    m;

    fv_init_mat_header(&m, rows, cols, atr, data, cols*FV_ELEM_SIZE(atr));

    return m;
}

#define FV_MAT_ELEM_PTR_FAST(mat, row, col, pix_size)  \
    (FV_ASSERT((unsigned)(row) < (unsigned)(mat).mt_rows &&   \
             (unsigned)(col) < (unsigned)(mat).mt_cols ),   \
     (mat).mt_data.dt_ptr + (size_t)(mat).mt_step*(row) + (pix_size)*(col))

#define FV_MAT_ELEM_PTR(mat, row, col)                 \
    FV_MAT_ELEM_PTR_FAST(mat, row, col, FV_ELEM_SIZE((mat).mt_atr))

#define FV_MAT_ELEM(mat, elemtype, row, col)           \
    (*(elemtype*)FV_MAT_ELEM_PTR_FAST(mat, row, col, sizeof(elemtype)))


#define fv_mset_scale(mat, scale, type) \
    do { \
        type    *data; \
        fv_s32  row; \
        fv_s32  col; \
        for (row = 0; row < mat.mt_rows; row++) { \
            for (col = 0; col < mat.mt_cols; col++) { \
                data = (type *)(mat.mt_data.dt_ptr + mat.mt_step*row + col); \
                *data *= scale; \
            } \
        } \
    } while(0)

#define FV_TERMCRIT_ITER    1
#define FV_TERMCRIT_NUMBER  FV_TERMCRIT_ITER
#define FV_TERMCRIT_EPS     2

typedef struct _fv_term_criteria_t {
    fv_u32      tc_type;  /*
                             may be combination of
                             CV_TERMCRIT_ITER
                             CV_TERMCRIT_EPS
                          */
    fv_u32      tc_max_iter;
    double      tc_epsilon;
} fv_term_criteria_t;

static inline fv_term_criteria_t
fv_term_criteria(fv_u32 type, fv_u32 max_iter, double epsilon)
{
    fv_term_criteria_t      t;

    t.tc_type = type;
    t.tc_max_iter = max_iter;
    t.tc_epsilon = epsilon;

    return t;
}

static inline fv_size_t 
fv_size(fv_s32 width, fv_s32 height)
{
    fv_size_t   s;

    s.sz_width = width;
    s.sz_height = height;

    return s;
}

static inline fv_point_t
fv_point(fv_s32 x, fv_s32 y)
{
    fv_point_t      p;

    p.pt_x = x;
    p.pt_y = y;

    return p;
}

static inline fv_rect_t 
fv_rect(fv_s32 x, fv_s32 y, fv_s32 width, fv_s32 height)
{
    fv_rect_t       r;

    r.rt_point = fv_point(x, y);
    r.rt_size = fv_size(width, height);

    return r;
}

static inline fv_size_2D32f_t 
fv_size_2D32f(double width, double height)
{
    fv_size_2D32f_t     s;

    s.sf_width = (float)width;
    s.sf_height = (float)height;

    return s;
}

static inline fv_point_2D32f_t 
fv_point_2D32f(double x, double y)
{
    fv_point_2D32f_t     p;

    p.pf_x = (float)x;
    p.pf_y = (float)y;

    return p;
}

static inline fv_point_2D32f_t  
fv_point_to_32f(fv_point_t point)
{
    return fv_point_2D32f((float)point.pt_x, (float)point.pt_y);
}


#endif
