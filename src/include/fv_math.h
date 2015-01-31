#ifndef __FV_MATH_H__
#define __FV_MATH_H__

#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <limits.h>
#include <float.h>

#include "fv_types.h"

#define fv_pi           3.1415926535897932384626433832795
#define fv_log2         0.69314718055994530941723212145818
#define fv_short_max    32767
#define fv_short_min    (-32768)
#define fv_int_max      INT_MAX
#define fv_int_min      (-INT_MAX)

#define fv_min(a, b) ((a) < (b) ? (a) : (b))
#define fv_max(a, b) ((a) > (b) ? (a) : (b))
#define fv_swap(a, b) \
    do { \
        double  t; \
        t = a; \
        a = b; \
        b = t; \
    } while(0)

#define FV_SWAP(a,b,t) ((t) = (a), (a) = (b), (b) = (t))

#define fv_align(len, align_num) \
    (len + (align_num - (len % align_num))%align_num)

#define fv_float_eq(f1, f2) (fabsf(f1 - f2) < FLT_EPSILON)
#define fv_float_is_zero(f) fv_float_eq(f, 0)

#define fv_double_eq(d1, d2) (fabs(d1 - d2) < DBL_EPSILON)
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

#define fv_saturate_cast_max(v, max) \
    ({\
        typeof(v)   ret;\
        if (v > max) { \
            ret = max; \
        } else { \
            ret = v; \
        } \
        ret;\
     })

#define fv_saturate_cast_min(v, min) \
    ({\
        typeof(v)   ret;\
        if (v < min) { \
            ret = min; \
        } else { \
            ret = v; \
        } \
        ret;\
     })


#define fv_saturate_no_cast(v) v
#define fv_saturate_cast_8u(v) fv_saturate_cast(v, 255, 0)
#define fv_saturate_cast_8s(v) fv_saturate_cast(v, 127, -128)
#define fv_saturate_cast_16u(v) fv_saturate_cast(v, 65535, 0)
#define fv_saturate_cast_16u_min(v) fv_saturate_cast_min(v, 0)
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

