
#include "fv_types.h"
#include "fv_core.h"
#include "fv_math.h"
#include "fv_log.h"
#include "fv_mem.h"
#include "fv_filter.h"
#include "fv_debug.h"
#include "fv_time.h"

static void 
_fv_get_scharr_kernels(fv_mat_t **_kx, fv_mat_t **_ky,
        fv_s32 dx, fv_s32 dy, fv_bool normalize, fv_s32 ktype)
{
    fv_mat_t    *kx;
    fv_mat_t    *ky;
    fv_mat_t    *kernel;
    float       *ker_i;
    fv_s32      k;
    fv_s32      ksize = 3;
    fv_s32      order;
    fv_s32      atr;

    FV_ASSERT(dx >= 0 && dy >= 0 && dx + dy > 0);

    atr = FV_MAKETYPE(FV_DEPTH_32S, 1);
    kx = fv_create_mat(ksize, 1, atr);
    FV_ASSERT(kx != NULL);
    ky = fv_create_mat(ksize, 1, atr);
    FV_ASSERT(ky != NULL);

    for (k = 0; k < 2; k++) {
        kernel = k == 0 ? kx : ky;
        order = k == 0 ? dx : dy;
        ker_i = kernel->mt_data.dt_fl;
        if (order == 0) {
            ker_i[0] = 3, ker_i[1] = 10, ker_i[2] = 3;
        } else if (order == 1) {
            ker_i[0] = -1, ker_i[1] = 0, ker_i[2] = 1;
        }
    }
    *_kx = kx;
    *_ky = ky;
}

static void 
fv_get_scharr_kernels(fv_mat_t **_kx, fv_mat_t **_ky,
        fv_s32 dx, fv_s32 dy, fv_s32 ksize, 
        fv_bool normalize, fv_s32 ktype)
{
    _fv_get_scharr_kernels(_kx, _ky, dx, dy, normalize, ktype);
}

static void 
fv_get_sobel_kernels(fv_mat_t **_kx, fv_mat_t **_ky,
        fv_s32 dx, fv_s32 dy, fv_s32 ksize, 
        fv_bool normalize, fv_s32 ktype)
{
    fv_mat_t    *kx;
    fv_mat_t    *ky;
    fv_mat_t    *kernel;
    float       *ker_i;
    fv_s32      i;
    fv_s32      j;
    fv_s32      k;
    fv_s32      k_size;
    fv_s32      ksize_x;
    fv_s32      ksize_y;
    fv_s32      order;
    fv_s32      oldval;
    fv_s32      newval;
    fv_s32      atr;

    if ((ksize & 0x1) == 0 || ksize > 31) {
        FV_LOG_ERR("The kernel size must be odd and not larger than 31(%d)\n",
                ksize);
    }

    FV_ASSERT(dx >= 0 && dy >= 0 && dx + dy > 0);

    ksize_x = ksize_y = ksize;
    if (ksize_x == 1 && dx > 0) {
        ksize_x = 3;
    }

    if (ksize_y == 1 && dy > 0) {
        ksize_y = 3;
    }

//    CV_Assert( ktype == CV_32F || ktype == CV_64F );

    atr = FV_MAKETYPE(FV_DEPTH_32S, 1);
    kx = fv_create_mat(ksize_x, 1, atr);
    FV_ASSERT(kx != NULL);
    ky = fv_create_mat(ksize_y, 1, atr);
    FV_ASSERT(ky != NULL);

    ker_i = fv_alloc(sizeof(*ker_i)*(fv_max(ksize_x, ksize_y) + 1));
    FV_ASSERT(ker_i != NULL);
    for (k = 0; k < 2; k++) {
        kernel = k == 0 ? kx : ky;
        order = k == 0 ? dx : dy;
        k_size = k == 0 ? ksize_x : ksize_y;

        FV_ASSERT(k_size > order);

        if (ksize == 1) {
            ker_i[0] = 1;
        } else if (ksize == 3) {
            if (order == 0) {
                ker_i[0] = 1, ker_i[1] = 2, ker_i[2] = 1;
            } else if( order == 1 ) {
                ker_i[0] = -1, ker_i[1] = 0, ker_i[2] = 1;
            } else {
                ker_i[0] = 1, ker_i[1] = -2, ker_i[2] = 1;
            }
        } else {
            ker_i[0] = 1;
            for (i = 0; i < ksize; i++) {
                ker_i[i + 1] = 0;
            }

            for (i = 0; i < ksize - order - 1; i++) {
                oldval = ker_i[0];
                for (j = 1; j <= ksize; j++) {
                    newval = ker_i[j] + ker_i[j - 1];
                    ker_i[j - 1] = oldval;
                    oldval = newval;
                }
            }

            for (i = 0; i < order; i++){
                oldval = -ker_i[0];
                for (j = 1; j <= ksize; j++) {
                    newval = ker_i[j - 1] - ker_i[j];
                    ker_i[j - 1] = oldval;
                    oldval = newval;
                }
            }
        }
        memcpy(kernel->mt_data.dt_fl, ker_i, kernel->mt_rows*kernel->mt_step);
    }

    fv_free(&ker_i);
    *_kx = kx;
    *_ky = ky;
}

