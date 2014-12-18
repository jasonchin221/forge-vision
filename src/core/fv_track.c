#include <opencv/cv.h>  
#include <opencv/highgui.h>

#include "fv_types.h"
#include "fv_log.h"
#include "fv_debug.h"
#include "fv_core.h"
#include "fv_corner.h"
#include "fv_imgproc.h"
#include "fv_matrix.h"
#include "fv_thresh.h"
#include "fv_morph.h"
#include "fv_mem.h"
#include "fv_math.h"
#include "fv_track.h"
#include "fv_samplers.h"
#include "fv_filter.h"
#include "fv_lkpyramid.h"
#include "fv_time.h"

#define FV_CORNER_SUB_PIX_MAX_ITERS     100

typedef struct _fv_sort_point_t {
    fv_point_t      sp_point;
    double          sp_value;
} fv_sort_point_t;

static void
fv_track_points_insert_sort(fv_sort_point_t *corners, fv_mat_t *mat,
                fv_s32 x, fv_s32 y, double value, fv_u32 total)
{
    fv_u32  i;
    fv_u32  j;

    for (i = 0; i < total; i++) {
        if (value > corners[i].sp_value) {
            for (j = total; j > i; j--) {
                corners[j] = corners[j - 1];
            }
            break;
        }
    }

    corners[i].sp_point.pt_x = x;
    corners[i].sp_point.pt_y = y;
    corners[i].sp_value = value;
}

/*
 * fv_good_features_to_track: 确定图像的强角点
 * @image: 输入图像，8-位或浮点32-比特，单通道
 * @corners: 输出参数，检测到的角点
 * @corner_count: 输出参数，检测到的角点数目
 * @quality_level: 认为一点是角点的可接受的最小特征值
 * @min_distance: 角点的最小距离
 * @mask: 是一个像素值为bool类型的image，用于指定参与角点计算的点
 * @block_size: 是计算导数的自相关矩阵时指定点的领域，
 *              采用小窗口计算的结果比单点
 *              （也就是block_size为1）计算的结果要好
 * @use_harris: 标志位。当use_harris的值为非0，则函数使用Harris
 *              的角点定义；若为0，则使用Shi-Tomasi的定义
 * @harris_k: 当use_harris为k且非0，则k为用于设置Hessian
 *              自相关矩阵即对Hessian行列式的相对权重的权重系数
 */
