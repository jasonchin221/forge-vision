#ifndef __FV_LKPYRAMID_H__
#define __FV_LKPYRAMID_H__


extern fv_s32 fv_build_optical_flow_pyramid(fv_mat_t *mat, fv_mat_t *pyramid, 
            fv_size_t win_size, fv_s32 max_level, fv_bool with_derivatives,
            fv_s32 pyr_border, fv_s32 deriv_border, 
            fv_bool try_reuse_input_image);
extern void fv_lk_tracker_invoker(fv_mat_t *prev_pyr, fv_mat_t *next_pyr,
            fv_mat_t *guess, fv_mat_t *prev_pts, fv_mat_t *next_pts, 
            fv_mat_t status, fv_mat_t err, fv_s32 npoints, 
            fv_size_t win_size, fv_term_criteria_t criteria, fv_s32 level, 
            fv_s32 max_level, fv_s32 flags, double min_eig_threshold);

#endif
