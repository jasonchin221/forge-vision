#ifndef __FV_CONVERT_H__
#define __FV_CONVERT_H__

extern void _fv_convert_scale(fv_mat_t *dst, fv_mat_t *src, 
        double scale, double shift);
extern void fv_convert_scale(fv_image_t *dst, fv_image_t *src, 
        double scale, double shift);

#define fv_convert(dst, src)  fv_convert_scale(dst, src, 1, 0)
#define _fv_convert(dst, src)  _fv_convert_scale(dst, src, 1, 0)
#define _fv_zero(mat)  _fv_convert_scale(mat, NULL, 0, 0)

#endif
