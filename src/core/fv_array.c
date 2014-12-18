#include <string.h>
#include <opencv/cv.h>  
#include <opencv/highgui.h>

#include "fv_types.h"
#include "fv_log.h"
#include "fv_mem.h"
#include "fv_math.h"
#include "fv_debug.h"
#include "fv_imgproc.h"
#include "fv_border.h"

static fv_s32 fv_image_create_data(fv_arr *arr);
static void fv_image_release_data(fv_arr *arr);
static fv_size_t fv_image_get_size(fv_arr *arr);

static fv_s32 fv_mat_create_data(fv_arr *arr);
static void fv_mat_release_data(fv_arr *arr);
static fv_size_t fv_mat_get_size(fv_arr *arr);
static fv_s32 fv_mat_inc_ref_data(fv_arr *arr);
static void fv_mat_dec_ref_data(fv_arr *arr);

static fv_base_operation_t fv_base_op[FV_BASE_TYPE_MAX] = {
    /* FV_BASE_TYPE_IMAGE  */
    {
        .bo_create_data = fv_image_create_data,
        .bo_release_data = fv_image_release_data,
        .bo_get_size = fv_image_get_size,
    },

    /* FV_BASE_TYPE_MAT  */
    {
        .bo_create_data = fv_mat_create_data,
        .bo_release_data = fv_mat_release_data,
        .bo_get_size = fv_mat_get_size,
        .bo_inc_ref = fv_mat_inc_ref_data,
        .bo_dec_ref = fv_mat_dec_ref_data,
    },
};

/* Decrements CvMat data reference counter and deallocates the data if
   it reaches 0 */
void  
fv_dec_ref_data(fv_arr *arr)
{
    fv_u32          type;

    type = *((fv_u32 *)arr);
    FV_ASSERT(type < FV_BASE_TYPE_MAX);

    if (fv_base_op[type].bo_dec_ref != NULL) {
        fv_base_op[type].bo_dec_ref(arr);
    }
}

static fv_s32  
fv_mat_inc_ref_data(fv_arr *arr)
{
    fv_mat_t        *mat;
    fv_s32          refcount = 0;

    mat = arr;
    if (mat->mt_refcount != NULL) {
        refcount = ++*mat->mt_refcount;
    }

    return refcount;
}

static fv_s32
fv_image_create_data(fv_arr *arr)
{
    fv_image_t      *img;

    img = arr;
    if (img->ig_image_data != NULL) {
        FV_LOG_ERR("Data is already allocated\n");
    }

    img->ig_image_data = img->ig_image_data_origin =
        fv_calloc(img->ig_image_size);

    if (img->ig_image_data == NULL) {
        FV_LOG_ERR("Data alloc failed!\n");
    }

    return FV_OK;
}

static fv_s32
fv_mat_create_data(fv_arr *arr)
{
    fv_mat_t    *mat;
    size_t      step;
    size_t      total_size;

    mat = arr;

    if (mat->mt_rows == 0 || mat->mt_cols == 0) {
        return FV_ERROR;
    }

    if (mat->mt_data.dt_ptr != 0) {
        FV_LOG_ERR("Data is already allocated\n");
    }

    step = mat->mt_step;
    if (step == 0) {
        step = FV_ELEM_SIZE(mat->mt_atr)*mat->mt_cols;
    }

    total_size = step*mat->mt_rows + sizeof(fv_s32);
    mat->mt_refcount = fv_calloc(total_size);
    mat->mt_total_size = total_size;
    FV_ASSERT(mat->mt_refcount != NULL);
    mat->mt_data.dt_ptr = (fv_u8 *)(mat->mt_refcount + 1);
    *mat->mt_refcount = 1;

    return FV_OK;
}

void
fv_create_data(fv_arr *arr)
{
    fv_u32          type;

    type = *((fv_u32 *)arr);
    FV_ASSERT(type < FV_BASE_TYPE_MAX && 
            fv_base_op[type].bo_create_data != NULL);
    fv_base_op[type].bo_create_data(arr);
}

