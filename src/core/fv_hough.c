
#include "fv_types.h"
#include "fv_math.h"
#include "fv_debug.h"
#include "fv_hough.h"
#include "fv_core.h"
#include "fv_log.h"
#include "fv_mem.h"
#include "fv_stat.h"
#include "fv_edge.h"
#include "fv_time.h"

#define hough_cmp_gt(l1,l2) (aux[l1] > aux[l2])

static FV_IMPLEMENT_QSORT_EX(fv_hough_sort, int, hough_cmp_gt, const int* )

static fv_s32
fv_hough_lines_standard(fv_mat_t *mat, float rho, float theta,
            fv_s32 threshold, fv_line_polar_t *lines, fv_s32 lines_max)
{
    fv_line_polar_t     *line;
    fv_u8               *data;
    fv_s32              *accum;
    fv_s32              *sort_buf;
    float               *tab_sin;
    float               *tab_cos;
    float               ang;
    float               irho = 1/rho;
    double              scale;
    fv_s32              numangle;
    fv_s32              numrho;
    fv_s32              mr;
    fv_s32              step;
    fv_s32              width;
    fv_s32              height;
    fv_s32              base;
    fv_s32              i;
    fv_s32              j;
    fv_s32              n;
    fv_s32              r;
    fv_s32              total;
    fv_s32              max;
    fv_s32              idx;

    FV_ASSERT(mat->mt_atr == FV_8UC1);

    step = mat->mt_step;
    width = mat->mt_cols;
    height = mat->mt_rows;
    numangle = fv_pi/theta;
    numrho = ((width + height) * 2 + 1)/rho;
    mr = ((numrho - 1) >> 1);

    accum = fv_calloc(sizeof(*accum)*(numangle + 2)*(numrho + 2));
    FV_ASSERT(accum != NULL);
    sort_buf = fv_calloc(numangle * numrho);
    FV_ASSERT(sort_buf != NULL);
    tab_sin = fv_alloc(sizeof(*tab_sin)*numangle*2);
    FV_ASSERT(tab_sin != NULL);
    tab_cos = tab_sin + numangle;
    for (i = 0, ang = 0; i < numangle; i++, ang += theta) {
        tab_sin[i] = sinf(ang)*irho;
        tab_cos[i] = cosf(ang)*irho;
    }

    // stage 1. fill accumulator
    for (i = 0; i < height; i++) {
        data = mat->mt_data.dt_ptr + i*step;
        for (j = 0; j < width; j++, data++) {
            if (*data == 0) {
                continue;
            }

            for (n = 0; n < numangle; n++) {
                r = i*tab_sin[n] + j*tab_cos[n];
                r += mr;
                accum[(n + 1)*(numrho + 2) + r + 1]++;
            }
        }
    }

    // stage 2. find local maximums
    total = 0;
    for (r = 0; r < numrho; r++) {
        for (n = 0; n < numangle; n++) {
            base = (n + 1)*(numrho + 2) + r + 1;
            if (accum[base] > threshold && accum[base] > accum[base - 1] &&
                    accum[base] >= accum[base + 1] && 
                    accum[base] > accum[base - numrho - 2] &&
                    accum[base] >= accum[base + numrho + 2]) {
                    sort_buf[total++] = base;
            }
        }
    }

    // stage 3. sort the detected lines by accumulator value
    fv_hough_sort(sort_buf, total, accum);

    // stage 4. store the first min(total,linesMax) lines to the output buffer
    max = fv_min(lines_max, total);
    line = lines;
    scale = 1.0/(numrho + 2);
    for (i = 0; i < max; i++, line++) {
        idx = sort_buf[i];
        n = (idx*scale - 1);
        r = idx - (n + 1)*(numrho + 2) - 1;
        line->lp_angle = n*theta;
        line->lp_rho = (r - mr)*rho;
        //printf("r = [%f %f] idx = %d\n", line->lp_rho, line->lp_angle, idx);
    }

    fv_free(&tab_sin);
    fv_free(&sort_buf);
    fv_free(&accum);

    //printf("max = %d, line_max = %d, total = %d\n", max, lines_max, total);

    return max;
}

