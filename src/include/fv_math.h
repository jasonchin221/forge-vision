#ifndef __FV_MATH_H__
#define __FV_MATH_H__

#include <math.h>
#include <stdlib.h>

#include "fv_types.h"

#define fv_pi           3.14159265
#define fv_short_max    32767
#define fv_short_min    (-32768)
#define fv_int_max      2147483647
#define fv_int_min      (-2147483648)

#define fv_min(a, b) ((a) < (b) ? (a) : (b))
#define fv_max(a, b) ((a) > (b) ? (a) : (b))

#define fv_align(len, align_num) \
    (len + (align_num - (len % align_num))%align_num)

#define fv_float_eq(f1, f2) (fabsf(f1 - f2) <= FLT_MIN)
#define fv_float_is_zero(f) fv_float_eq(f, 0)

#define fv_double_eq(d1, d2) (fabs(d1 - d2) <= DBL_MIN)
#define fv_double_is_zero(d) fv_double_eq(d, 0)

#define fv_rand()  ((double)(rand())/RAND_MAX)

#define fv_saturate_cast(v, max, min) \
    ({\
        typeof(v)   ret;\
        if (v > max) { \
            ret = max; \
        } else if ( v < min) { \
            ret = min; \
        } else { \
            ret = v; \
        } \
        ret;\
     })

#define fv_saturate_cast_8u(v) fv_saturate_cast(v, 255, 0)
#define fv_saturate_cast_8s(v) fv_saturate_cast(v, 127, -128)
#define fv_saturate_cast_16u(v) fv_saturate_cast(v, 65535, 0)
#define fv_saturate_cast_16s(v) fv_saturate_cast(v, fv_short_max, fv_short_min)
#define fv_saturate_cast_32s(v) fv_saturate_cast(v, fv_int_max, fv_int_min)
#define fv_saturate_cast_32f(v) fv_saturate_cast(v, FLT_MAX, -FLT_MAX)

#define fv_exchange_ptr(ptr1, ptr2) \
    do { \
        void    *tmp; \
        tmp = ptr1; \
        ptr1 = ptr2; \
        ptr2 = tmp; \
    } while(0)

#endif
