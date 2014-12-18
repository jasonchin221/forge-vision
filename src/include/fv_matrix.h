#ifndef __FV_MATRIX_H__
#define __FV_MATRIX_H__

#define FV_MATRIX_DIMENSION_MAX     2

typedef struct _fv_square_matrix_t {
    fv_u32      sm_dimension;
    float       (*sm_determinant)(float *);     //行列式
    void        (*sm_adjoint)(float *, float *); //伴随阵
} fv_square_matrix_t;

extern float fv_matrix_determinant(float *matrix, fv_u32 dimension);
extern fv_s32 fv_matrix_inverse(float *dst, float *src, fv_u32 dimension);

extern void _fv_min_max_loc(fv_mat_t *img, double *min_val, double *max_val,
        fv_point_t *min_loc, fv_point_t *max_loc, fv_mat_t *mask);
extern void fv_min_max_loc(fv_arr *arr, double *min_val, double *max_val,
        fv_point_t* min_loc, fv_point_t* max_loc, fv_arr *mask);
extern fv_s32 fv_cv_min_max_loc(IplImage *cv_img, fv_bool image);

#endif
