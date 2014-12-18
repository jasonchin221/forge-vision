#include <math.h>

#include "fv_types.h"
#include "fv_core.h"
#include "fv_edge.h"
#include "fv_debug.h"
#include "fv_log.h"
#include "fv_filter.h"
#include "fv_smooth.h"
#include "fv_time.h"

static void 
fv_calc_eigen_val(fv_mat_t *dst, fv_mat_t *cov, double k, 
            float (*calc)(float *, float, float, float, float))
{
    float       *dst_data;
    float       *cov_data;
    fv_s32      row;
    fv_s32      col;
    float       a;
    float       b;
    float       c;

    dst_data = dst->mt_data.dt_fl;
    for (row = 0; row < cov->mt_rows; row++) {
        cov_data = (float *)(cov->mt_data.dt_ptr + cov->mt_step*row);
        for (col = 0; col < cov->mt_cols; col++, dst_data++) {
            a = cov_data[col*3];
            b = cov_data[col*3 + 1];
            c = cov_data[col*3 + 2];
            *dst_data = calc(cov_data, k, a, b, c);
        }
    }
}

static float 
_fv_calc_min_eigen_val(float *cov, float k, float a, float b, float c)
{
    a *= 0.5;
    c *= 0.5;
    return (float)((a + c) - sqrt((a - c)*(a - c) + b*b));
}

static void 
fv_calc_min_eigen_val(fv_mat_t *dst, fv_mat_t *cov, double k)
{
    fv_calc_eigen_val(dst, cov, k, _fv_calc_min_eigen_val);
}

static float 
_fv_calc_harris(float *cov, float k, float a, float b, float c)
{
    return (float)(a*c - b*b - k*(a + c)*(a + c));
}

static void 
fv_calc_harris(fv_mat_t *dst, fv_mat_t *cov, double k)
{
    fv_calc_eigen_val(dst, cov, k, _fv_calc_harris);
}

#if 0
void eigen2x2( const float* cov, float* dst, int n )
{
    for( int j = 0; j < n; j++ )
    {
        double a = cov[j*3];
        double b = cov[j*3+1];
        double c = cov[j*3+2];

        double u = (a + c)*0.5;
        double v = std::sqrt((a - c)*(a - c)*0.25 + b*b);
        double l1 = u + v;
        double l2 = u - v;

        double x = b;
        double y = l1 - a;
        double e = fabs(x);

        if( e + fabs(y) < 1e-4 )
        {
            y = b;
            x = l1 - c;
            e = fabs(x);
            if( e + fabs(y) < 1e-4 )
            {
                e = 1./(e + fabs(y) + FLT_EPSILON);
                x *= e, y *= e;
            }
        }

        double d = 1./std::sqrt(x*x + y*y + DBL_EPSILON);
        dst[6*j] = (float)l1;
        dst[6*j + 2] = (float)(x*d);
        dst[6*j + 3] = (float)(y*d);

        x = b;
        y = l2 - a;
        e = fabs(x);

        if( e + fabs(y) < 1e-4 )
        {
            y = b;
            x = l2 - c;
            e = fabs(x);
            if( e + fabs(y) < 1e-4 )
            {
                e = 1./(e + fabs(y) + FLT_EPSILON);
                x *= e, y *= e;
            }
        }

        d = 1./std::sqrt(x*x + y*y + DBL_EPSILON);
        dst[6*j + 1] = (float)l2;
        dst[6*j + 4] = (float)(x*d);
        dst[6*j + 5] = (float)(y*d);
    }
}


static void 
fv_calc_eigen_vals_vecs(fv_mat_t *dst, fv_mat_t *cov, double k)
{
    Size size = _cov.size();
    if( _cov.isContinuous() && _dst.isContinuous() )
    {
        size.width *= size.height;
        size.height = 1;
    }

    for( int i = 0; i < size.height; i++ )
    {
        const float* cov = (const float*)(_cov.data + _cov.step*i);
        float* dst = (float*)(_dst.data + _dst.step*i);

        eigen2x2(cov, dst, size.width);
    }

}
#endif

