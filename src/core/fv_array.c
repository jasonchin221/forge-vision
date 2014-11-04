#include <string.h>
#include <opencv/cv.h>  
#include <opencv/highgui.h>

#include "fv_types.h"
#include "fv_log.h"
#include "fv_mem.h"
#include "fv_math.h"

void
fv_create_data(fv_arr *arr)
{
    fv_image_t      *img;

    if (FV_IS_IMAGE_HDR(arr)) {
        img = arr;
        if (img->ig_image_data != NULL) {
            FV_LOG_ERR("Data is already allocated\n");
        }

        img->ig_image_data = img->ig_image_data_origin =
            fv_alloc(img->ig_image_size);

        if (img->ig_image_data == NULL) {
            FV_LOG_ERR("Data alloc failed!\n");
        }

        return;
    }

    FV_LOG_ERR("Unrecognized or unsupported array type!\n");
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
    img->ig_size = sizeof(*img);

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
        FV_LOG_ERR("Unsupported format\n");
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
    img->ig_align = align;
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

void
fv_release_data(fv_arr *arr)
{
    fv_image_t      *img;
    fv_s8           *ptr;

    if (FV_IS_IMAGE_HDR(arr)) {
        img = arr;
        ptr = img->ig_image_data_origin;
        img->ig_image_data = img->ig_image_data_origin = NULL;
        fv_free(&ptr);
        return;
    }

    FV_LOG_ERR("Unrecognized or unsupported array type\n");
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

fv_size_t
fv_get_size(fv_arr *arr)
{
    fv_image_t      *img;
    fv_size_t       size;

    if (FV_IS_IMAGE_HDR(arr)) {
        img = arr;
        if (img->ig_roi) {
            size.sz_width = img->ig_roi->ri_width;
            size.sz_height = img->ig_roi->ri_width;
        } else {
            size.sz_width = img->ig_width;
            size.sz_height = img->ig_height;
        }
    } else {
        FV_LOG_ERR("Unrecognized or unsupported array type\n");
    }

    return size;
}

fv_image_t *
fv_convert_image(IplImage *cv_img) 
{
    fv_image_t      *img;
    fv_size_t       size;

    size = fv_size(cv_img->width, cv_img->height);

    img = fv_create_image(size, cv_img->depth, cv_img->nChannels);
    if (img == NULL) {
        return NULL;
    }

    memcpy(img->ig_image_data, cv_img->imageData, 
            img->ig_image_size);

    return img;
}

