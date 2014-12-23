
#include "fv_types.h"
#include "fv_log.h"
#include "fv_debug.h"
#include "fv_core.h"
#include "fv_mem.h"
#include "fv_pyramid.h"
#include "fv_matrix.h"

fv_s32 
fv_build_optical_flow_pyramid(fv_mat_t *mat, fv_mat_t *pyramid, 
            fv_size_t win_size, fv_s32 max_level, fv_bool with_derivatives,
            fv_s32 pyr_border, fv_s32 deriv_border, 
            fv_bool try_reuse_input_image)
{
    fv_mat_t        *prev;
    fv_mat_t        *this;
    fv_s32          level;

#if 0
    FV_ASSERT(mat->mt_depth == FV_8U && win_size.sz_width > 2 && 
            win_size.sz_height > 2);
#endif

    *pyramid++ = *mat;
    fv_inc_ref_data(mat);

    prev = mat;
    for (level = 1; level <= max_level; ++level, pyramid++) {
        this = fv_create_mat(prev->mt_rows >> 1, 
                prev->mt_cols >> 1, prev->mt_atr);
        FV_ASSERT(this != NULL);
        *pyramid = *this;
        _fv_pyr_down(pyramid, prev, 0);
        fv_free(&this);
        prev = pyramid;
    }

    return max_level;
}

static void
fv_track_get_mismatch_vector(float *mismatch_vector, fv_mat_t *prev_pyr, 
            fv_mat_t *next_pyr, fv_point_2D32f_t track_point, float *guess, 
            float *vector, fv_mat_t *grad, fv_size_t win)
{
    float               *g;
    float               img_diff;
    fv_s32              x;
    fv_s32              y;
    fv_s32              offset_x;
    fv_s32              offset_y;
    fv_s32              ox1;
    fv_s32              oy1;
    fv_s32              ox2;
    fv_s32              oy2;
    fv_s32              vector_y;
    fv_s32              vector_x;
    fv_s32              width;
    fv_s32              height;

    vector_x = guess[0] + vector[0];
    vector_y = guess[1] + vector[1];
    mismatch_vector[0] = mismatch_vector[1] = 0;
    g = grad->mt_data.dt_fl;
    x = track_point.pf_x;
    y = track_point.pf_y;
    width = prev_pyr->mt_width;
    height = prev_pyr->mt_height;
    for (offset_y = -win.sz_height; 
            offset_y <= win.sz_height; offset_y++) {
        oy1 = y + offset_y;
        oy2 = oy1 + vector_y;
        if (oy1 < 0 || oy2 < 0 || oy1 >= height || oy2 >= height) {
            continue;
        }
        for (offset_x = -win.sz_width; 
                offset_x <= win.sz_width; offset_x++, g += 2) {
            ox1 = x + offset_x;
            ox2 = ox1 + vector_x;
            if (ox1 < 0 || ox2 < 0 || ox1 >= width || ox2 >= width) {
                continue;
            }
            img_diff = prev_pyr->mt_data.dt_ptr[oy1*width + ox1] -
                next_pyr->mt_data.dt_ptr[oy2*width + ox2];
            mismatch_vector[0] += g[0]*img_diff;
            mismatch_vector[1] += g[1]*img_diff;
        }
    }
}

static void
fv_lk_guess_thresh(fv_s32 *thresh, fv_u8 layer_num, fv_size_t win)
{
    fv_u8       i;

    thresh[0] = thresh[1] = 0;
    for (i = 0; i <= layer_num; i++) {
        thresh[0] *= 2;
        thresh[0] += win.sz_width;
        thresh[1] *= 2;
        thresh[1] += win.sz_height;
    }
}