static fv_s32
fv_hough_lines_probabilistic(fv_mat_t *mat, float rho, float theta,
            fv_s32 threshold, fv_s32 line_length, fv_s32 line_gap,
            fv_point_t *lines, fv_s32 lines_max)
{
    fv_mat_t                *mask;
    fv_point_t              *point;
    fv_point_t              *p;
    fv_s32                  *accum;
    fv_s32                  *adata;
    fv_u8                   *data;
    fv_u8                   *m;
    fv_u8                   *mdata;
    float                   *trigtab;
    float                   *ttab;
    fv_point_t              pt;
    fv_point_t              line_end[2];
    float                   a;
    float                   b;
    float                   ang;
    float                   irho = 1/rho;
    fv_s32                  numangle;
    fv_s32                  numrho;
    fv_s32                  mr;
    fv_s32                  width;
    fv_s32                  height;
    fv_s32                  step;
    fv_s32                  n;
    fv_s32                  count;
    fv_s32                  idx;
    fv_s32                  x;
    fv_s32                  y;
    fv_s32                  x0;
    fv_s32                  y0;
    fv_s32                  dx;
    fv_s32                  dy;
    fv_s32                  dx0;
    fv_s32                  dy0;
    fv_s32                  i;
    fv_s32                  j;
    fv_s32                  k;
    fv_s32                  i1;
    fv_s32                  j1;
    fv_s32                  r;
    fv_s32                  gap;
    fv_s32                  val;
    fv_s32                  max_val;
    fv_s32                  max_n;
    fv_s32                  shift = 16;
    fv_s32                  num = 0;
    fv_bool                 xflag;
    fv_bool                 good_line;

    step = mat->mt_step;
    width = mat->mt_cols;
    height = mat->mt_rows;
    numangle = fv_pi/theta;
    numrho = ((width + height) * 2 + 1)/rho;
    mr = ((numrho - 1) >> 1);

    mask = fv_create_mat(height, width, FV_8UC1);
    FV_ASSERT(mask != NULL);
    accum = fv_calloc(sizeof(*accum)*(numangle + 2)*(numrho + 2));
    FV_ASSERT(accum != NULL);
 
    trigtab = fv_alloc(sizeof(*trigtab)*numangle*2);
    FV_ASSERT(trigtab != NULL);

    ttab = trigtab;
    for (n = 0, ang = 0; n < numangle; n++, ang += theta, ttab += 2) {
        ttab[0] = cosf(ang)*irho;
        ttab[1] = sinf(ang)*irho;
    }
    ttab = trigtab;

    count = _fv_count_non_zero(mat);
    point = fv_alloc(sizeof(*point)*count);
    FV_ASSERT(point != NULL);
    // stage 1. collect non-zero image points
    for (pt.pt_y = 0, p = point; pt.pt_y < height; pt.pt_y++) {
        data = mat->mt_data.dt_ptr + pt.pt_y*step;
        m = mask->mt_data.dt_ptr + pt.pt_y*width;
        for (pt.pt_x = 0; pt.pt_x < width; pt.pt_x++) {
            if (data[pt.pt_x] != 0) {
                m[pt.pt_x] = 1;
                *p = pt;
                p++;
            } else {
                m[pt.pt_x] = 0;
            }
        }
    }

    // stage 2. process all the points in random order
    for (mdata = mask->mt_data.dt_ptr; count > 0; count--) {
        idx = random() % count;
        pt = point[idx];
        point[idx] = point[count - 1];
        i = pt.pt_x;
        j = pt.pt_y;
        // check if it has been excluded already
        // (i.e. belongs to some other line)
        if (mdata[j*width + i] == 0) {
            continue;
        }

        adata = accum;
        ttab = trigtab;
        max_n = 0;
        max_val = threshold - 1;
        // update accumulator, find the most probable line
        for (n = 0; n < numangle; n++, adata += numrho, ttab += 2) {
            r =  i*ttab[0] + j*ttab[1];
            r += mr;
            val = ++adata[r];
        //    printf("r = %d %d, x = %d y = %d, cos = %f, sin = %f n = %d\n",
         //           r, r - mr, i, j, ttab[0], ttab[1],n);
            if (max_val < val) {
                max_val = val;
                max_n = n;
            }
        }
        // if it is too "weak" candidate, continue with another point
        if (max_val < threshold) {
            continue;
        }

        // from the current point walk in each direction
        // along the found line and extract the line segment
        a = trigtab[2*max_n + 1];  //sin
        b = trigtab[2*max_n];       //cos
        x0 = i;
        y0 = j;
        if (fabs(a) > fabs(b)) {
            xflag = 1;
            dx0 = 1;
            dy0 = -b*(1 << shift)/a;
            y0 = (y0 << shift) + (1 << (shift - 1));
        } else {
            xflag = 0;
            dy0 = 1;
            dx0 = -a*(1 << shift)/b;
            x0 = (x0 << shift) + (1 << (shift - 1));
        }

        for (k = 0; k < 2; k++) {
            gap = 0, x = x0, y = y0, dx = dx0, dy = dy0;
            if (k > 0) {
                dx = -dx, dy = -dy;
            }

            // walk along the line using fixed-point arithmetics,
            // stop at the image border or in case of too big gap
            for (;; x += dx, y += dy) {
                if( xflag ) {
                    j1 = x;
                    i1 = (y >> shift);
                } else {
                    j1 = (x >> shift);
                    i1 = y;
                }

                if (j1 < 0 || j1 >= width || i1 < 0 || i1 >= height) {
                    break;
                }

                m = mask->mt_data.dt_ptr + i1*width + j1;

                // for each non-zero point:
                //    update line end,
                //    clear the mask element
                //    reset the gap
                if (*m) {
                    gap = 0;
                    line_end[k].pt_y = i1;
                    line_end[k].pt_x = j1;
                } else if (++gap > line_gap) {
                    break;
                }
            }
        }

        good_line = abs(line_end[1].pt_x - line_end[0].pt_x) >= line_length ||
                    abs(line_end[1].pt_y - line_end[0].pt_y) >= line_length;

        for (k = 0; k < 2; k++) {
            x = x0, y = y0, dx = dx0, dy = dy0;
            if( k > 0 ) {
                dx = -dx, dy = -dy;
            }

            // walk along the line using fixed-point arithmetics,
            // stop at the image border or in case of too big gap
            for (;; x += dx, y += dy) {
                if (xflag) {
                    j1 = x;
                    i1 = y >> shift;
                } else {
                    j1 = x >> shift;
                    i1 = y;
                }

                m = mask->mt_data.dt_ptr + i1*width + j1;

                // for each non-zero point:
                //    update line end,
                //    clear the mask element
                //    reset the gap
                if (*m) {
                    if (good_line) {
                        adata = accum;
                        ttab = trigtab;
                        for (n = 0; n < numangle; n++, adata += numrho,
                                ttab += 2) {
                            r = j1 * ttab[0] + i1 * ttab[1];
                            r += mr;
                            adata[r]--;
                        }
                    }
                    *m = 0;
                }

                if (i1 == line_end[k].pt_y && j1 == line_end[k].pt_x) {
                    break;
                }
            }
        }

        if (good_line) {
            lines[0] = line_end[0];
            lines[1] = line_end[1];
            lines += 2;
            num++;
            if (num >= lines_max) {
                return num;
            }
        }
    }

    fv_free(&point);
    fv_release_mat(&mask);
    fv_free(&trigtab);
    fv_free(&accum);

    return num;
}