static void
fv_corner_eigen_vals_vecs(fv_mat_t *dst, fv_mat_t *src, 
                    fv_s32 block_size, fv_s32 aperture_size, 
                    double k, fv_s32 border_type, 
                    void (*calc_eigen_val)(fv_mat_t *, fv_mat_t *, double))
{
    fv_mat_t        *dx;
    fv_mat_t        *dy;
    fv_mat_t        *cov;
    fv_mat_t        *cov2;
    float           *cov_data; 
    float           *dxdata;
    float           *dydata;
    double          scale;
    float           dx_data;
    float           dy_data;
    fv_u32          atr;
    fv_s32          depth;
    fv_s32          i;
    fv_s32          j;
    
    FV_ASSERT(dst->mt_atr == FV_32FC1 && dst->mt_rows == src->mt_rows &&
            dst->mt_cols == src->mt_cols);

    scale = (double)(1 << ((aperture_size > 0 ? 
                    aperture_size : 3) - 1)) * block_size;

    if (aperture_size < 0) {
        scale *= 2;
    }

    depth = FV_MAT_DEPTH(src);
    if (depth == FV_DEPTH_8U) {
        scale *= 255;
    }

    scale = 1.0/scale;

    FV_ASSERT(src->mt_atr == FV_8UC1 || src->mt_atr == FV_32FC1);
    atr = FV_32FC(FV_MAT_NCHANNEL(src));
    dx = fv_create_mat(src->mt_rows, src->mt_cols, atr);
    FV_ASSERT(dx != NULL);

    dy = fv_create_mat(src->mt_rows, src->mt_cols, atr);
    FV_ASSERT(dy != NULL);

    FV_LOG_PRINT("scale = %f\n", scale);
    if (aperture_size > 0) {
        _fv_sobel(dx, src, FV_DEPTH_32F, 1, 0, aperture_size, 
                scale, 0, border_type);
        _fv_sobel(dy, src, FV_DEPTH_32F, 0, 1, aperture_size, 
                scale, 0, border_type);
    } else {
        _fv_scharr(dx, src, FV_DEPTH_32F, 1, 0, scale, 0, border_type);
        _fv_scharr(dy, src, FV_DEPTH_32F, 0, 1, scale, 0, border_type);
    }

    cov = fv_create_mat(src->mt_rows, src->mt_cols, FV_32FC3);
    FV_ASSERT(cov != NULL);
    dxdata = dx->mt_data.dt_fl;
    dydata = dy->mt_data.dt_fl;
    for (i = 0; i < src->mt_rows; i++) {
        cov_data = (float *)(cov->mt_data.dt_ptr + i*cov->mt_step);
        for (j = 0; j < src->mt_cols; j++, dxdata++, dydata++) {
            dx_data = *dxdata;
            dy_data = *dydata;
            cov_data[j*3] = dx_data*dx_data;
            cov_data[j*3 + 1] = dx_data*dy_data;
            cov_data[j*3 + 2] = dy_data*dy_data;
        }
    }

    fv_release_mat(&dy);
    fv_release_mat(&dx);

    cov2 = fv_create_mat(cov->mt_rows, cov->mt_cols, cov->mt_atr);
    FV_ASSERT(cov2 != NULL);
    fv_box_filter(cov2, cov, FV_MAT_DEPTH(cov), fv_size(block_size, block_size),
        fv_point(-1, -1), 0, border_type);
    fv_release_mat(&cov);
    calc_eigen_val(dst, cov2, k);
    fv_release_mat(&cov2);
}

void 
fv_corner_min_eigen_val(fv_mat_t *dst, fv_mat_t *src, 
        fv_s32 block_size, fv_s32 ksize, fv_s32 border_type)
{
    fv_corner_eigen_vals_vecs(dst, src, block_size, ksize,
                0, border_type, fv_calc_min_eigen_val);
}

void
fv_corner_harris(fv_mat_t *dst, fv_mat_t *src, fv_s32 block_size, 
        fv_s32 ksize, double k, fv_s32 border_type)
{
    fv_corner_eigen_vals_vecs(dst, src, block_size, ksize, k, 
            border_type, fv_calc_harris);
}