void
fv_init_image_header(fv_image_t *img, fv_size_t size, fv_s32 depth, 
            fv_s32 channels, fv_s32 origin, fv_s32 align)
{      
    //const char *colorModel, *channelSeq;

    if (img == NULL) {
        FV_LOG_ERR("Null pointer to header!\n");
    }

    memset(img, 0, sizeof(*img));
    img->ig_type = FV_BASE_TYPE_IMAGE;

#if 0
    icvGetColorModel( channels, &colorModel, &channelSeq );
    strncpy( image->colorModel, colorModel, 4 );
    strncpy( image->channelSeq, channelSeq, 4 );
#endif

    if (size.sz_width < 0 || size.sz_height < 0) {
        FV_LOG_ERR("Bad input roi\n");
    }

    if ((depth != FV_DEPTH_1U && depth != FV_DEPTH_8U &&
         depth != FV_DEPTH_8S && depth != FV_DEPTH_16U &&
         depth != FV_DEPTH_16S && depth != FV_DEPTH_32S &&
         depth != FV_DEPTH_32F && depth != FV_DEPTH_64F) ||
         channels < 0 ) {
        FV_LOG_ERR("Unsupported format, depth = %d, channels = %d\n",
                depth, channels);
    }

    if (origin != FV_ORIGIN_BL && origin != FV_ORIGIN_TL) {
        FV_LOG_ERR("Bad input origin\n");
    }

    if (align != 4 && align != 8) {
        FV_LOG_ERR("Bad input align\n");
    }

    img->ig_width = size.sz_width;
    img->ig_height = size.sz_height;

    if (img->ig_roi) {
        img->ig_roi->ri_coi = 0;
        img->ig_roi->ri_x_offset = img->ig_roi->ri_y_offset = 0;
        img->ig_roi->ri_width = size.sz_width;
        img->ig_roi->ri_height = size.sz_height;
    }

    img->ig_channels = fv_max(channels, 1);
    img->ig_depth = depth;
    img->ig_width_step = (((img->ig_width * img->ig_channels *
         (img->ig_depth & ~FV_DEPTH_SIGN) + 7)/8) + align - 1) & (~(align - 1));
    img->ig_origin = origin;
    img->ig_image_size = img->ig_width_step * img->ig_height;
}

static fv_image_t *
fv_create_image_header(fv_size_t size, fv_s32 depth, fv_s32 channels)
{
    fv_image_t      *img;

    img = fv_alloc(sizeof(*img));
    if (img == NULL) {
        return NULL;
    }

    fv_init_image_header(img, size, depth, channels, 
            FV_ORIGIN_BL, FV_DEFAULT_IMAGE_ROW_ALIGN);

    return img;
}

fv_image_t *
fv_create_image(fv_size_t size, fv_s32 depth, fv_s32 channels)
{
    fv_image_t      *img;
    
    img = fv_create_image_header(size, depth, channels);
    if (img == NULL) {
        FV_LOG_ERR("Alloc image header failed!\n");
    }

    fv_create_data(img);

    return img;
}

static void
fv_image_release_data(fv_arr *arr)
{
    fv_image_t      *img;
    fv_s8           *ptr;

    img = arr;
    ptr = img->ig_image_data_origin;
    img->ig_image_data = img->ig_image_data_origin = NULL;
    fv_free(&ptr);
}

static void
fv_mat_release_data(fv_arr *arr)
{
    fv_dec_ref_data(arr);
}

void
fv_release_data(fv_arr *arr)
{
    fv_u32          type;

    type = *((fv_u32 *)arr);
    FV_ASSERT(type < FV_BASE_TYPE_MAX &&
            fv_base_op[type].bo_release_data != NULL);
    fv_base_op[type].bo_release_data(arr);
}

void
fv_release_image_header(fv_image_t **image)
{
    fv_image_t      *img;

    if (!image) {
        FV_LOG_ERR("Can't free image!'\n");
    }

    if (*image == NULL) {
        return;
    }

    img = *image;
    *image = NULL;

    fv_free(&img->ig_roi);
    fv_free(&img);
}

void
fv_release_image(fv_image_t **image)
{
    fv_image_t      *img;

    if (!image) {
        FV_LOG_ERR("Can't free image!'\n");
    }

    if (*image == NULL) {
        return;
    }

    img = *image;
    *image = NULL;

    fv_release_data(img);
    fv_release_image_header(&img);
}

static fv_size_t
fv_mat_get_size(fv_arr *arr)
{
    fv_mat_t        *mat;
    fv_size_t       size;

    mat = arr;
    size.sz_width = mat->mt_cols;
    size.sz_height = mat->mt_rows;

    return size;
}

static fv_size_t
fv_image_get_size(fv_arr *arr)
{
    fv_image_t      *img;
    fv_size_t       size;

    img = arr;
    if (img->ig_roi) {
        size.sz_width = img->ig_roi->ri_width;
        size.sz_height = img->ig_roi->ri_width;
    } else {
        size.sz_width = img->ig_width;
        size.sz_height = img->ig_height;
    }

    return size;
}