void
fv_good_features_to_track(fv_image_t *image, fv_point_2D32f_t *corners,
                        fv_s32 *corner_count, double quality_level, 
                        double min_distance, fv_s32 block_size, 
                        fv_image_t *mask, fv_s32 use_harris, 
                        double harris_k)
{
    fv_mat_t            *eig;
    fv_mat_t            *tmp;
    fv_mat_t            *__mask = NULL;
    fv_track_grid_t     *grid;
    fv_track_grid_t     *g;
    fv_sort_point_t     *points;
    float               *eig_data;
    float               *tmp_data;
    fv_u8               *mask_data;
    fv_mat_t            _mask;
    fv_mat_t            img;
    fv_size_t           size;
    double              max_val = 0;
    float               val;
    float               dx;
    float               dy;
    fv_s32              total = 0;
    fv_s32              count = 0;
    fv_s32              x;
    fv_s32              y;
    fv_s32              x1;
    fv_s32              y1;
    fv_s32              x2;
    fv_s32              y2;
    fv_s32              xx;
    fv_s32              yy;
    fv_s32              x_cell;
    fv_s32              y_cell;
    fv_s32              i;
    fv_s32              j;
    fv_s32              cell_size;
    fv_s32              grid_width;
    fv_s32              grid_height;
    fv_bool             good;

    FV_ASSERT(quality_level > 0 && min_distance >= 0 && *corner_count >= 0);
    FV_ASSERT(mask == NULL || (mask->ig_depth == FV_DEPTH_8U &&
                mask->ig_channels == 1 && 
                mask->ig_image_size == image->ig_image_size));

    img = fv_image_to_mat(image);
    FV_ASSERT(img.mt_atr == FV_8UC1 || img.mt_atr == FV_32FC1);
    size = fv_get_size(image);
    eig = fv_create_mat(size.sz_height, size.sz_width, FV_32FC1);
    FV_ASSERT(eig != NULL);
    tmp = fv_create_mat(size.sz_height, size.sz_width, FV_32FC1);
    FV_ASSERT(tmp != NULL);

    if (mask != NULL) {
        _mask = fv_image_to_mat(mask);
        __mask = &_mask;
    }

    /*
     * 计算输入图像的每一个像素点的最小特征值,并将结果存储到变量eig中
     */
    fv_time_meter_set(FV_TIME_METER1);
    if (use_harris) {
        fv_corner_harris(eig, &img, block_size, 3, harris_k, FV_BORDER_DEFAULT);
    } else {
        fv_corner_min_eigen_val(eig, &img, block_size, 3, FV_BORDER_DEFAULT);
    }
    fv_time_meter_get(FV_TIME_METER1, 0);

    _fv_min_max_loc(eig, NULL, &max_val, NULL, NULL, __mask);
    printf("max = %f\n", max_val);
    fv_time_meter_set(FV_TIME_METER1);
    _fv_threshold(eig, eig, max_val*quality_level, 0, FV_THRESH_TOZERO);
    fv_time_meter_get(FV_TIME_METER1, 0);
    fv_time_meter_set(FV_TIME_METER1);
    fv_dilate_default(tmp, eig, NULL);
    fv_time_meter_get(FV_TIME_METER1, 0);

    points = fv_alloc(sizeof(*points)*size.sz_height*size.sz_width);
    FV_ASSERT(points != NULL);
    // collect list of pointers to features - put them into temporary image
    fv_time_meter_set(FV_TIME_METER1);
    for (y = 1; y < size.sz_height - 1; y++) {
        eig_data = eig->mt_data.dt_fl + y*size.sz_width;
        tmp_data = tmp->mt_data.dt_fl + y*size.sz_width;
        mask_data = __mask ? __mask->mt_data.dt_ptr : NULL;
        for (x = 1; x < size.sz_width - 1; x++) {
            val = eig_data[x];
            if (val != 0 && val == tmp_data[x] && 
                    (!mask_data || mask_data[x])) {
                fv_track_points_insert_sort(points, eig, x, y, val, total);
                total++;
            }
        }
    }
    fv_time_meter_get(FV_TIME_METER1, 0);

    fv_release_mat(&tmp);
    fv_release_mat(&eig);

    if (min_distance >= 1) {
         // Partition the image into larger grids
        cell_size = min_distance;
        grid_width = (image->ig_width + cell_size - 1)/cell_size;
        grid_height = (image->ig_height + cell_size - 1)/cell_size;

        grid = fv_calloc(sizeof(*grid)*grid_height*grid_width);
        FV_ASSERT(grid != NULL);
        min_distance *= min_distance;
        for (i = 0; i < total; i++) {
            y = points[i].sp_point.pt_y;
            x = points[i].sp_point.pt_x;
            x_cell = x/cell_size;
            y_cell = y/cell_size;
            good = 1;

            x1 = x_cell - 1;
            y1 = y_cell - 1;
            x2 = x_cell + 1;
            y2 = y_cell + 1;

            // boundary check
            x1 = fv_max(0, x1);
            y1 = fv_max(0, y1);
            x2 = fv_min(grid_width - 1, x2);
            y2 = fv_min(grid_height - 1, y2);

            for (yy = y1; yy <= y2; yy++) {
                for (xx = x1; xx <= x2; xx++) {
                    g = &grid[yy*grid_width + xx];
                    if (g->tg_points == NULL) {
                        continue;
                    }

                    for(j = 0; j < g->tg_size; j++) {
                        dx = x - g->tg_points[j].pf_x;
                        dy = y - g->tg_points[j].pf_y;
                        if (dx*dx + dy*dy < min_distance) {
                            good = 0;
                            goto break_out;
                        }
                    }
                }
            }

break_out:
            if (!good) {
                continue;
            }
            g = &grid[y_cell*grid_width + x_cell];
            if (g->tg_points == NULL) {
                g->tg_points = 
                    fv_alloc(sizeof(*(g->tg_points))*cell_size*cell_size);
                FV_ASSERT(g->tg_points != NULL);
            }
            g->tg_points[g->tg_size].pf_x = points[i].sp_point.pt_x;
            g->tg_points[g->tg_size].pf_y = points[i].sp_point.pt_y;
            corners[count] = g->tg_points[g->tg_size];
            g->tg_size++;
            count++;
            FV_ASSERT(g->tg_size <= cell_size*cell_size);
            if (count == *corner_count) {
                break;
            }
        }
        for (j = 0; j < grid_width*grid_height; j++) {
            fv_free(&grid[j].tg_points);
        }
        fv_free(&grid);
    } else {
        count = fv_min(total, *corner_count);
        for (i = 0; i < count; i++) {
            corners[i].pf_x = points[i].sp_point.pt_x;
            corners[i].pf_y = points[i].sp_point.pt_y;
        }
        memcpy(corners, points, sizeof(*corners)*count);
    }

    *corner_count = count;
    fv_free(&points);
}