static fv_s32
fv_hough_lines_sdiv(fv_mat_t *mat, float rho, float theta,
            fv_s32 threshold, fv_s32 srn, fv_s32 stn,
            fv_line_polar_t *lines, fv_s32 lines_max)
{
    return 0;
}


/*
 * fv_hough_lines: 利用 Hough 变换在二值图像中找到直线
 * image: 输入 8-比特、单通道 (二值) 图像,其内容可能被函数所改变
 * line_storage:
 *      检测到的线段存储仓.
 * len: line_storage的长度(字节).
 * method: Hough 变换变量,是下面变量的其中之一:
 *      • CV_HOUGH_STANDARD - 传统或标准 Hough 变换. 每一个线段由两个浮点数 (ρ, θ)
 *          表示,其中 ρ 是点与原点 (0,0) 之间的距离,θ 线段与 x-轴之间的夹角。所以
 *          line_storage的存储类型为fv_line_polar_t
 *      • CV_HOUGH_PROBABILISTIC - 概率 Hough 变换(如果图像包含一些长的线性分割,
 *          则效率更高). 它返回线段分割而不是整个线段。每个分割用起点和终点来表示,所
 *          以line_storage的存储类型为fv_line_t
 *      • CV_HOUGH_MULTI_SCALE - 传统 Hough 变换的多尺度变种。线段的编码方式与
 *          CV_HOUGH_STANDARD 的一致。
 * rho: 与象素相关单位的距离精度
 * theta: 弧度测量的角度精度
 * threshold: 阈值参数。如果相应的累计值大于 threshold, 则函数返回的这个线段.
 * param1: 第一个方法相关的参数:
 *      • 对传统 Hough 变换,不使用(0).
 *      • 对概率 Hough 变换,它是最小线段长度.
 *      • 对多尺度 Hough 变换,它是距离精度 rho 的分母 (大致的距离精度是 rho 而精确
 *          的应该是 rho / param1 ).
 * param2: 第二个方法相关参数:
 *      • 对传统 Hough 变换,不使用 (0).
 *      • 对概率 Hough 变换,这个参数表示在同一条直线上进行碎线段连接的最大间隔值
 *          (gap), 即当同一条直线上的两条碎线段之间的间隔小于 param2 时,将其合二为一。
 *      • 对多尺度 Hough 变换,它是角度精度 theta 的分母 (大致的角度精度是 theta 而
 *          精确的角度应该是 theta / param2 ).
 */