void 
fv_lk_tracker_invoker(fv_mat_t *prev_pyr, fv_mat_t *next_pyr,
            fv_mat_t *guess, fv_mat_t *prev_pts, fv_mat_t *next_pts,
            fv_mat_t status, fv_mat_t err, fv_s32 npoints, 
            fv_size_t win_size, fv_term_criteria_t criteria, 
            fv_s32 level, fv_s32 max_level, fv_s32 flags, 
            double min_eig_threshold)
{
    fv_mat_t                *grad;
    fv_u8                   *s;
    float                   *p;
    float                   *n;
    float                   *g;
    float                   *d;
    fv_point_2D32f_t        track_point;
    float                   g_matrix[4] = {};
    float                   g_inverse[4] = {};
    float                   vector[2];
    float                   mismatch_vector[2] = {};
    float                   residual[2];
    double                  gx;
    double                  gy;
    fv_s32                  thresh[2];
    fv_s32                  i;
    fv_s32                  offset_x;
    fv_s32                  offset_y;
    fv_s32                  x;
    fv_s32                  y;
    fv_s32                  iter;
    fv_s32                  ret;
    fv_s32                  width;
    fv_s32                  height;

    width = prev_pyr->mt_width;
    height = prev_pyr->mt_height;
 
    grad = fv_create_mat(win_size.sz_height*2 + 1, 
            win_size.sz_width*2 + 1, FV_32FC2);
    FV_ASSERT(grad != NULL);
    p = prev_pts->mt_data.dt_fl;
    n = next_pts->mt_data.dt_fl;
    g = guess->mt_data.dt_fl;
    s = status.mt_data.dt_ptr;
    for (i = 0; i < npoints; i++, p += 2, g += 2, n += 2) {
        if (s != NULL && *s == 0) {
            s++;
            continue;
        }
        track_point.pf_x = p[0]/(1 << level);
        track_point.pf_y = p[1]/(1 << level);
        d = grad->mt_data.dt_fl;
        g_matrix[0] = g_matrix[1] = g_matrix[3] = 0;
        for (offset_y = -win_size.sz_height; 
                offset_y <= win_size.sz_height; offset_y++) {
            y = (fv_s32)track_point.pf_y + offset_y;
            if (y < 1 || y >= height - 1) {
                continue;
            }
            for (offset_x = -win_size.sz_width; 
                    offset_x <= win_size.sz_width; offset_x++) {
                x = (fv_s32)track_point.pf_x + offset_x;
                if (x < 1 || x >= width - 1) {
                    continue;
                }
                gx = (prev_pyr->mt_data.dt_ptr[y*width + x + 1] - 
                        prev_pyr->mt_data.dt_ptr[y*width + x - 1])/2.0;
                gy = (prev_pyr->mt_data.dt_ptr[(y + 1)*width + x] - 
                        prev_pyr->mt_data.dt_ptr[(y - 1)*width + x])/2.0;
                d[0] = gx;
                d[1] = gy;
                d += 2;
                g_matrix[0] += gx*gx;
                g_matrix[1] += gx*gy;
                g_matrix[3] += gy*gy;
            }
        }

        g_matrix[2] = g_matrix[1];
        ret = fv_matrix_inverse(&g_inverse[0], &g_matrix[0], 2);
        if (ret != FV_OK) {
            if (s != NULL) {
                *s++ = 0;
            }
            continue;
        }

        g[0] = 2*g[0];
        g[1] = 2*g[1];

        memset(vector, 0, sizeof(vector));

        for (iter = 0; iter < criteria.tc_max_iter; iter++) {
            fv_track_get_mismatch_vector(mismatch_vector, 
                    prev_pyr, next_pyr, track_point, 
                    g, vector, grad, win_size);

            residual[0] = mismatch_vector[0]*g_inverse[0] + 
                mismatch_vector[1]*g_inverse[1];
            residual[1] = mismatch_vector[0]*g_inverse[2] + 
                mismatch_vector[1]*g_inverse[3];

            vector[0] += residual[0];
            vector[1] += residual[1];
            if (fabs(residual[0]) < criteria.tc_epsilon && 
                    fabs(residual[1]) < criteria.tc_epsilon) {
                break;
            }
        }

        g[0] += vector[0];
        g[1] += vector[1];

        fv_lk_guess_thresh(thresh, max_level - level, win_size);
        if (fabs(g[0]) > thresh[0] || fabs(g[1]) > thresh[1]) {
            if (s != NULL) {
                *s++ = 0;
                continue;
            }
        }
        if (level == 0) {
            n[0] = p[0] + g[0];
            n[1] = p[1] + g[1];
        }
        if (s != NULL) {
            s++;
        }
    }

    fv_release_mat(&grad);
}