void
_fv_find_corner_sub_pix(fv_mat_t *mat, fv_point_2D32f_t *corners,
                    fv_s32 count, fv_size_t win, fv_size_t zero_zone,
                    fv_term_criteria_t criteria)
{
    float               *mask_x;
    float               *mask_y;
    float               *mask;
    float               *src_buffer;
    float               *gx_buffer;
    float               *gy_buffer;
    float               *buffer;
    fv_point_2D32f_t    ci;
    fv_point_2D32f_t    ci2;
    fv_point_2D32f_t    ci3;
    fv_point_2D32f_t    ct;
    fv_size_t           size;
    fv_size_t           src_buf_size;
    double              eps = 0;
    double              coeff;
    double              err;
    double              a;
    double              b;
    double              c;
    double              bb1;
    double              bb2;
    double              scale;
    double              det;
    double              m;
    double              tgx;
    double              tgy;
    double              gxx;
    double              gxy;
    double              gyy;
    double              px;
    double              py;
    float               drv[] = {-1.0, 0.0, 1.0};
    fv_s32              iter = 0;
    fv_s32              i;
    fv_s32              j;
    fv_s32              k;
    fv_s32              pt_i;
    fv_s32              max_iters = 0;
    fv_s32              win_w = win.sz_width * 2 + 1;
    fv_s32              win_h = win.sz_height * 2 + 1;
    fv_s32              win_rect_size = (win_w + 4) * (win_h + 4);
    fv_s32              height;
    fv_s32              ret;

    if (mat->mt_atr != FV_8UC1) {
        FV_LOG_ERR("The source image must be 8-bit single-channel (CV_8UC1)\n");
    }

    FV_ASSERT(corners != NULL);
    FV_ASSERT(count >= 0);

    if (count == 0) {
        return;
    }

    if (win.sz_width <= 0 || win.sz_height <= 0) {
        FV_LOG_ERR("Size invalid!\n");
    }

    size = fv_get_size(mat);
    if (size.sz_width < win_w + 4 || size.sz_height < win_h + 4) {
        FV_LOG_ERR("Image size is too small!\n");
    }

    /* initialize variables, controlling loop termination */
    switch (criteria.tc_type) {
        case FV_TERMCRIT_ITER:
            eps = 0;
            max_iters = criteria.tc_max_iter;
            break;
        case FV_TERMCRIT_EPS:
            eps = criteria.tc_epsilon;
            max_iters = FV_CORNER_SUB_PIX_MAX_ITERS;
            break;
        case FV_TERMCRIT_ITER | FV_TERMCRIT_EPS:
            eps = criteria.tc_epsilon;
            max_iters = criteria.tc_max_iter;
            break;
        default:
            FV_LOG_ERR("Unknow criteria type(%d)!\n", criteria.tc_type);
    }

    eps = fv_max(eps, 0);
    eps *= eps;                 /* use square of error in comparsion operations. */

    max_iters = fv_max(max_iters, 1);
    max_iters = fv_min(max_iters, FV_CORNER_SUB_PIX_MAX_ITERS);

    height = mat->mt_rows;

    buffer = fv_alloc(sizeof(*buffer)*(win_rect_size * 5 + 
                win_w + win_h + 32));
    FV_ASSERT(buffer != NULL);
    /* assign pointers */
    mask_x = buffer;
    mask_y = mask_x + win_w + 4;
    mask = mask_y + win_h + 4;
    src_buffer = mask + win_w * win_h;
    gx_buffer = src_buffer + win_rect_size;
    gy_buffer = gx_buffer + win_rect_size;

    coeff = 1.0/(win.sz_width * win.sz_width);

    /* calculate mask */
    for (i = -win.sz_width, k = 0; i <= win.sz_width; i++, k++) {
        mask_x[k] = (float)exp(-i * i * coeff);
    }

    if (win.sz_width == win.sz_height) {
        mask_y = mask_x;
    } else {
        coeff = 1.0/(win.sz_height * win.sz_height);
        for (i = -win.sz_height, k = 0; i <= win.sz_height; i++, k++) {
            mask_y[k] = (float)exp(-i * i * coeff);
        }
    }

    for (i = 0; i < win_h; i++) {
        for (j = 0; j < win_w; j++) {
            mask[i * win_w + j] = mask_x[j] * mask_y[i];
        }
    }

    /* make zero_zone */
    if(zero_zone.sz_width >= 0 && zero_zone.sz_height >= 0 &&
        zero_zone.sz_width * 2 + 1 < win_w && 
        zero_zone.sz_height * 2 + 1 < win_h) {
        for (i = win.sz_height - zero_zone.sz_height; 
                i <= win.sz_height + zero_zone.sz_height; i++) {
            for (j = win.sz_width - zero_zone.sz_width; 
                    j <= win.sz_width + zero_zone.sz_width; j++) {
                mask[i * win_w + j] = 0;
            }
        }
    }

    /* set sizes of image rectangles, used in convolutions */
    src_buf_size.sz_width = win_w + 2;
    src_buf_size.sz_height = win_h + 2;

    /* do optimization loop for all the points */
    for (pt_i = 0; pt_i < count; pt_i++) {
        ct = corners[pt_i];
        ci = ci3 = ct;
        ci3.pf_y = height - 1 - ci3.pf_y;
        iter = 0;
        do {
            ret = fv_get_rect_sub_pix_8u32f_C1R(src_buffer, 
                    src_buf_size.sz_width * sizeof(src_buffer[0]),
                    src_buf_size, mat->mt_data.dt_ptr, 
                    mat->mt_step, size, ci3, height);
#if 0
            int k;
            for (k = 0, i = 0; i < (win_h + 2); i++) {
                for (j = 0; j < (win_w + 2); j++, k++) {
                    printf("%f ", src_buffer[k]);
                }
                printf("\n");
            }
            printf("====ci[%f %f]\n", ci.pf_x, 2047 - ci.pf_y);
#endif
            FV_ASSERT(ret == FV_OK);

            /* calc derivatives */
            fv_sep_conv_small3_32f(gx_buffer, win_w * sizeof(gx_buffer[0]), 
                    src_buffer + src_buf_size.sz_width, 
                    src_buf_size.sz_width * sizeof(src_buffer[0]),
                    src_buf_size, drv, NULL, NULL);
#if 0
            for (k = 0, i = 0; i < (win_h); i++) {
                for (j = 0; j < (win_w); j++, k++) {
                    printf("%f ", gx_buffer[k]);
                }
                printf("\n");
            }
            printf("gx\n");
#endif
            fv_sep_conv_small3_32f(gy_buffer, win_w * sizeof(gy_buffer[0]),
                    src_buffer + 1, 
                    src_buf_size.sz_width * sizeof(src_buffer[0]),
                    src_buf_size, NULL, drv, NULL);
#if 0
            for (k = 0, i = 0; i < (win_h); i++) {
                for (j = 0; j < (win_w); j++, k++) {
                    printf("%f ", gy_buffer[k]);
                }
                printf("\n");
            }
            printf("gy\n");
#endif
            a = b = c = bb1 = bb2 = 0;

            /* process gradient */
            for (i = 0, k = 0; i < win_h; i++) {
                py = i - win.sz_height;
                for (j = 0; j < win_w; j++, k++) {
                    m = mask[k];
                    tgx = gx_buffer[k];
                    tgy = gy_buffer[k];
                    gxx = tgx * tgx * m;
                    gxy = tgx * tgy * m;
                    gyy = tgy * tgy * m;
                    px = j - win.sz_width;

                    a += gxx;
                    b += gxy;
                    c += gyy;

                    bb1 += gxx * px + gxy * py;
                    bb2 += gxy * px + gyy * py;
                }
            }

            det = a*c - b*b;
            if (fabs(det) > DBL_EPSILON*DBL_EPSILON) {
                // 2x2 matrix inversion
                scale = 1.0/det;
                ci2.pf_x = ci3.pf_x = 
                    (float)(ci.pf_x + c*scale*bb1 - b*scale*bb2);
                //ci2.pf_y = (float)(ci.pf_y - b*scale*bb1 + a*scale*bb2);
                ci3.pf_y = (float)((height - 1 - ci.pf_y) + b*scale*bb1 - a*scale*bb2);
                ci2.pf_y = height - 1 - ci3.pf_y;
            } else {
                ci2 = ci;
            }

            err = (ci2.pf_x - ci.pf_x) * (ci2.pf_x - ci.pf_x) + 
                (ci2.pf_y - ci.pf_y) * (ci2.pf_y - ci.pf_y);
            ci = ci2;
        } while (++iter < max_iters && err > eps);

        /* if new point is too far from initial, it means poor convergence.
           leave initial point as the result */
        if (fabs(ci.pf_x - ct.pf_x) > win.sz_width || 
                fabs(ci.pf_y - ct.pf_y) > win.sz_height) {
            ci = ct;
        }

        corners[pt_i] = ci;     /* store result */
    }
}