fv_s32 
fv_hough_lines(fv_image_t *image, void *line_storage, fv_s32 len,
        fv_s32 method, double rho, double theta, fv_s32 threshold, 
        double param1, double param2)
{
    fv_mat_t    img;
    fv_s32      num = 0;

    img = fv_image_to_mat(image);

    switch (method) {
        case FV_HOUGH_STANDARD:
            num = fv_hough_lines_standard(&img, rho, theta, threshold, 
                    line_storage, len/sizeof(fv_line_polar_t));
            break;
        case FV_HOUGH_PROBABILISTIC: 
            num = fv_hough_lines_probabilistic(&img, rho, theta, threshold,
                param1, param2, line_storage, len/(2*sizeof(fv_point_t)));
            break;
        case FV_HOUGH_MULTI_SCALE:
            num = fv_hough_lines_sdiv(&img, rho, theta, threshold,
                param1, param2, line_storage, len/sizeof(fv_point_t));
            break;
        default:
            FV_LOG_ERR("Unknow method %d\n", method);
            break;
    }

    return num;
}

#define FV_HOUGH_SHIFT      10
#define FV_HOUGH_ONE        (1 << FV_HOUGH_SHIFT)
static fv_s32
fv_hough_circles_gradient(fv_mat_t *mat, float dp, float min_dist,
                         fv_s32 min_radius, fv_s32 max_radius,
                         fv_s32 canny_threshold, fv_s32 acc_threshold,
                         fv_circle_t *circles, fv_s32 max_num,
                         fv_s32 circles_max)
{
    fv_circle_t     *c;
    fv_mat_t        *edges;
    fv_mat_t        *dx;
    fv_mat_t        *dy;
    fv_mat_t        *accum = NULL;
    fv_mat_t        *dist_buf = NULL;
    fv_u8           *edges_row = NULL;
    fv_s16          *dx_row;
    fv_s16          *dy_row;
    fv_s32          *adata;
    fv_s32          *sort_buf;
    fv_s32          *sort_buf2;
    fv_point_t      *nz;
    float           *ddata;
    fv_point_t      pt;
    float           idp;
    float           dr;
    float           vx;
    float           vy;
    float           _dx;
    float           _dy;
    float           _r2;
    float           mag;
    float           cx;
    float           cy;
    float           start_dist;
    float           d;
    float           r_best = 0;
    float           r_cur;
    float           min_radius2 = min_radius*min_radius;
    float           max_radius2 = max_radius*max_radius;
    fv_s32          x;
    fv_s32          y;
    fv_s32          rows;
    fv_s32          cols;
    fv_s32          arows;
    fv_s32          acols;
    fv_s32          sx;
    fv_s32          sy;
    fv_s32          x0;
    fv_s32          y0;
    fv_s32          x1;
    fv_s32          y1;
    fv_s32          x2;
    fv_s32          y2;
    fv_s32          r;
    fv_s32          i;
    fv_s32          j;
    fv_s32          k;
    fv_s32          k1;
    fv_s32          astep;
    fv_s32          count;
    fv_s32          nz_count;
    fv_s32          base;
    fv_s32          center_count;
    fv_s32          ofs;
    fv_s32          max_count = 0;
    fv_s32          circles_total = 0;
    fv_s32          nz_count1;
    fv_s32          start_idx;
    fv_s32          width;
    fv_s32          height;

    width = mat->mt_cols;
    height = mat->mt_rows;
    edges = fv_create_mat(height, width, FV_8UC1);
    FV_ASSERT(edges != NULL);

    _fv_canny(edges, mat, fv_max(canny_threshold/2,1), canny_threshold, 3);

    dx = fv_create_mat(height, width, FV_16SC1);
    FV_ASSERT(dx != NULL);
    dx->mt_depth = FV_16S;
    dy = fv_create_mat(height, width, FV_16SC1);
    FV_ASSERT(dy != NULL);
    dy->mt_depth = FV_16S;

    _fv_sobel(dx, mat, FV_16S, 1, 0, 3, 1, 0, FV_BORDER_CONSTANT);
    _fv_sobel(dy, mat, FV_16S, 0, 1, 3, 1, 0, FV_BORDER_CONSTANT);

    if (dp < 1.0) {
        dp = 1.0;
    }

    idp = 1.0/dp;
    accum = fv_create_mat(mat->mt_rows*idp + 2, mat->mt_cols*idp + 2, FV_32SC1);
    FV_ASSERT(accum != NULL);
    accum->mt_depth = FV_32S;

    sort_buf = fv_alloc(sizeof(*sort_buf)*mat->mt_total);
    FV_ASSERT(sort_buf != NULL);

    rows = mat->mt_rows;
    cols = mat->mt_cols;

    arows = accum->mt_rows - 2;
    acols = accum->mt_cols - 2;
    adata = accum->mt_data.dt_i;
    memset(adata, 0, accum->mt_total_size);
    astep = accum->mt_step/sizeof(adata[0]);

    count = _fv_count_non_zero(edges);
    nz = fv_alloc(sizeof(*nz)*count);
    FV_ASSERT(nz != NULL);
 
    sort_buf2 = fv_alloc(sizeof(*sort_buf2)*count);
    FV_ASSERT(sort_buf2 != NULL);
    nz_count = 0;
    // Accumulate circle evidence for each edge pixel
    for (y = 0; y < rows; y++) {
        edges_row = edges->mt_data.dt_ptr + y*edges->mt_step;
        dx_row = dx->mt_data.dt_s + y*width;
        dy_row = dy->mt_data.dt_s + y*width;
        for (x = 0; x < cols; x++) {
            vx = dx_row[x];
            vy = dy_row[x];

            if (!edges_row[x] || (vx == 0 && vy == 0)) {
                continue;
            }

            mag = sqrt(vx*vx+vy*vy);
            FV_ASSERT(mag >= 1);
            sx = vx*idp*FV_HOUGH_ONE/mag;
            sy = vy*idp*FV_HOUGH_ONE/mag;

            x0 = x*idp*FV_HOUGH_ONE;
            y0 = y*idp*FV_HOUGH_ONE;
            // Step from min_radius to max_radius in both directions of the gradient
            for (k1 = 0; k1 < 2; k1++) {
                x1 = x0 + min_radius * sx;
                y1 = y0 + min_radius * sy;
                for (r = min_radius; r <= max_radius; x1 += sx, y1 += sy, r++) {
                    x2 = x1 >> FV_HOUGH_SHIFT, y2 = y1 >> FV_HOUGH_SHIFT;
                    if( (unsigned)x2 >= (unsigned)acols ||
                        (unsigned)y2 >= (unsigned)arows ) {
                        break;
                    }
                    adata[y2*astep + x2]++;
                }

                sx = -sx; sy = -sy;
            }

            pt.pt_x = x; 
            pt.pt_y = y;
            nz[nz_count++] = pt;
        }
    }

    FV_ASSERT(nz_count <= count);
    if (!nz_count) {
        goto out;
    }

    //Find possible circle centers
    center_count = 0;
    for (y = 1; y < arows - 1; y++) {
        for (x = 1; x < acols - 1; x++) {
            base = y*(acols + 2) + x;
            if (adata[base] > acc_threshold &&
                adata[base] > adata[base - 1] && 
                adata[base] > adata[base + 1] &&
                adata[base] > adata[base - acols - 2] && 
                adata[base] > adata[base + acols + 2]) {
                sort_buf[center_count++] = base;
            }
        }
    }

    if (!center_count) {
        goto out;
    }

    fv_hough_sort(sort_buf, center_count, adata);
    dist_buf = fv_create_mat(1, nz_count, FV_32FC1);
    dist_buf->mt_depth = FV_32F;
    ddata = dist_buf->mt_data.dt_fl;

    dr = dp;
    min_dist = fv_max(min_dist, dp);
    min_dist *= min_dist;
    // For each found possible center
    // Estimate radius and check support
    for (circles_total = 0, i = 0; i < center_count; i++) {
        ofs = sort_buf[i];
        y = ofs/(acols + 2);
        x = ofs - y*(acols + 2);
        //Calculate circle's center in pixels
        cx = (float)((x + 0.5f)*dp);
        cy = (float)((y + 0.5f)*dp);
        // Check distance with previously detected circles
        for (j = 0; j < circles_total; j++) {
            c = circles + j;
            if ((c->cl_center.pt_x - cx)*(c->cl_center.pt_x - cx) + 
                    (c->cl_center.pt_y - cy)*(c->cl_center.pt_y - cy) < 
                    min_dist) {
                break;
            }
        }

        if (j < circles_total) {
            continue;
        }
        // Estimate best radius
        for (j = k = 0; j < nz_count; j++) {
            pt = nz[j];
            _dx = cx - pt.pt_x; 
            _dy = cy - pt.pt_y;
            _r2 = _dx*_dx + _dy*_dy;
            if (min_radius2 <= _r2 && _r2 <= max_radius2) {
                ddata[k] = _r2;
                sort_buf2[k] = k;
                k++;
            }
        }

        nz_count1 = k;
        fv_hough_sort(sort_buf2, nz_count1, (fv_s32 *)ddata);
        start_idx = nz_count1 - 1;
        if (nz_count1 == 0) {
            continue;
        }
        dist_buf->mt_cols = nz_count1;
        fv_pow(dist_buf, dist_buf, 0.5);

        start_dist = ddata[sort_buf2[nz_count1 - 1]];
        max_count = 0;
        r_best = 0;
        for (j = nz_count1 - 2; j >= 0; j--) {
            d = ddata[sort_buf2[j]];
            if (d > max_radius) {
                break;
            }

            if (d - start_dist > dr) {
                r_cur = ddata[sort_buf2[(j + start_idx)/2]];
                if ((start_idx - j)*r_best >= max_count*r_cur ||
                    (r_best < FLT_EPSILON && start_idx - j >= max_count) ) {
                    r_best = r_cur;
                    max_count = start_idx - j;
                }
                start_dist = d;
                start_idx = j;
            }
        }

        // Check if the circle has enough support
        if (max_count > acc_threshold) {
            c = &circles[circles_total++];
            c->cl_center.pt_x = cx;
            c->cl_center.pt_y = cy;
            c->cl_radius = r_best;
            printf("total = %d, center=[%d %d], r = %f\n",
                    circles_total, c->cl_center.pt_x, c->cl_center.pt_y, r_best);
            if (circles_total > circles_max) {
                goto out;
            }
        }
    }

out:
    fv_release_mat(&dist_buf);
    fv_free(&sort_buf2);
    fv_free(&sort_buf);
    fv_free(&nz);
    fv_release_mat(&accum);
    fv_release_mat(&dy);
    fv_release_mat(&dx);
    fv_release_mat(&edges);

    return circles_total;
}

