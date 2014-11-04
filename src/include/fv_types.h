#ifndef __FV_TYPES_H__
#define __FV_TYPES_H__

#include <stdbool.h>
#include <stddef.h>

#define FV_OK                           0
#define FV_ERROR                        (-1)
#define FV_ORIGIN_TL                    0   /* Top Left is origin */
#define FV_ORIGIN_BL                    1   /* Bottom Left is origin */
#define FV_DEFAULT_IMAGE_ROW_ALIGN      4   /* default image row align (in bytes) */

#define FV_DEPTH_SIGN                   0x80000000

#define FV_DEPTH_1U                     1
#define FV_DEPTH_8U                     8
#define FV_DEPTH_16U                    16
#define FV_DEPTH_32F                    32

#define FV_DEPTH_8S                     (FV_DEPTH_SIGN| 8)
#define FV_DEPTH_16S                    (FV_DEPTH_SIGN|16)
#define FV_DEPTH_32S                    (FV_DEPTH_SIGN|32)
#define FV_DEPTH_64F                    64

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

typedef struct _fv_point_t {
    fv_s32          pt_x;
    fv_s32          pt_y;
} fv_point_t;

typedef struct _fv_size_t {
    fv_s32          sz_width;
    fv_s32          sz_height;
} fv_size_t;

typedef struct _fv_size_2D32f_t {
    float       sf_width;
    float       sf_height;
} fv_size_2D32f_t;

typedef struct _fv_roi_t {
    fv_s32      ri_coi; /* 0 - no COI (all channels are selected), 1 - 0th channel is selected ...*/
    fv_s32      ri_x_offset;
    fv_s32      ri_y_offset;
    fv_s32      ri_width;
    fv_s32      ri_height;
} fv_roi_t;

typedef struct _fv_image_t {
    fv_s32      ig_size;        /* sizeof(fv_image_t) */
    fv_s32      ig_channels;    /* Most of OpenCV functions support 1,2,3 or 4 channels */
    fv_s32      ig_depth;       /* Pixel depth in bits: 
                                   FV_DEPTH_8U, FV_DEPTH_8S, FV_DEPTH_16S,
                                   FV_DEPTH_32S, FV_DEPTH_32F and FV_DEPTH_64F are supported.  */
    fv_s32      ig_data_order;   /* 0 - interleaved color channels, 1 - separate color channels.
                                    cvCreateImage can only create interleaved images */
    fv_s32      ig_origin;       /* 0 - top-left origin,
                                    1 - bottom-left origin (Windows bitmaps style).  */
    fv_s32      ig_align;        /* Alignment of image rows (4 or 8).
                                    OpenCV ignores it and uses widthStep instead.    */
    fv_s32      ig_width;        /* Image width in pixels.                           */
    fv_s32      ig_height;       /* Image height in pixels.                          */
    fv_roi_t    *ig_roi;         /* Image ROI. If NULL, the whole image is selected. */
    fv_s32      ig_image_size;         /* Image data size in bytes
                                            (==image->height*image->widthStep
                                            in case of interleaved data)*/
    fv_s8       *ig_image_data;        /* Pointer to aligned image data.         */
    fv_s32      ig_width_step;         /* Size of aligned image row in bytes.    */
    fv_s8       *ig_image_data_origin;  /* Pointer to very origin of image data
                                            (not necessarily aligned) -
                                            needed for correct deallocation */
} fv_image_t;

#define FV_IS_IMAGE_HDR(img) \
    ((img) != NULL && \
     ((const fv_image_t *)(img))->ig_size == sizeof(fv_image_t))

#define FV_IS_IMAGE(img) \
    (FV_IS_IMAGE_HDR(img) && ((fv_image_t *)img)->ig_image_data != NULL)


static inline fv_size_t 
fv_size(fv_s32 width, fv_s32 height)
{
    fv_size_t   s;

    s.sz_width = width;
    s.sz_height = height;

    return s;
}

static inline fv_size_2D32f_t 
fv_size_2D32f(double width, double height)
{
    fv_size_2D32f_t     s;

    s.sf_width = (float)width;
    s.sf_height = (float)height;

    return s;
}

#endif