void
fv_find_corner_sub_pix(fv_image_t *img, fv_point_2D32f_t *corners,
                    fv_s32 count, fv_size_t win, fv_size_t zero_zone,
                    fv_term_criteria_t criteria)
{
    fv_mat_t        mat;

    mat = fv_image_to_mat(img);
    _fv_find_corner_sub_pix(&mat, corners, count, win, zero_zone, criteria);
}

static void 
_fv_calc_optical_flow_pyr_lk(fv_mat_t *prev_img, fv_mat_t *next_img,
                           fv_mat_t *prev_pts, fv_mat_t *next_pts,
                           fv_mat_t status, fv_mat_t err,
                           fv_size_t win_size, fv_s32 max_level,
                           fv_s32 npoints, fv_term_criteria_t criteria,
                           fv_s32 flags, double min_eig_threshold)
{
    fv_mat_t    *prev_pyr;
    fv_mat_t    *next_pyr;
    fv_mat_t    *guess;
    fv_s32      level = 0;
    //fv_s32      i;

    FV_ASSERT(max_level >= 0 && win_size.sz_width > 2 && 
            win_size.sz_height > 2);

    FV_ASSERT(npoints == next_pts->mt_total);
    memset(status.mt_data.dt_ptr, 1, npoints);

    prev_pyr = fv_alloc((max_level + 1)*sizeof(*prev_pyr));
    next_pyr = fv_alloc((max_level + 1)*sizeof(*next_pyr));
    FV_ASSERT(prev_pyr != NULL && next_pyr != NULL);

    fv_build_optical_flow_pyramid(prev_img, prev_pyr, win_size, max_level, 
            0, FV_BORDER_REFLECT_101, FV_BORDER_CONSTANT, 1);
    fv_build_optical_flow_pyramid(next_img, next_pyr, win_size, max_level,
            0, FV_BORDER_REFLECT_101, FV_BORDER_CONSTANT, 1);

    criteria.tc_max_iter = fv_min(fv_max(criteria.tc_max_iter, 0), 100);
    criteria.tc_epsilon = fv_min(fv_max(criteria.tc_epsilon, 0), 10.0);
    criteria.tc_epsilon *= criteria.tc_epsilon;

    guess = fv_create_mat(prev_img->mt_rows, prev_img->mt_cols, 
                    prev_img->mt_atr);
    FV_ASSERT(guess != NULL);
    for (level = max_level; level >= 0; level--) {
        fv_debug_save_img("prev_pyr", prev_pyr + level);
        fv_lk_tracker_invoker(&prev_pyr[level], &next_pyr[level], guess, 
                prev_pts, next_pts, status, err, npoints,
                win_size, criteria, level, max_level,
                flags, min_eig_threshold);
    }

    fv_release_mat(&guess);
    for (level = 0; level < max_level; level++) {
        _fv_release_mat(prev_pyr + level);
        _fv_release_mat(next_pyr + level);
    }

    fv_free(&next_pyr);
    fv_free(&prev_pyr);
}