fv_size_t
fv_get_size(fv_arr *arr)
{
    fv_u32          type;

    type = *((fv_u32 *)arr);
    FV_ASSERT(type < FV_BASE_TYPE_MAX && fv_base_op[type].bo_get_size != NULL);

    return fv_base_op[type].bo_get_size(arr);
}

static fv_roi_t* 
fv_icv_create_roi(fv_s32 coi, fv_s32 x_offset, fv_s32 y_offset, 
            fv_s32 width, fv_s32 height)
{
    fv_roi_t    *roi;

    roi = fv_alloc(sizeof(*roi));

    roi->ri_coi = coi;
    roi->ri_x_offset = x_offset;
    roi->ri_y_offset = y_offset;
    roi->ri_width = width;
    roi->ri_height = height;

    return roi;
}

fv_image_t *
fv_convert_image(IplImage *cv_img) 
{
    fv_image_t      *img;
    struct _IplROI  *roi;
    fv_size_t       size;
    fv_u32          i;
    fv_u16          depth;

    size = fv_size(cv_img->width, cv_img->height);

    switch (cv_img->depth) {
        case IPL_DEPTH_8S:
            depth = FV_DEPTH_8S;
            break;
        case IPL_DEPTH_16S:
            depth = FV_DEPTH_16S;
            break;
        case IPL_DEPTH_32S:
            depth = FV_DEPTH_32S;
            break;
        default:
            depth = cv_img->depth;
            break;
    }
    img = fv_create_image(size, depth, cv_img->nChannels);
    if (img == NULL) {
        return NULL;
    }

    roi = cv_img->roi;
    if (roi != NULL) {
        img->ig_roi = fv_icv_create_roi(roi->coi - 1, roi->xOffset, 
                cv_img->height - roi->yOffset - roi->height, 
                roi->width, roi->height);
    }
    
    for (i = 0; i < cv_img->height; i++) {
        memcpy(img->ig_image_data + i*img->ig_width_step, 
                cv_img->imageData + (cv_img->height - i - 1)*cv_img->widthStep,
                img->ig_width_step);
    }

    return img;
}

fv_mat_t
fv_image_to_mat(fv_image_t *image) 
{
    fv_s32      atr = FV_IMAGE_GET_TYPE(image);

    return fv_mat(image->ig_height, image->ig_width, atr, 
            image->ig_image_data);
}

fv_mat_t *
fv_create_mat_header(fv_s32 rows, fv_s32 cols, fv_u32 atr)
{
    fv_mat_t    *arr;
    fv_s32      min_step;

    min_step = FV_ELEM_SIZE(atr)*cols;
    if (min_step <= 0) {
        FV_LOG_ERR("Invalid matrix type(%d %d)\n", atr, FV_ELEM_SIZE(atr));
    }

    arr = fv_alloc(sizeof(*arr));

    fv_init_mat_header(arr, rows, cols, atr, NULL, min_step);

    return arr;
}

fv_mat_t *
fv_create_mat(fv_s32 height, fv_s32 width, fv_u32 atr)
{
    fv_mat_t    *arr;
    
    arr = fv_create_mat_header(height, width, atr);
    fv_create_data(arr);

    return arr;
}

fv_mat_t *
fv_mat_extract_image_coi(fv_image_t *image) 
{
    fv_mat_t    *mat;
    fv_roi_t    *roi;
    fv_u32      i;
    fv_u32      j;
    fv_u32      elem_size;
    fv_s32      offset;
    fv_s32      atr = FV_IMAGE_GET_TYPE(image);

    roi = image->ig_roi;
    FV_ASSERT(roi != NULL);
    if (roi->ri_coi < 0) {
        mat = fv_create_mat(roi->ri_height, roi->ri_width, atr);
    } else {
        mat = fv_create_mat(roi->ri_height, roi->ri_width, 
                FV_MAKETYPE(image->ig_depth, 1));
    }
    FV_ASSERT(mat != NULL);

    elem_size = FV_ELEM_SIZE(mat->mt_atr);
    offset = roi->ri_y_offset*image->ig_width_step + 
        roi->ri_x_offset*elem_size*image->ig_channels;
    if (roi->ri_coi < 0) {
        for (i = 0; i < roi->ri_height; i++) {
            memcpy(mat->mt_data.dt_ptr + i*mat->mt_step, 
                    image->ig_image_data + offset + i*image->ig_width_step,
                    mat->mt_step);
        }
    } else {
        FV_ASSERT(roi->ri_coi < image->ig_channels);
        for (i = 0; i < roi->ri_height; i++) {
            for (j = 0; j < roi->ri_width; j++) {
                memcpy(mat->mt_data.dt_ptr + i*mat->mt_step + j*elem_size,
                        image->ig_image_data + offset +
                        i*image->ig_width_step +
                        j*elem_size*image->ig_channels +
                        elem_size*roi->ri_coi, elem_size);
            }
        }
    }

    return mat;
}