/*
 * fv_hough_circles: 利用 Hough 变换在灰度图中找圆
 * image: 输入 8-比特、单通道图像
 * circls_storage:
 *      检测到的圆存储仓.
 * len: circle_storage的长度(字节).
 * method: Hough 变换变量, 只能是FV_HOUGH_GRADIENT
 * dp: 累加器图像的分辨率; 必须不能小于1; 是1时分辨率与输入图像一致
 * min_dist: 两个不同的圆之间的最小距离
 * param1: Canny边缘阈值
 * param2: 累加器阈值
 * min_radius: 所能发现的圆半径的最小值 
 * max_radius: 所能发现的圆半径的最大值 
 */
fv_s32
fv_hough_circles(fv_image_t *image, void *circle_storage,
                fv_s32 len, fv_s32 method, double dp,
                double min_dist, double param1, double param2,
                fv_s32 min_radius, fv_s32 max_radius)
{
    fv_mat_t        img;
    fv_s32          num = 0;
    fv_s32          circles_max = INT_MAX;
    fv_s32          canny_threshold = param1;
    fv_s32          acc_threshold = param2;

    img = fv_image_to_mat(image);
    min_radius = fv_max(min_radius, 0);
    if (max_radius <= 0) {
        max_radius = fv_max(img.mt_rows, img.mt_cols);
    } else if (max_radius <= min_radius) {
        max_radius = min_radius + 2;
    }

    switch (method) {
        case FV_HOUGH_GRADIENT:
            num = fv_hough_circles_gradient(&img, dp, min_dist,
                                min_radius, max_radius, canny_threshold,
                                acc_threshold, circle_storage, len/sizeof(fv_circle_t),
                                circles_max);
            break;
        default:
            FV_LOG_ERR("Unknow method %d\n", method);
            break;
    }

    return num;
}

