
#include "fv_types.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_math.h"

#define fv_count_non_zero_core(src, total) \
    ({\
        fv_u32      i;\
        fv_s32      count = 0;\
        for (i = 0; i < total; i++) {\
            count += src[i] != 0; \
        }\
        count; \
     })

static fv_s32 
fv_count_non_zero_8u(fv_u8 *src, fv_s32 total)
{
    return fv_count_non_zero_core(src, total);
}

static fv_s32 
fv_count_non_zero_8s(fv_s8 *src, fv_s32 total)
{
    return fv_count_non_zero_core(src, total);
}

static fv_s32 
fv_count_non_zero_16u(fv_u16 *src, fv_s32 total)
{
    return fv_count_non_zero_core(src, total);
}

static fv_s32 
fv_count_non_zero_16s(fv_s16 *src, fv_s32 total)
{
    return fv_count_non_zero_core(src, total);
}

static fv_s32 
fv_count_non_zero_32s(fv_s32 *src, fv_s32 total)
{
    return fv_count_non_zero_core(src, total);
}

static fv_s32 
fv_count_non_zero_32f(float *src, fv_s32 total)
{
    return fv_count_non_zero_core(src, total);
}

static fv_s32 
fv_count_non_zero_64f(double *src, fv_s32 total)
{
    return fv_count_non_zero_core(src, total);
}

typedef fv_s32 (*fv_count_non_zero_func)(void *, fv_s32);

static fv_count_non_zero_func fv_count_non_zero_tab[] = {
    (fv_count_non_zero_func)fv_count_non_zero_8u,
    (fv_count_non_zero_func)fv_count_non_zero_8s,
    (fv_count_non_zero_func)fv_count_non_zero_16u,
    (fv_count_non_zero_func)fv_count_non_zero_16s,
    (fv_count_non_zero_func)fv_count_non_zero_32s,
    (fv_count_non_zero_func)fv_count_non_zero_32f,
    (fv_count_non_zero_func)fv_count_non_zero_64f,
};

#define fv_count_non_zero_tab_size \
    (sizeof(fv_count_non_zero_tab)/sizeof(fv_count_non_zero_func))

static fv_count_non_zero_func 
fv_get_count_non_zero_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_count_non_zero_tab_size);

    return fv_count_non_zero_tab[depth];
}

fv_s32 
_fv_count_non_zero(fv_mat_t *mat)
{
    fv_count_non_zero_func      func;

    FV_ASSERT(FV_MAT_NCHANNEL(mat) == 1);

    func = fv_get_count_non_zero_tab(mat->mt_depth);
    FV_ASSERT(func != NULL);
    return func(mat->mt_data.dt_ptr, mat->mt_total);
}

fv_s32 
fv_count_non_zero(fv_arr *arr)
{
    fv_image_t  *img = arr;
    fv_mat_t    mat;
    fv_mat_t    *_mat = NULL;
    fv_s32      num;

    mat = fv_image_to_mat(img);
    if (img->ig_channels > 1) {
        FV_ASSERT(img->ig_roi != NULL && img->ig_roi->ri_coi >= 0);
        _mat = fv_mat_extract_image_coi(img);
    } else {
        _mat = &mat;
    }

    num = _fv_count_non_zero(_mat);

    if (_mat != &mat) {
        fv_release_mat(&_mat);
    }

    return num;
}