static void  
fv_mat_dec_ref_data(fv_arr *arr)
{
    fv_mat_t    *mat;
    
    if (arr == NULL) {
        return;
    }

    mat = arr;
    mat->mt_data.dt_ptr = NULL;
    if (mat->mt_refcount != NULL && --*mat->mt_refcount == 0) {
        fv_free(&mat->mt_refcount);
    }
    mat->mt_refcount = NULL;
}

#if 0
void  
fv_matnd_dec_ref_data(fv_arr *arr)
{
    CvMatND* mat = (CvMatND*)arr;
    mat->data.ptr = NULL;
    if (mat->refcount != NULL && --*mat->refcount == 0)
        cvFree( &mat->refcount );
    mat->refcount = NULL;
}
#endif

#if 0
static fv_s32  
fv_matnd_inc_ref_data(fv_arr *arr)
{
    else if( CV_IS_MATND( arr ))
    {
        CvMatND* mat = (CvMatND*)arr;
        if( mat->refcount != NULL )
            refcount = ++*mat->refcount;
    }
    return refcount;
}
#endif

/* Increments CvMat data reference counter */
fv_s32  
fv_inc_ref_data(fv_arr *arr)
{
    fv_u32          type;

    type = *((fv_u32 *)arr);
    FV_ASSERT(type < FV_BASE_TYPE_MAX);

    if (fv_base_op[type].bo_inc_ref != NULL) {
        return fv_base_op[type].bo_inc_ref(arr);
    }

    return FV_ERROR;
}

void
_fv_release_mat(fv_mat_t *array)
{
    if (array == NULL) {
        return;
    }

    fv_dec_ref_data(array);
}

void
fv_release_mat(fv_mat_t **array)
{
    fv_mat_t    *arr;

    if (array == NULL) {
        FV_LOG_ERR("\n");
    }

    _fv_release_mat(*array);
    arr = *array;
    *array = NULL;
    fv_free(&arr);
}

#if 0
// Creates a copy of matrix
CV_IMPL CvMat*
cvCloneMat( const CvMat* src )
{
    if( !CV_IS_MAT_HDR( src ))
        CV_Error( CV_StsBadArg, "Bad CvMat header" );

    CvMat* dst = cvCreateMatHeader( src->rows, src->cols, src->type );

    if( src->data.ptr )
    {
        cvCreateData( dst );
        cvCopy( src, dst );
    }

    return dst;
}
#endif

void
fv_copy_mat(fv_mat_t *dst, fv_mat_t *src) 
{
    FV_ASSERT(dst->mt_atr == src->mt_atr &&
            dst->mt_rows == src->mt_rows &&
            dst->mt_cols == src->mt_cols);

    memcpy(dst->mt_data.dt_ptr, src->mt_data.dt_ptr, 
            dst->mt_rows*dst->mt_step);
}

double
fv_mget(fv_mat_t *mat, fv_s32 row, fv_s32 col, 
        fv_u16 channel, fv_u32 border_type)
{
    void        *data;
    double      v = 0;
    fv_s32      depth;
    fv_u32      elem_size;

    FV_ASSERT(channel < FV_MAX_CHANNEL);

    if (row < 0 || row >= mat->mt_rows || col < 0 ||
            col >= mat->mt_cols) {
        if (border_type == FV_BORDER_CONSTANT) {
            return 0;
        }

        row = fv_border_get_value(border_type, row, mat->mt_rows);
        col = fv_border_get_value(border_type, col, mat->mt_cols);
    }

    elem_size = FV_ELEM_SIZE(mat->mt_atr);
    data = mat->mt_data.dt_ptr + mat->mt_step*row + col*elem_size;
    depth = FV_MAT_DEPTH(mat);
    switch (depth) {
        case FV_DEPTH_1U:
        case FV_DEPTH_8U:
            v = *((fv_u8 *)data + channel);
            break;
        case FV_DEPTH_16U:
            v = *((fv_u16 *)data + channel);
            break;
        case FV_DEPTH_32F:
            v = *((float *)data + channel);
            break;
        case FV_DEPTH_64F:
            v = *((double *)data + channel);
            break;
        default:
            FV_LOG_ERR("Unknow depth type %d!\n", depth);
    } 

    return v;
}