/*
 * 计算一个稀疏特征集的光流,使用金字塔中的迭代 Lucas-Kanade 方法
 * @prev: 在时间 t 的第一帧
 * @curr: 在时间 t + dt 的第二帧
 * @prev_features: 需要发现光流的点集
 * @curr_features: 包含新计算出来的位置的点集
 * @features: 第二幅图像中
 * @count: 特征点的数目
 * @win_size: 每个金字塔层的搜索窗口尺寸
 * @level: 最大的金字塔层数。如果为 0 , 不使用金字塔 (即金字塔为单层), 
 *          如果为 1 , 使用两层,下面依次类推。
 * @status:数组。如果对应特征的光流被发现,数组中的每一个元素都被设置为 1, 否则设置为 0。
 * @error: 双精度数组,包含原始图像碎片与移动点之间的差。为可选参数,可以是 NULL .
 * @criteria: 准则,指定在每个金字塔层,为某点寻找光流的迭代过程的终止条件。
 * @flags:其它选项:
 * • CV_LKFLOW_PYR_A_READY , 在调用之前,先计算第一帧的金字塔
 * • CV_LKFLOW_PYR_B_READY , 在调用之前,先计算第二帧的金字塔))
 */
void 
fv_calc_optical_flow_pyr_lk(fv_image_t *prev, fv_image_t *curr,
        fv_point_2D32f_t *prev_features, fv_point_2D32f_t *curr_features,
        fv_s32 count, fv_size_t win_size, fv_s32 level, fv_s8 *status,
        float *error, fv_term_criteria_t criteria, fv_s32 flags)
{
    fv_mat_t    a;
    fv_mat_t    b;
    fv_mat_t    pt_a;
    fv_mat_t    pt_b;
    fv_mat_t    st;
    fv_mat_t    err;

    if (count <= 0) {
        return;
    }

    FV_ASSERT(prev_features && curr_features && prev->ig_channels == 1 &&
            curr->ig_channels == 1);

    a = fv_image_to_mat(prev);
    b = fv_image_to_mat(curr);

    pt_a = fv_mat(count, 1, FV_32FC2, prev_features);
    pt_b = fv_mat(count, 1, FV_32FC2, curr_features);

    st = fv_mat(count, 1, FV_8UC1, status);
    err = fv_mat(count, 1, FV_32FC1, error);

    _fv_calc_optical_flow_pyr_lk(&a, &b, &pt_a, &pt_b, st, 
            err, win_size, level, count, criteria, flags, 0);
}

