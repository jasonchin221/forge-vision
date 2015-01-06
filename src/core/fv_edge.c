
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

    printf("kx[%f %f %f], ky[%f %f %f]\n", 
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

void
_fv_canny(fv_mat_t *dst, fv_mat_t *src, double low_thresh, double high_thresh,
        fv_s32 aperture_size)
{
    fv_mat_t    *dx;
    fv_mat_t    *dy;
    fv_u8       *dst_data;
    fv_u8       *d;
    fv_u8       *map;
    fv_u8       *_map;
    fv_u8       *pmap;
    fv_u8       *buffer;
    fv_u8       **stack;
    fv_u8       **stack_top;
    fv_u8       **stack_bottom;
    fv_s16      *dx_data;
    fv_s16      *dy_data;
    fv_s32      *mag_buf[3];
    fv_s32      *mag;
    fv_s32      *norm;
    fv_u32      cn;
    fv_s32      x;
    fv_s32      y;
    fv_s32      i;
    fv_s32      j;
    fv_s32      width;
    fv_s32      height;
    fv_s32      mapstep;
    fv_s32      magstep1;
    fv_s32      magstep2;
    fv_s32      off_y;
    fv_s32      stack_size = 0;
    fv_s32      g;
    fv_s32      g1;
    fv_s32      g2;
    fv_s32      g3;
    fv_s32      g4;
    fv_s32      temp1 = 0;
    fv_s32      temp2 = 0;
    fv_s32      sz = 0;
    double      t;
    double      dweight;
    double      theta;
    fv_bool     prev_flag;

    if (low_thresh > high_thresh) {
        fv_swap(low_thresh, high_thresh);
    }
    cn = src->mt_nchannel;
    dx = fv_create_mat(src->mt_rows, src->mt_cols, FV_16SC(cn));
    FV_ASSERT(dx != NULL);
    dx->mt_depth = FV_16S;
    dy = fv_create_mat(src->mt_rows, src->mt_cols, FV_16SC(cn));
    FV_ASSERT(dy != NULL);
    dy->mt_depth = FV_16S;
    _fv_sobel(dx, src, FV_16S, 1, 0, aperture_size, 1, 0, 
                FV_BORDER_REPLICATE);
    _fv_sobel(dy, src, FV_16S, 0, 1, aperture_size, 1, 0, 
                FV_BORDER_REPLICATE);

    width = dst->mt_cols;
    height = dst->mt_rows;

    mapstep = width + 2;
    buffer = fv_alloc(mapstep*(height + 2) + mapstep * 3 * sizeof(fv_s32));
    FV_ASSERT(buffer != NULL);
    mag_buf[0] = (fv_s32 *)buffer;
    mag_buf[1] = mag_buf[0] + mapstep;
    mag_buf[2] = mag_buf[1] + mapstep;
    map = (fv_u8 *)(mag_buf[2] + mapstep);
    memset(map, 1, mapstep);
    memset(map + mapstep*(height + 1), 1, mapstep);

    stack_size = fv_max(1 << 10, src->mt_total/10);
    stack_bottom = fv_alloc(sizeof(*stack_bottom)*stack_size); 
    FV_ASSERT(stack_bottom != NULL);

    stack_top = stack_bottom;

    #define FV_CANNY_PUSH(d)        *(d) = 2, *stack_top++ = (d)
    #define FV_CANNY_POP(d)         (d) = *--stack_top
    
    // calculate magnitude and angle of gradient, perform non-maxima supression.
    // fill the map with one of the following values:
    //   0 - the pixel might belong to an edge
    //   1 - the pixel can not belong to an edge
    //   2 - the pixel does belong to an edge
       
    prev_flag = 0;
   
    for (i = 0, off_y = 0; i <= height; i++, off_y += width) {
        norm = mag_buf[(i > 0) + 1] + 1;
        dx_data = &dx->mt_data.dt_s[off_y];
        dy_data = &dy->mt_data.dt_s[off_y];
        if (i < height) {
            for (j = 0; j < width; j++) {
                norm[j] = fabs(dx_data[j]) + fabs(dy_data[j]);
            }
            norm[-1] = norm[width] = 0;
        } else {
            memset(norm - 1, 0, mapstep*sizeof(fv_s32));
        }

        if (i == 0) {
            continue;
        }

        if ((stack_top - stack_bottom) + width > stack_size) {
            sz = stack_top - stack_bottom;
            stack = stack_bottom;
            stack_size = stack_size*3/2;
            stack_bottom = fv_alloc(sizeof(*stack_bottom)*stack_size); 
            FV_ASSERT(stack_bottom != NULL);

            stack_top = stack_bottom + sz;
            memcpy(stack_bottom, stack, sz*sizeof(*stack_bottom));
            fv_free(&stack);
        }

        _map = map + mapstep*i + 1;
        _map[-1] = _map[width] = 1;
        mag = mag_buf[1] + 1; // take the central row 
        magstep1 = mag_buf[2] - mag_buf[1];
        magstep2 = mag_buf[0] - mag_buf[1];
        dx_data -= width;
        dy_data -= width;
        for (j = 0; j < width; j++, dx_data++, dy_data++) {
            g = mag[j];
            if (g <= low_thresh) {
                _map[j] = 1;
                prev_flag = 0;
                continue;
            }
            if (*dx_data == 0) {
                temp1 = mag[j + magstep2];
                temp2 = mag[j + magstep1];
            } else if (*dy_data == 0) {
                temp1 = mag[j - 1];
                temp2 = mag[j + 1];
            } else {
                t = *(dy_data)/(double)(*(dx_data));
                theta = atan(t)*180/fv_pi;
                /*
                 * 首先判断属于那种情况，然后根据情况插值
                 * First:
                 *      g1 g2
                 *         C
                 *         g3 g4
                 */

                if (theta <= -45 && theta > -90) {
                    g1 = mag[j + magstep1 - 1];
                    g2 = mag[j + magstep1];
                    g3 = mag[j + magstep2];
                    g4 = mag[j + magstep2 + 1];
                    dweight = fabs(*(dx_data)/(double)(*(dy_data)));
                    temp1 = g1*dweight + g2*(1 - dweight);  
                    temp2 = g4*dweight + g3*(1 - dweight); 
                }
                /*
                 * Second:
                 *      g1
                 *      g2 C g3
                 *           g4
                 */
                else if (theta <= 0 && theta > -45) {
                    g1 = mag[j + magstep1 - 1];
                    g2 = mag[j - 1];
                    g3 = mag[j + 1];
                    g4 = mag[j + magstep2 + 1];
                    dweight = fabs(t);
                    temp1 = g1*dweight + g2*(1 - dweight);  
                    temp2 = g4*dweight + g3*(1 - dweight); 
                }
                /*
                 * Third:
                 *         g1 g2
                 *         C
                 *      g4 g3
                 */
                else if (theta >= 45 && theta < 90) {
                    g1 = mag[j + magstep1];
                    g2 = mag[j + magstep1 + 1];
                    g3 = mag[j + magstep2];
                    g4 = mag[j + magstep2 - 1];
                    dweight = fabs(*(dx_data)/(double)(*(dy_data)));
                    temp1 = g2*dweight + g1*(1 - dweight);  
                    temp2 = g4*dweight + g3*(1 - dweight); 
                }
                /*
                 * Fourth:
                 *            g1
                 *      g4 C  g2
                 *      g3
                 */
                else if (theta > 0 && theta < 45) {
                    g1 = mag[j + magstep1 + 1];
                    g2 = mag[j + 1];
                    g3 = mag[j + magstep2 - 1];
                    g4 = mag[j - 1];
                    dweight = fabs(t);
                    temp1 = g1*dweight + g2*(1 - dweight);  
                    temp2 = g3*dweight + g4*(1 - dweight); 
                } else {
                    temp1 = temp2 = 0;
                }
            }
            if (g < temp1 || g < temp2) {
                _map[j] = 1;
                prev_flag = 0;
                continue;
            }
            if (!prev_flag && g > high_thresh && _map[j - mapstep] != 2) {
                FV_CANNY_PUSH(_map + j);
                prev_flag = 1;
            } else {
                _map[j] = 0;
            }
        }
        mag = mag_buf[0];
        mag_buf[0] = mag_buf[1];
        mag_buf[1] = mag_buf[2];
        mag_buf[2] = mag;
    }

    while (stack_top > stack_bottom) {
        FV_CANNY_POP(d);

        if (!d[1]) {
            FV_CANNY_PUSH(d + 1);
        }

        if (!d[-1]) {
            FV_CANNY_PUSH(d - 1);
        }

        if (!d[mapstep]) {
            FV_CANNY_PUSH(d + mapstep);
        }

        if (!d[-mapstep]) {
            FV_CANNY_PUSH(d - mapstep);
        }

        if (!d[mapstep + 1]) {
            FV_CANNY_PUSH(d + mapstep + 1);
        }

        if (!d[mapstep - 1]) {
            FV_CANNY_PUSH(d + mapstep - 1);
        }
        
        if (!d[-mapstep + 1]) {
            FV_CANNY_PUSH(d - mapstep + 1);
        }

        if (!d[-mapstep - 1]) {
            FV_CANNY_PUSH(d - mapstep - 1);
        }
    }

    dst_data = &dst->mt_data.dt_ptr[0];
    pmap = map + mapstep + 1;
    for (y = 0; y < dst->mt_rows; y++, pmap += mapstep) {
        for (x = 0; x < width; x++, dst_data++) {
            *dst_data = (fv_u8)-(pmap[x] >> 1);
        }
    }
 
    fv_free(&buffer);
    fv_free(&stack_bottom);
    fv_release_mat(&dy);
    fv_release_mat(&dx);
}

void
fv_canny(fv_image_t *dst, fv_image_t *src, double thresh1, double thresh2, 
        fv_s32 aperture_size)
{
    fv_mat_t    _dst;
    fv_mat_t    _src;

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);

    FV_ASSERT(_src.mt_total == _dst.mt_total && _src.mt_atr == FV_8UC1 && 
            _dst.mt_atr == FV_8UC1);

    _fv_canny(&_dst, &_src, thresh1, thresh2, aperture_size);
}