void 
fv_get_deriv_kernels(fv_mat_t **kx, fv_mat_t **ky, fv_s32 dx, fv_s32 dy,
                          fv_s32 ksize, fv_bool normalize, fv_s32 ktype)
{
    if (ksize <= 0) {
        fv_get_scharr_kernels(kx, ky, dx, dy, ksize, normalize, ktype);
    } else {
        fv_get_sobel_kernels(kx, ky, dx, dy, ksize, normalize, ktype);
    }
}

static void
fv_edge_filter(fv_mat_t *dst, fv_mat_t *src, fv_s32 ddepth, fv_s32 dx, 
                fv_s32 dy, fv_s32 ksize, double scale, double delta, 
                fv_s32 border_type, void (*get_kernels)(fv_mat_t **, fv_mat_t **,
                    fv_s32, fv_s32, fv_s32, fv_bool, fv_s32))
{
    fv_mat_t    *kx;
    fv_mat_t    *ky;
    fv_s32      ktype;// = std::max(CV_32F, std::max(ddepth, src.depth()));

    ktype = fv_max(ddepth, FV_MAT_DEPTH(src));
    get_kernels(&kx, &ky, dx, dy, ksize, false, ktype);
    if (scale != 1) {
        // usually the smoothing part is the slowest to compute,
        // so try to scale it instead of the faster differenciating part
        if (dx == 0) {
            fv_mset_scale((*kx), scale, float);
        } else {
            fv_mset_scale((*ky), scale, float);
        }
    }

    FV_LOG_PRINT("kx[%f %f %f], ky[%f %f %f]\n", 
            kx->mt_data.dt_fl[0], kx->mt_data.dt_fl[1],kx->mt_data.dt_fl[2],
            ky->mt_data.dt_fl[0], ky->mt_data.dt_fl[1],ky->mt_data.dt_fl[2]);
    fv_sep_filter2D(dst, src, kx, ky, fv_point(-1, -1), 
            delta, border_type);
    fv_release_mat(&ky);
    fv_release_mat(&kx);

    fv_debug_save_img("Sobel", dst);
}

void 
_fv_sobel(fv_mat_t *dst, fv_mat_t *src, fv_s32 ddepth, fv_s32 dx, 
                fv_s32 dy, fv_s32 ksize, double scale, double delta, 
                fv_s32 border_type)
{
    fv_edge_filter(dst, src, ddepth, dx, dy, ksize, scale, delta, 
            border_type, fv_get_deriv_kernels);
}

void
fv_sobel(fv_image_t *dst, fv_image_t *src, fv_s32 dx, 
                fv_s32 dy, fv_s32 aperture_size)
{
    fv_mat_t    _dst;
    fv_mat_t    _src;

    FV_ASSERT(src->ig_image_size == dst->ig_image_size && 
            src->ig_channels == dst->ig_channels);

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);
    _fv_sobel(&_dst, &_src, src->ig_depth, dx, dy, aperture_size, 1, 0, 0);
}

void
_fv_scharr(fv_mat_t *dst, fv_mat_t *src, fv_s32 ddepth, fv_s32 dx, 
                fv_s32 dy, double scale, double delta, fv_s32 border_type)
{
    fv_edge_filter(dst, src, ddepth, dx, dy, 0, scale, delta, border_type, 
            fv_get_scharr_kernels);
}

#define fv_grad_add_core(dst, src1, src2, total, castop) \
    do { \
        fv_s32      i; \
        \
        for (i = 0; i < total; i++, dst++, src1++, src2++) { \
            *dst = castop((*src1 + *src2)); \
        } \
    } while(0)


static void
fv_grad_add_8u(fv_u8 *dst, double *src1, double *src2, fv_u32 total)
{
    fv_grad_add_core(dst, src1, src2, total, fv_saturate_cast_8u);
}

static void
fv_grad_add_8s(fv_s8 *dst, double *src1, double *src2, fv_u32 total)
{
    fv_grad_add_core(dst, src1, src2, total, fv_saturate_cast_8u);
}

