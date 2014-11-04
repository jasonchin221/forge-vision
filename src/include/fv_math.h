#ifndef __FV_MATH_H__
#define __FV_MATH_H__

#include <math.h>
#include <stdlib.h>

#include "fv_types.h"

#define fv_pi       3.14159265

#define fv_min(a, b) ((a) < (b) ? (a) : (b))
#define fv_max(a, b) ((a) > (b) ? (a) : (b))

#define fv_align(len, align_num) \
    (len + (align_num - (len % align_num))%align_num)

#define fv_float_eq(f1, f2) (fabsf(f1 - f2) <= 0.000001)
#define fv_float_is_zero(f) fv_float_eq(f, 0)

#define fv_rand()  ((double)(rand())/RAND_MAX)

#endif
