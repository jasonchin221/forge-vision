#ifndef __FV_CORE_H__
#define __FV_CORE_H__

extern fv_image_t *fv_create_image(fv_size_t size, 
                fv_s32 depth, fv_s32 channels);
extern void fv_release_image(fv_image_t **image);
extern fv_image_t *fv_convert_image(IplImage *cv_img);

#endif
