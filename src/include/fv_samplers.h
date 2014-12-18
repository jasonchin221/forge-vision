#ifndef __FV_SAMPLERS_H__
#define __FV_SAMPLERS_H__

extern fv_s32 fv_get_rect_sub_pix_8u32f_C1R(float *dst, fv_s32 dst_step, 
        fv_size_t win_size, fv_u8 *src, fv_s32 src_step, fv_size_t src_size,
        fv_point_2D32f_t center, fv_s32 height);

#endif
