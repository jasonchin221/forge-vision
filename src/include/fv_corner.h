#ifndef __FV_CORNER_H__
#define __FV_CORNER_H__

extern void fv_corner_min_eigen_val(fv_mat_t *dst, fv_mat_t *src, 
        fv_s32 block_size, fv_s32 ksize, fv_s32 border_type);
extern void fv_corner_harris(fv_mat_t *dst, fv_mat_t *src, 
        fv_s32 block_size, fv_s32 ksize, double k, fv_s32 border_type);

#endif
