#include "fv_types.h"
#include "fv_log.h"
#include "fv_debug.h"
#include "fv_core.h"
#include "fv_math.h"

static void *
fv_adjust_rect(void *srcptr, fv_s32 src_step, fv_s32 pix_size,
               fv_size_t src_size, fv_size_t win_size,
               fv_point_t ip, fv_rect_t *p_rect)
{
    fv_rect_t   rect;
    fv_u8       *src = srcptr;

    if (ip.pt_x >= 0) {
        src += ip.pt_x*pix_size;
        rect.rt_x = 0;
    } else {
        rect.rt_x = -ip.pt_x;
        if (rect.rt_x > win_size.sz_width) {
            rect.rt_x = win_size.sz_width;
        }
    }

    if (ip.pt_x < src_size.sz_width - win_size.sz_width) {
        rect.rt_width = win_size.sz_width;
    } else {
        rect.rt_width = src_size.sz_width - ip.pt_x - 1;
        if (rect.rt_width < 0) {
            src += rect.rt_width*pix_size;
            rect.rt_width = 0;
        }
        FV_ASSERT(rect.rt_width <= win_size.sz_width);
    }

    if (ip.pt_y >= 0) {
        src += ip.pt_y * src_step;
        rect.rt_y = 0;
    } else {
        rect.rt_y = -ip.pt_y;
    }

    if (ip.pt_y < src_size.sz_height - win_size.sz_height) {
        rect.rt_height = win_size.sz_height;
    } else {
        rect.rt_height = src_size.sz_height - ip.pt_y - 1;
        if (rect.rt_height < 0) {
            src += rect.rt_height*src_step;
            rect.rt_height = 0;
        }
    }

    *p_rect = rect;

    return src - rect.rt_x*pix_size;
}

fv_s32 
fv_get_rect_sub_pix_8u32f_C1R(float *dst, fv_s32 dst_step, fv_size_t win_size,
        fv_u8 *src, fv_s32 src_step, fv_size_t src_size,
        fv_point_2D32f_t center, fv_s32 height)
{
    fv_u8           *src2;
    fv_u8           *src_data;
    fv_point_t      ip;
    fv_rect_t       r;
    float           a12;
    float           a22;
    float           b1;
    float           b2;
    float           a;
    float           b;
    float           prev;
    float           t;
    double          s = 0;
    fv_s32          i;
    fv_s32          j;

    center.pf_x -= (win_size.sz_width - 1)*0.5;
    center.pf_y += (win_size.sz_height - 1)*0.5;

    ip.pt_x = center.pf_x;
    ip.pt_y = center.pf_y;

    if (win_size.sz_width <= 0 || win_size.sz_height <= 0) {
        return FV_ERROR;
    }

    a = center.pf_x - ip.pt_x;
    b = center.pf_y - ip.pt_y;
    a = fv_max(a, 0.0001);
    a12 = a*(1.0 - b);
    a22 = a*b;
    b1 = 1.0 - b;
    b2 = b;
    s = (1.0 - a)/a;

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    ip.pt_y = height - 1 - ip.pt_y;
    if (0 <= ip.pt_x && ip.pt_x < src_size.sz_width - win_size.sz_width &&
        win_size.sz_height <= ip.pt_y && ip.pt_y < src_size.sz_height) {
        // extracted rectangle is totally inside the image
        src_data = src + ip.pt_y * src_step + ip.pt_x;
        for (; win_size.sz_height--; src_data += src_step, dst += dst_step) {
            prev = (1 - a)*(b1*src_data[0] + b2*src_data[-src_step]);
            for (j = 0; j < win_size.sz_width; j++) {
                t = a12*src_data[j + 1] + a22*src_data[j + 1 - src_step];
                dst[j] = prev + t;
                prev = t*s;
            }
        }
    } else {
        src = (fv_u8 *)fv_adjust_rect(src, src_step*sizeof(*src),
                sizeof(*src), src_size, win_size, ip, &r);
        for (i = 0; i < win_size.sz_height; i++, dst += dst_step) {
            src2 = src + src_step;
            if (i < r.rt_y || i >= r.rt_height) {
                src2 -= src_step;
            }

            for (j = 0; j < r.rt_x; j++) {
                dst[j] = src[r.rt_x]*b1 + src2[r.rt_x]*b2;
            }

            if (j < r.rt_width) {
                prev = (1 - a)*(b1*src[j] + b2*src2[j]);
                for (; j < r.rt_width; j++) {
                    t = a12*src[j + 1] + a22*src2[j + 1];
                    dst[j] = prev + t;
                    prev = (t*s);
                }
            }

            for (; j < win_size.sz_width; j++) {
                 dst[j]= src[r.rt_width]*b1 + src2[r.rt_width]*b2;
            }

            if (i < r.rt_height){
                src = src2;
            }
        }
    }

    return FV_OK;
}


