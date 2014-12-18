#ifndef __FV_TRACK_H__
#define __FV_TRACK_H__

#define FV_TRACK_WIN_SIZE           5
#define FV_TRACK_QUALITY_LEVEL      0.01
#define FV_TRACK_MIN_DISTANCE       5.0
#define FV_TRACK_BLOCK_SIZE         3
#define FV_TRACK_USE_HARRIS         0
#define FV_TRACK_HARRIS_K           0.04
#define FV_TRACK_MAX_ITER           20
#define FV_TRACK_EPSISON            0.03
#define FV_TRACK_PYR_LEVEL          5

typedef struct _fv_track_grid_t {
    fv_u32              tg_size;
    fv_point_2D32f_t    *tg_points;
} fv_track_grid_t;

extern fv_s32 _fv_cv_lk_optical_flow(IplImage *cv_img, fv_bool image);
extern fv_s32 fv_cv_lk_optical_flow(IplImage *cv_img, fv_bool image);
extern fv_s32 _fv_cv_track_points(IplImage *cv_img, fv_bool image);
extern fv_s32 fv_cv_track_points(IplImage *cv_img, fv_bool image);
extern fv_s32 fv_cv_sub_pix(IplImage *cv_img, fv_bool image);
extern void fv_good_features_to_track(fv_image_t *image, 
                        fv_point_2D32f_t *corners,
                        fv_s32 *corner_count, double quality_level, 
                        double min_distance, fv_s32 block_size, 
                        fv_image_t *mask, fv_s32 use_harris, 
                        double harris_k);
extern void fv_find_corner_sub_pix(fv_image_t *img, fv_point_2D32f_t *corners,
                    fv_s32 count, fv_size_t win, fv_size_t zero_zone,
                    fv_term_criteria_t criteria);
extern void fv_calc_optical_flow_pyr_lk(fv_image_t *prev, fv_image_t *curr,
        fv_point_2D32f_t *prev_features, fv_point_2D32f_t *curr_features,
        fv_s32 count, fv_size_t win_size, fv_s32 level, fv_s8 *status,
        float *track_error, fv_term_criteria_t criteria, fv_s32 flags);

#endif