void
fv_mset(fv_mat_t *mat, fv_s32 row, fv_s32 col, fv_u16 channel, double value)
{
    void        *data;
    fv_u32      depth;
    fv_u32      elem_size;

    FV_ASSERT((unsigned)row < (unsigned)mat->mt_rows &&
            (unsigned)col < (unsigned)mat->mt_cols);

    depth = FV_MAT_DEPTH(mat);
    elem_size = FV_ELEM_SIZE(mat->mt_atr);
    data = mat->mt_data.dt_ptr + mat->mt_step*row + col*elem_size;
    //printf("depth = %d, elem_size = %d, %d %d %d\n", depth, elem_size, mat->mt_atr , 
    //        mat->mt_atr & ~FV_DEPTH_SIGN, ~FV_DEPTH_SIGN);
    switch (depth) {
        case FV_DEPTH_1U:
        case FV_DEPTH_8U:
            if (value > 255) {
                *((fv_u8 *)data + channel) = 255;
                break;
            }
            *((fv_u8 *)data + channel) = value;
            break;
        case FV_DEPTH_16U:
            if (value > 65535) {
                *((fv_u16 *)data + channel) = 65535;
                break;
            }
            *((fv_u16 *)data + channel) = value;
            break;
        case FV_DEPTH_32F:
            *((float *)data + channel) = value;
            break;
        case FV_DEPTH_64F:
            *((double *)data + channel) = value;
            break;
        default:
            FV_LOG_ERR("Unknow depth type %d! atr =%x\n", depth, mat->mt_atr);
    } 
}

static void
fv_convert_mat_float_to_double(double *dst, float *src, fv_s32 total)
{
    fv_s32      i;

    for (i = 0; i < total; i++, dst++, src++) {
        *dst = *src;
    }
}

static void
fv_convert_mat_double_to_float(float *dst, double *src, fv_s32 total)
{
    fv_s32      i;

    for (i = 0; i < total; i++, dst++, src++) {
        *dst = *src;
    }
}

static void
fv_convert_mat_uchar_to_double(double *dst, fv_u8 *src, fv_s32 total)
{
    fv_s32      i;

    for (i = 0; i < total; i++, dst++, src++) {
        *dst = *src;
    }
}

static void
fv_convert_mat_double_to_uchar(fv_u8 *dst, double *src, fv_s32 total)
{
    fv_s32      i;

    for (i = 0; i < total; i++, dst++, src++) {
        if (*src < 0) {
            *dst = 0;
            continue;
        }
        if (*src > 255) {
            *dst = 255;
            continue;
        }
        *dst = *src;
    }
}

void
fv_convert_mat(fv_mat_t *dst, fv_mat_t *src) 
{
    void        *dst_data;
    void        *src_data;
    fv_u16      dst_depth;
    fv_u16      src_depth;
    fv_s32      total;

    FV_ASSERT(dst->mt_nchannel == src->mt_nchannel &&
            dst->mt_rows == src->mt_rows &&
            dst->mt_cols == src->mt_cols);

    dst_depth = dst->mt_idepth;
    src_depth = src->mt_idepth;
    if (dst_depth == src_depth) {
        fv_free(&dst->mt_data.dt_ptr);
        dst->mt_data.dt_ptr = src->mt_data.dt_ptr;
        fv_inc_ref_data(src);
        return;
    }
    total = dst->mt_total*dst->mt_nchannel;
    dst_data = dst->mt_data.dt_ptr;
    src_data = src->mt_data.dt_ptr;
    if (dst_depth == FV_DEPTH_64F && src_depth == FV_DEPTH_32F) {
        fv_convert_mat_float_to_double(dst_data, src_data, total);
        return;
    }
    if (dst_depth == FV_DEPTH_32F && src_depth == FV_DEPTH_64F) {
        fv_convert_mat_double_to_float(dst_data, src_data, total);
        return;
    }

    if (dst_depth == FV_DEPTH_64F && src_depth == FV_DEPTH_8U) {
        fv_convert_mat_uchar_to_double(dst_data, src_data, total);
        return;
    }

    if (dst_depth == FV_DEPTH_8U && src_depth == FV_DEPTH_64F) {
        fv_convert_mat_double_to_uchar(dst_data, src_data, total);
        return;
    }

    FV_LOG_ERR("Unknow convert format!\n");
}

