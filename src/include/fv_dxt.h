#ifndef __FV_DXT_H__
#define __FV_DXT_H__

enum {
    FV_DXT_FORWARD,
    FV_DXT_INVERSE,
};

extern fv_s32 fv_get_optimal_dft_size(fv_s32 size);
extern void fv_dft(fv_mat_t *dst, fv_mat_t *src, 
        fv_s32 flags, fv_s32 nonzero_rows);
extern void fv_fft_real(float *dst, float *src, fv_s32 n, fv_s32 m, float pi2);
extern void fv_mul_spectrums(fv_mat_t *dst, fv_mat_t *dft_a, fv_mat_t *dft_b);

#endif
