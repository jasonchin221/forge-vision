
#include "fv_types.h"
#include "fv_log.h"
#include "fv_debug.h"
#include "fv_core.h"
#include "fv_filter.h"
#include "fv_imgproc.h"
#include "fv_math.h"
#include "fv_mem.h"
#include "fv_border.h"

#define FV_PD_SIZE      5

#define fv_pyr_abstract_row(dst, src, step, cn, tab_m) \
    do { \
        fv_u32          k; \
        fv_u32          x; \
        fv_s32          i; \
        fv_s32          sx; \
        for (k = 0; k < cn; k++) { \
            for (x = cn, i = 1; x < step - cn; x += cn, i++) { \
                sx = tab_m[i]*cn + k; \
                dst[x + k] = src[sx]*6 + \
                (src[sx - cn] + src[sx + cn])*4 + \
                src[sx - (cn << 1)] + src[sx + (cn << 1)]; \
            } \
        } \
    } while(0)
 
#define fv_pyr_abstract_core(dst, src, dsize, ssize, cn, cast_op) \
    do { \
        typeof(src)     src_data; \
        typeof(dst)     dst_data; \
        double          *buf; \
        double          *buf_data; \
        double          *row[FV_PD_SIZE]; \
        double          *row0; \
        double          *row1; \
        double          *row2; \
        double          *row3; \
        double          *row4; \
        fv_u32          i; \
        fv_u32          k; \
        fv_u32          x; \
        fv_u32          y; \
        fv_u32          srow_num; \
        fv_u32          drow_num; \
        fv_s32          *tab_m; \
        fv_s32          sy; \
        \
        drow_num = dsize.sz_width*cn; \
        srow_num = ssize.sz_width*cn; \
        buf = fv_alloc(sizeof(*buf)*drow_num*FV_PD_SIZE); \
        FV_ASSERT(buf != NULL); \
        tab_m = fv_alloc(sizeof(*tab_m)*dsize.sz_width); \
        FV_ASSERT(tab_m != NULL); \
        for (x = 0; x < dsize.sz_width; x++) { \
           tab_m[x] = (x << 1); \
        } \
        for (i = 0; i < 4; i++) { \
            sy = fv_border_get_value(FV_BORDER_REFLECT_101, \
                    ((i - 2) << 1), ssize.sz_width); \
            buf_data = buf + i*drow_num; \
            src_data = src + sy*srow_num; \
            fv_pyr_abstract_row(buf_data, src_data, \
                    drow_num, cn, tab_m); \
        } \
        for (y = 0; y < dsize.sz_height; y++) { \
            for (i = 0; i < FV_PD_SIZE; i++) { \
                row[i] = buf + ((i + y) % FV_PD_SIZE)*drow_num; \
            } \
            row0 = row[0]; \
            row1 = row[1]; \
            row2 = row[2]; \
            row3 = row[3]; \
            row4 = row[4]; \
            dst_data = dst + y*drow_num; \
            sy = fv_border_get_value(FV_BORDER_REFLECT_101, \
                    ((y + 2) << 1), ssize.sz_height); \
            src_data = src + sy*srow_num; \
            fv_pyr_abstract_row(row4, src_data, drow_num, cn, tab_m); \
            for (k = 0; k < cn; k++) { \
                for (x = 0; x < drow_num; x += cn) { \
                    dst_data[x + k] = cast_op((row2[x + k]*6 + \
                            (row1[x + k] + row3[x + k])*4 + \
                            row0[x + k] + row4[x + k])/256); \
                } \
            } \
        } \
        fv_free(&tab_m); \
        fv_free(&buf); \
    } while(0)

static void
fv_pyr_abstract_8u(fv_u8 *dst, fv_u8 *src, fv_size_t dsize,
            fv_size_t ssize, fv_u32 cn)
{
    fv_pyr_abstract_core(dst, src, dsize, ssize, cn, fv_saturate_cast_8u);
}

void 
_fv_pyr_down(fv_mat_t *dst, fv_mat_t *src, fv_u32 filter)
{
    fv_size_t       dsize;
    fv_size_t       ssize;

    FV_ASSERT(abs(dst->mt_rows*2 - src->mt_rows) <= 2 && 
            abs(dst->mt_cols*2 - src->mt_cols) <= 2);

    dsize = fv_get_size(dst);
    ssize = fv_get_size(src);
    fv_pyr_abstract_8u(dst->mt_data.dt_ptr, src->mt_data.dt_ptr, 
            dsize, ssize, dst->mt_nchannel);
}

void 
fv_pyr_down(fv_image_t *dst, fv_image_t *src, fv_u32 filter)
{
    fv_mat_t    _dst;
    fv_mat_t    _src;

    FV_ASSERT(src->ig_depth == dst->ig_depth && 
            src->ig_channels == dst->ig_channels);

    _dst = fv_image_to_mat(dst);
    _src = fv_image_to_mat(src);
 
    _fv_pyr_down(&_dst, &_src, filter);
}
