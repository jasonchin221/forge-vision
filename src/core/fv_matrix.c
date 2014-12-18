
#include "fv_types.h"
#include "fv_core.h"
#include "fv_debug.h"
#include "fv_math.h"
#include "fv_imgproc.h"
#include "fv_matrix.h"
#include "fv_log.h"

static float fv_matrix0_determinant(float *matrix);
static float fv_matrix1_determinant(float *matrix);
static float fv_matrix2_determinant(float *matrix);
static void fv_matrix1_adjoint(float *dst, float *src);
static void fv_matrix2_adjoint(float *dst, float *src);

static fv_square_matrix_t
fv_square_matrix[FV_MATRIX_DIMENSION_MAX + 1] = {
    {
        .sm_dimension = 0,
        .sm_determinant = fv_matrix0_determinant,
    },
    {
        .sm_dimension = 1,
        .sm_determinant = fv_matrix1_determinant,
        .sm_adjoint = fv_matrix1_adjoint,
    },
    {
        .sm_dimension = 2,
        .sm_determinant = fv_matrix2_determinant,
        .sm_adjoint = fv_matrix2_adjoint,
    },
};

static float fv_matrix0_determinant(float *matrix)
{
    return 0;
}

static float fv_matrix1_determinant(float *matrix)
{
    return *matrix;
}

static float fv_matrix2_determinant(float *matrix)
{
    return (*matrix)*(*(matrix + 3)) - *(matrix + 1)*(*(matrix + 2));
}

static void fv_matrix1_adjoint(float *dst, float *src)
{
    *dst = *src;
}

static void fv_matrix2_adjoint(float *dst, float *src)
{
    *dst = *(src + 3);
    *(dst + 1) = -*(src + 1);
    *(dst + 2) = -*(src + 2);
    *(dst + 3) = *src;
}

float 
fv_matrix_determinant(float *matrix, fv_u32 dimension)
{
    if (dimension > FV_MATRIX_DIMENSION_MAX) {
        return 0;
    }

    return fv_square_matrix[dimension].sm_determinant(matrix);
}

fv_s32
fv_matrix_inverse(float *dst, float *src, fv_u32 dimension)
{
    fv_square_matrix_t      *matrix;
    float                   determinant;
    fv_s32                  i;

    if (dimension > FV_MATRIX_DIMENSION_MAX) {
        FV_LOG_ERR("Matrix dimension is too big!\n");
        return FV_ERROR;
    }

    matrix = &fv_square_matrix[dimension];
    if (matrix->sm_dimension != dimension) {
        FV_LOG_ERR("Matrix dimension not match!\n");
        return FV_ERROR;
    }

    determinant = matrix->sm_determinant(src);
    if (fv_float_is_zero(determinant)) {
        FV_LOG_PRINT("Matrix have no inverse matrix!\n");
        return FV_ERROR;
    }

    if (matrix->sm_adjoint == NULL) {
        FV_LOG_PRINT("Square matrix(%d) have no adjoint matrix!\n", 
                dimension);
        return FV_ERROR;
    }

    matrix->sm_adjoint(dst, src);

    for (i = 0; i < dimension*dimension; i++) {
        *(dst + i) /= determinant;
    }

    return FV_OK;
}

#define fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len)\
    do { \
        typeof(src)     src_data; \
        typeof(*src)    val; \
        double          min_val; \
        double          max_val; \
        fv_s32          i; \
                        \
        min_val = *minval; \
        max_val = *maxval; \
                            \
        src_data = src; \
        if (!mask) { \
            for (i = 0; i < len; i++) { \
                val = src_data[i]; \
                if (val < min_val) { \
                    min_val = val; \
                    *minidx = i; \
                } \
                if (val > max_val) { \
                    max_val = val; \
                    *maxidx = i; \
                } \
            } \
        } else{ \
            for (i = 0; i < len; i++) { \
                val = src_data[i]; \
                if (mask[i] && val < min_val) { \
                    min_val = val; \
                    *minidx = i; \
                } \
                if (mask[i] && val > max_val) { \
                    max_val = val; \
                    *maxidx = i; \
                } \
            } \
        } \
            \
        *minval = min_val; \
        *maxval = max_val; \
    } while(0)


static void 
fv_min_max_idx_8u(fv_u8 *src, fv_u8 *mask, double *minval, double *maxval, 
            size_t *minidx, size_t *maxidx, fv_s32 len)
{
    fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len);
}

static void 
fv_min_max_idx_8s(fv_s8 *src, fv_u8 *mask, double *minval, double *maxval,
            size_t *minidx, size_t *maxidx, fv_s32 len)
{
    fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len);
}

static void 
fv_min_max_idx_16u(fv_u16 *src, fv_u8 *mask, double *minval, double *maxval,
            size_t *minidx, size_t *maxidx, fv_s32 len)
{
    fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len);
}

