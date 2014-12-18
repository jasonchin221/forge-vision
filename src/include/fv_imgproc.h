#ifndef __FV_IMGPROC_H__
#define __FV_IMGPROC_H__

#include "fv_types.h"

/*
 *  Various border types, image boundaries are denoted with '|'
 *
 * FV_BORDER_CONSTANT:      iiiiii|abcdefgh|iiiiiii  with some specified 'i'
 * FV_BORDER_REPLICATE:     aaaaaa|abcdefgh|hhhhhhh
 * FV_BORDER_REFLECT:       fedcba|abcdefgh|hgfedcb
 * FV_BORDER_REFLECT_101:   gfedcb|abcdefgh|gfedcba
 * FV_BORDER_WRAP:          cdefgh|abcdefgh|abcdefg
 */
enum { 
    FV_BORDER_CONSTANT = 0,
    FV_BORDER_REPLICATE,
    FV_BORDER_REFLECT,
    FV_BORDER_WRAP,
    FV_BORDER_REFLECT_101 = 4, 
    FV_BORDER_REFLECT101 = FV_BORDER_REFLECT_101,
    FV_BORDER_TRANSPARENT = 5,
    FV_BORDER_DEFAULT = FV_BORDER_REFLECT_101, 
    FV_BORDER_ISOLATED = 16,
    FV_BORDER_MAX,
};

enum {
    FV_THRESH_BINARY      = 0,  /* value = value > threshold ? max_value : 0       */
    FV_THRESH_BINARY_INV  = 1,  /* value = value > threshold ? 0 : max_value       */
    FV_THRESH_TRUNC       = 2,  /* value = value > threshold ? threshold : value   */
    FV_THRESH_TOZERO      = 3,  /* value = value > threshold ? value : 0           */
    FV_THRESH_TOZERO_INV  = 4,  /* value = value > threshold ? 0 : value           */
    FV_THRESH_MASK        = 7,
    FV_THRESH_OTSU        = 8  /* use Otsu algorithm to choose the optimal threshold value;
                                 combine the flag with one of the above FV_THRESH_* values */
};

enum {
    FV_KERNEL_GENERAL = 0, 
    FV_KERNEL_SYMMETRICAL = 1, 
    FV_KERNEL_ASYMMETRICAL = 2,                                      
    FV_KERNEL_SMOOTH = 4,
    FV_KERNEL_INTEGER = 8,
};


#endif