#define FV_IMPLEMENT_QSORT_EX( func_name, T, LT, user_data_type )                   \
void func_name( T *array, size_t total, user_data_type aux )                        \
{                                                                                   \
    int isort_thresh = 7;                                                           \
    T t;                                                                            \
    int sp = 0;                                                                     \
                                                                                    \
    struct                                                                          \
    {                                                                               \
        T *lb;                                                                      \
        T *ub;                                                                      \
    }                                                                               \
    stack[48];                                                                      \
                                                                                    \
    aux = aux;                                                                      \
                                                                                    \
    if( total <= 1 )                                                                \
        return;                                                                     \
                                                                                    \
    stack[0].lb = array;                                                            \
    stack[0].ub = array + (total - 1);                                              \
                                                                                    \
    while( sp >= 0 )                                                                \
    {                                                                               \
        T* left = stack[sp].lb;                                                     \
        T* right = stack[sp--].ub;                                                  \
                                                                                    \
        for(;;)                                                                     \
        {                                                                           \
            int i, n = (int)(right - left) + 1, m;                                  \
            T* ptr;                                                                 \
            T* ptr2;                                                                \
                                                                                    \
            if( n <= isort_thresh )                                                 \
            {                                                                       \
            insert_sort:                                                            \
                for( ptr = left + 1; ptr <= right; ptr++ )                          \
                {                                                                   \
                    for( ptr2 = ptr; ptr2 > left && LT(ptr2[0],ptr2[-1]); ptr2--)   \
                        FV_SWAP( ptr2[0], ptr2[-1], t );                            \
                }                                                                   \
                break;                                                              \
            }                                                                       \
            else                                                                    \
            {                                                                       \
                T* left0;                                                           \
                T* left1;                                                           \
                T* right0;                                                          \
                T* right1;                                                          \
                T* pivot;                                                           \
                T* a;                                                               \
                T* b;                                                               \
                T* c;                                                               \
                int swap_cnt = 0;                                                   \
                                                                                    \
                left0 = left;                                                       \
                right0 = right;                                                     \
                pivot = left + (n/2);                                               \
                                                                                    \
                if( n > 40 )                                                        \
                {                                                                   \
                    int d = n / 8;                                                  \
                    a = left, b = left + d, c = left + 2*d;                         \
                    left = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))     \
                                      : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));    \
                                                                                    \
                    a = pivot - d, b = pivot, c = pivot + d;                        \
                    pivot = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))    \
                                      : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));    \
                                                                                    \
                    a = right - 2*d, b = right - d, c = right;                      \
                    right = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))    \
                                      : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));    \
                }                                                                   \
                                                                                    \
                a = left, b = pivot, c = right;                                     \
                pivot = LT(*a, *b) ? (LT(*b, *c) ? b : (LT(*a, *c) ? c : a))        \
                                   : (LT(*c, *b) ? b : (LT(*a, *c) ? a : c));       \
                if( pivot != left0 )                                                \
                {                                                                   \
                    FV_SWAP( *pivot, *left0, t );                                   \
                    pivot = left0;                                                  \
                }                                                                   \
                left = left1 = left0 + 1;                                           \
                right = right1 = right0;                                            \
                                                                                    \
                for(;;)                                                             \
                {                                                                   \
                    while( left <= right && !LT(*pivot, *left) )                    \
                    {                                                               \
                        if( !LT(*left, *pivot) )                                    \
                        {                                                           \
                            if( left > left1 )                                      \
                                FV_SWAP( *left1, *left, t );                        \
                            swap_cnt = 1;                                           \
                            left1++;                                                \
                        }                                                           \
                        left++;                                                     \
                    }                                                               \
                                                                                    \
                    while( left <= right && !LT(*right, *pivot) )                   \
                    {                                                               \
                        if( !LT(*pivot, *right) )                                   \
                        {                                                           \
                            if( right < right1 )                                    \
                                FV_SWAP( *right1, *right, t );                      \
                            swap_cnt = 1;                                           \
                            right1--;                                               \
                        }                                                           \
                        right--;                                                    \
                    }                                                               \
                                                                                    \
                    if( left > right )                                              \
                        break;                                                      \
                    FV_SWAP( *left, *right, t );                                    \
                    swap_cnt = 1;                                                   \
                    left++;                                                         \
                    right--;                                                        \
                }                                                                   \
                                                                                    \
                if( swap_cnt == 0 )                                                 \
                {                                                                   \
                    left = left0, right = right0;                                   \
                    goto insert_sort;                                               \
                }                                                                   \
                                                                                    \
                n = MIN( (int)(left1 - left0), (int)(left - left1) );               \
                for( i = 0; i < n; i++ )                                            \
                    FV_SWAP( left0[i], left[i-n], t );                              \
                                                                                    \
                n = MIN( (int)(right0 - right1), (int)(right1 - right) );           \
                for( i = 0; i < n; i++ )                                            \
                    FV_SWAP( left[i], right0[i-n+1], t );                           \
                n = (int)(left - left1);                                            \
                m = (int)(right1 - right);                                          \
                if( n > 1 )                                                         \
                {                                                                   \
                    if( m > 1 )                                                     \
                    {                                                               \
                        if( n > m )                                                 \
                        {                                                           \
                            stack[++sp].lb = left0;                                 \
                            stack[sp].ub = left0 + n - 1;                           \
                            left = right0 - m + 1, right = right0;                  \
                        }                                                           \
                        else                                                        \
                        {                                                           \
                            stack[++sp].lb = right0 - m + 1;                        \
                            stack[sp].ub = right0;                                  \
                            left = left0, right = left0 + n - 1;                    \
                        }                                                           \
                    }                                                               \
                    else                                                            \
                        left = left0, right = left0 + n - 1;                        \
                }                                                                   \
                else if( m > 1 )                                                    \
                    left = right0 - m + 1, right = right0;                          \
                else                                                                \
                    break;                                                          \
            }                                                                       \
        }                                                                           \
    }                                                                               \
}

#define fv_complex_multiply(r1, i1, r2, i2, r, i) \
    do { \
        r = (r1)*(r2) - (i1)*(i2); \
        i = (r1)*(i2) + (r2)*(i1); \
    } while(0)

static inline fv_u32
fv_num_map(fv_u32 n, fv_u8 b)
{
     fv_u32     ret = 0;
     fv_u32     i;
     fv_u32     j; 
     fv_u32     k;

     for (i = 0; i < b; i++) {
        j = (1 << i);
        k = (1 << (b - 1 - i));
        if (n & j) {
            ret |= k;
        }
     }

     return ret;
}


extern void fv_pow(fv_mat_t *dst, fv_mat_t *src, double pow);

#endif