static void 
fv_min_max_idx_16s(fv_s16 *src, fv_u8 *mask, double *minval, double *maxval,
            size_t *minidx, size_t *maxidx, fv_s32 len)
{
    fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len);
}

static void 
fv_min_max_idx_32s(fv_s32 *src, fv_u8 *mask, double *minval, double *maxval,
            size_t *minidx, size_t *maxidx, fv_s32 len)
{
    fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len);
}

static void 
fv_min_max_idx_32f(float *src, fv_u8 *mask, double *minval, double *maxval,
            size_t *minidx, size_t *maxidx, fv_s32 len)
{
    fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len);
}

static void 
fv_min_max_idx_64f(double *src, fv_u8 *mask, double *minval, double *maxval,
            size_t *minidx, size_t *maxidx, fv_s32 len)
{
    fv_min_max_idx_core(src, mask, minval, maxval, minidx, maxidx, len);
}

typedef void (*fv_min_max_idx_func)(void *, fv_u8 *, double *, double *, 
            size_t *, size_t *, fv_s32);

static fv_min_max_idx_func fv_min_max_tab[] = {
    (fv_min_max_idx_func)fv_min_max_idx_8u,
    (fv_min_max_idx_func)fv_min_max_idx_8s,
    (fv_min_max_idx_func)fv_min_max_idx_16u,
    (fv_min_max_idx_func)fv_min_max_idx_16s,
    (fv_min_max_idx_func)fv_min_max_idx_32s,
    (fv_min_max_idx_func)fv_min_max_idx_32f,
    (fv_min_max_idx_func)fv_min_max_idx_64f,
};

#define fv_min_max_tab_size (sizeof(fv_min_max_tab)/sizeof(fv_min_max_idx_func))

static fv_min_max_idx_func 
fv_get_min_max_tab(fv_u32 depth)
{
    FV_ASSERT(depth < fv_min_max_tab_size);

    return fv_min_max_tab[depth];
}

void
_fv_min_max_loc(fv_mat_t *mat, double *min_val, double *max_val, 
        fv_point_t *min_loc, fv_point_t *max_loc, fv_mat_t *mask)
{
    fv_min_max_idx_func     func;
    fv_u8                   *m;
    fv_s32                  width;
    size_t                  min_idx;
    size_t                  max_idx;
    double                  min_value = DBL_MAX;
    double                  max_value = -DBL_MAX;

    func = fv_get_min_max_tab(mat->mt_depth);
    FV_ASSERT(func != NULL);
    m = mask != NULL ? mask->mt_data.dt_ptr:NULL;
    func(mat->mt_data.dt_ptr, m, &min_value, &max_value, &min_idx, 
            &max_idx, mat->mt_total);
    width = mat->mt_width;
    if (min_val) {
        *min_val = min_value;
    }

    if (min_loc) {
        min_loc->pt_x = min_idx % width;
        min_loc->pt_y = min_idx/width;
    }

    if (max_val) {
        *max_val = max_value;
    }

    if (max_loc) {
        max_loc->pt_x = max_idx % width;
        max_loc->pt_y = max_idx/width;
    }
}

/*
 * fv_min_max_loc: 查找数组和子数组的全局最小值和最大值
 * @arr: 输入数组, 单通道或者设置了 COI 的多通道。
 * @min_val: 指向返回的最小值的指针。
 * @max_val: 向返回的最大值的指针。
 * @min_loc: 指向返回的最小值的位置指针。
 * @max_loc: 指向返回的最大值的位置指针。
 * @mask: 选择一个子数组的操作掩模。
 * 函数 MinMaxLoc 查找元素中的最小值和最大值以及他们的位置。函数在整个数组、或
 * 选定的 ROI 区域(对 IplImage)或当 MASK 不为 NULL 时指定的数组区域中,搜索极值 。
 * 如果数组不止一个通道,它就必须是设置了 COI 的 IplImage 类型。 如果是多维数组
 * min_loc->x 和 max_loc->x 将包含极值的原始位置信息 (线性的)。
*/
void 
fv_min_max_loc(fv_arr *arr, double *min_val, double *max_val,
        fv_point_t* min_loc, fv_point_t* max_loc, fv_arr *mask)
{    
    fv_image_t  *img = arr;
    fv_mat_t    mat;
    fv_mat_t    *_mat = NULL;
    fv_mat_t    _mask;

    mat = fv_image_to_mat(img);
    if (mask) {
        _mask = fv_image_to_mat(mask);
        FV_ASSERT(_mask.mt_atr == FV_8UC1);
        mask = &_mask;
    }

    if (img->ig_channels > 1) {
        FV_ASSERT(img->ig_roi != NULL && img->ig_roi->ri_coi >= 0);
        _mat = fv_mat_extract_image_coi(img);
    } else {
        _mat = &mat;
    }

    _fv_min_max_loc(_mat, min_val, max_val, min_loc, max_loc, mask);
    if (_mat != &mat) {
        fv_release_mat(&_mat);
    }
}