static void
fv_grad_add_16u(fv_u16 *dst, double *src1, double *src2, fv_u32 total)
{
    fv_grad_add_core(dst, src1, src2, total, fv_saturate_cast_8u);
}

static void
fv_grad_add_16s(fv_s16 *dst, double *src1, double *src2, fv_u32 total)
{
    fv_grad_add_core(dst, src1, src2, total, fv_saturate_cast_8u);
}

static void
fv_grad_add_32s(fv_s32 *dst, double *src1, double *src2, fv_u32 total)
{
    fv_grad_add_core(dst, src1, src2, total, fv_saturate_cast_8u);
}

static void
fv_grad_add_32f(float *dst, double *src1, double *src2, fv_u32 total)
{
    fv_grad_add_core(dst, src1, src2, total, fv_saturate_cast_8u);
}

static void
fv_grad_add_64f(double *dst, double *src1, double *src2, fv_u32 total)
{
    fv_s32      i;

    for (i = 0; i < total; i++, dst++, src1++, src2++) {
        *dst = (*src1 + *src2);
    }
}

typedef void (*fv_grad_add_func)(void *, void *, void *, fv_u32);

static fv_grad_add_func fv_grad_add_tab[] = {
    (fv_grad_add_func)fv_grad_add_8u,
    (fv_grad_add_func)fv_grad_add_8s,
    (fv_grad_add_func)fv_grad_add_16u,
    (fv_grad_add_func)fv_grad_add_16s,
    (fv_grad_add_func)fv_grad_add_32s,
    (fv_grad_add_func)fv_grad_add_32f,
    (fv_grad_add_func)fv_grad_add_64f,
};

#define fv_grad_add_tab_size \
    (sizeof(fv_grad_add_tab)/sizeof(fv_grad_add_func))

static fv_grad_add_func 
fv_get_grad_add_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_grad_add_tab_size);

    return fv_grad_add_tab[depth];
}

void
fv_laplacian(fv_mat_t *dst, fv_mat_t *src, fv_u16 ddepth, fv_s32 ksize, 
            double scale, double delta, fv_s32 border_type)
{
    fv_grad_add_func    func;
    fv_mat_t            *dx;
    fv_mat_t            *dy;
    fv_mat_t            *kx;
    fv_mat_t            *ky;
    fv_mat_t            kernel = {};
    float               k[2][9] = {{0, 1, 0, 1, -4, 1, 0, 1, 0},
                {2, 0, 2, 0, -8, 0, 2, 0, 2}};
    fv_u16              cn;

    if (ksize == 1 || ksize == 3) {
        kernel = fv_mat(3, 3, FV_32FC1, k[ksize == 3]);
        if (scale != 1) {
            fv_mset_scale(kernel, scale, float);
        }

        fv_filter2D(dst, src, ddepth, &kernel, fv_point(-1, -1),
                delta, border_type);
        return;
    }

    cn = src->mt_nchannel;
    dx = fv_create_mat(src->mt_rows, src->mt_cols,
            FV_MAKETYPE(FV_DEPTH_64F, cn));
    FV_ASSERT(dx != NULL);
    dy = fv_create_mat(src->mt_rows, src->mt_cols,
            FV_MAKETYPE(FV_DEPTH_64F, cn));
    FV_ASSERT(dy != NULL);
    fv_get_sobel_kernels(&kx, &ky, 2, 0, ksize, 0, 0);

    fv_time_meter_set(1);
    fv_sep_filter2D(dx, src, kx, ky, fv_point(-1, -1), 
            delta, border_type);
    fv_sep_filter2D(dy, src, ky, kx, fv_point(-1, -1), 
            delta, border_type);
    fv_time_meter_get(1, 0);

    func = fv_get_grad_add_tab(dst->mt_depth);
    FV_ASSERT(func != NULL);
    fv_time_meter_set(1);
    func(dst->mt_data.dt_ptr, dx->mt_data.dt_db,
            dy->mt_data.dt_db, dst->mt_total*cn);
    fv_time_meter_get(1, 0);
    fv_release_mat(&kx);
    fv_release_mat(&ky);
    fv_release_mat(&dy);
    fv_release_mat(&dx);
}

void
fv_laplace(fv_image_t *dst, fv_image_t *src, fv_s32 aperture_size)
{
    fv_mat_t    _dst;
    fv_mat_t    _src;

    FV_ASSERT(src->ig_image_size == dst->ig_image_size && 
            src->ig_channels == dst->ig_channels);

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);

    fv_laplacian(&_dst, &_src, dst->ig_depth, aperture_size, 1, 0,
                FV_BORDER_REPLICATE);
}

