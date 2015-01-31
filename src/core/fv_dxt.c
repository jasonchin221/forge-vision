
#include "fv_types.h"
#include "fv_debug.h"
#include "fv_core.h"
#include "fv_math.h"
#include "fv_dxt.h"
#include "fv_time.h"
#include "fv_mem.h"

typedef void (*fv_dft_bow_func)(float, float, float, float, float, 
        float *, float *, float *, float *);

fv_s32
fv_get_optimal_dft_size(fv_s32 size)
{
    fv_s32      s;

    s = log(size)/log(2);
    if ((1 << s) != size) {
        s++;
    }

    return (1 << s);
}

static void
fv_dft_bow(float r1, float i1, float r2, float i2, float theta, 
        float *or1, float *oi1, float *or2, float *oi2)
{
    float       r;
    float       i;

    fv_complex_multiply(r2, i2, cos(theta), -sin(theta), r, i);

    r1 *= 0.5;
    r *= 0.5;
    i1 *= 0.5;
    i *= 0.5;
    *or1 = r1 + r;
    *oi1 = i1 + i;
    *or2 = r1 - r;
    *oi2 = i1 - i;
}

static void
fv_dft_bow_inverse(float r1, float i1, float r2, float i2, float theta, 
        float *or1, float *oi1, float *or2, float *oi2)
{
    float       r;
    float       i;

    fv_complex_multiply(r2, i2, cos(theta), sin(theta), r, i);

    *or1 = r1 + r;
    *oi1 = i1 + i;
    *or2 = r1 - r;
    *oi2 = i1 - i;
}

static void
fv_fft(float *data, fv_s32 n, fv_s32 m, float pi2, fv_dft_bow_func bow)
{
    float       *f1;
    float       *f2;
    float       theta;
    fv_s32      y;
    fv_s32      z;
    fv_s32      k;
    fv_s32      g;
    fv_s32      off;
    fv_s32      off_g;

    /* m级蝶形运算 */
    for (k = 0; k < m; k++) {
        g = (1 << k);
        off = (g << 1);
        off_g = (off << 1);
        for (y = 0; y < n >> (k + 1); y++) {
            for (z = 0; z < g; z++) {
                f1 = data + y*off_g + (z << 1);
                f2 = f1 + off;
                theta = pi2*z/(1 << (k + 1));
                bow(f1[0], f1[1], f2[0], f2[1], theta, f1, f1 + 1,
                        f2, f2 + 1);
            }
        }
    }
}

void
fv_fft_inverse_row(float *dst, float *src, fv_s32 n, fv_s32 m, float pi2)
{
    float       *b;
    fv_s32      k;
    fv_s32      i;
    fv_s32      l;

    /* 初始化输入数据 */
    for (k = 0; k < n; k++) {
        b = &dst[2*k];
        i = fv_num_map(k, m);
        l = (i << 1);
        b[0] = src[l];
        b[1] = src[l + 1];
    }

    fv_fft(dst, n, m, pi2, fv_dft_bow_inverse);
}
 
static void
fv_fft_inverse_column(float *dst, float *src, fv_s32 n, fv_s32 m, 
            fv_s32 step, float pi2)
{
    float       *b;
    fv_s32      k;
    fv_s32      i;
    fv_s32      l;

    /* 初始化输入数据 */
    for (k = 0; k < n; k++) {
        b = &dst[2*k];
        i = fv_num_map(k, m);
        l = i*step;
        b[0] = src[l];
        b[1] = src[l + 1];
    }

    fv_fft(dst, n, m, pi2, fv_dft_bow_inverse);
}
 
static void
fv_dft_inverse(fv_mat_t *dst, fv_mat_t *src,
        fv_s32 m, fv_s32 n, float *col_data)
{
    fv_mat_t    *tmp;
    float       *dst_data;
    float       *src_data;
    float       pi2;
    fv_s32      row;
    fv_s32      col;
    fv_s32      width;
    fv_s32      height;
    fv_s32      step;
    fv_s32      offset;

    width = dst->mt_cols;
    height = dst->mt_rows;

    pi2 = fv_pi*2;
    src_data = src->mt_data.dt_fl;
    step = 2*width;
    tmp = fv_create_mat(height, width, FV_32FC2);
    FV_ASSERT(tmp != NULL);
    dst_data = tmp->mt_data.dt_fl;
    for (row = 0; row < height; row++, dst_data += step, src_data += step) {
        fv_fft_inverse_row(dst_data, src_data, width, m, pi2);
    }

    dst_data = dst->mt_data.dt_fl;
    src_data = tmp->mt_data.dt_fl;
    for (col = 0; col < width; col++) {
        offset = (col << 1);
        fv_fft_inverse_column(col_data, src_data + offset,
                height, n, step, pi2);
        for (row = 0; row < height; row++) {
            dst_data[row*width + col] = col_data[2*row];
        }
    }

    fv_release_mat(&tmp);
}


void
fv_fft_real(float *dst, float *src, fv_s32 n, fv_s32 m, float pi2)
{
    float       *b;
    fv_s32      k;
    fv_s32      i;

    /* 初始化输入数据 */
    for (k = 0; k < n; k++) {
        b = &dst[2*k];
        i = fv_num_map(k, m);
        b[0] = src[i];
        b[1] = 0;
    }

    fv_fft(dst, n, m, pi2, fv_dft_bow);
}
 
static void
fv_fft_complex(float *dst, float *src, fv_s32 n, fv_s32 m, 
            fv_s32 step, float pi2)
{
    float       *b;
    fv_s32      k;
    fv_s32      i;
    fv_s32      l;

    /* 初始化输入数据 */
    for (k = 0; k < n; k++) {
        b = &dst[2*k];
        i = fv_num_map(k, m);
        l = i*step;
        b[0] = src[l];
        b[1] = src[l + 1];
    }

    fv_fft(dst, n, m, pi2, fv_dft_bow);
}
 
void
fv_dft(fv_mat_t *dst, fv_mat_t *src, fv_s32 flags, fv_s32 nonzero_rows)
{
    fv_mat_t    *column;
    float       *dst_data;
    float       *src_data;
    float       *col_data;
    float       pi2;
    fv_s32      m;
    fv_s32      n;
    fv_s32      row;
    fv_s32      col;
    fv_s32      offset;
    fv_s32      width;
    fv_s32      height;
    fv_s32      step;

    width = dst->mt_cols;
    height = dst->mt_rows;

    column = fv_create_mat(fv_max(width, height), 1, FV_32FC2);
    FV_ASSERT(column != NULL);

    m = log(width)/log(2);
    FV_ASSERT((1 << m) == width);

    n = log(height)/log(2);
    FV_ASSERT((1 << n) == height);

    col_data = column->mt_data.dt_fl;

    if (flags == FV_DXT_INVERSE) {
        FV_ASSERT(height == src->mt_rows && width == src->mt_cols &&
                dst->mt_atr == FV_32FC1 && src->mt_atr == FV_32FC2);

        fv_dft_inverse(dst, src, m, n, col_data);
        fv_release_mat(&column);
        return ;
    }

    FV_ASSERT(width == src->mt_cols &&
            dst->mt_atr == FV_32FC2 && src->mt_atr == FV_32FC1);

    pi2 = fv_pi*2;
    dst_data = dst->mt_data.dt_fl;
    src_data = src->mt_data.dt_fl;
    step = 2*width;
    fv_time_meter_set(3);
    for (row = 0; row < height; row++, dst_data += step, src_data += width) {
        fv_fft_real(dst_data, src_data, width, m, pi2);
    }

    dst_data = dst->mt_data.dt_fl;
    for (col = 0; col < width; col++) {
        offset = (col << 1);
        fv_fft_complex(col_data, dst_data + offset, height, n, step, pi2);
        for (row = 0; row < height; row++) {
            memcpy(&dst_data[row*step + offset], &col_data[2*row],
                    sizeof(*col_data)*2);
        }
    }
    fv_time_meter_get(3, 0);
    fv_release_mat(&column);
}

void
fv_mul_spectrums(fv_mat_t *dst, fv_mat_t *dft_a, fv_mat_t *dft_b)
{
    float       *d;
    float       *a;
    float       *b;
    fv_s32      total;
    fv_s32      k;
    float       r;
    float       i;

    FV_ASSERT(dst->mt_atr == dft_a->mt_atr && dft_a->mt_atr == dft_b->mt_atr && 
            dst->mt_rows == dft_a->mt_rows && 
            dft_a->mt_rows == dft_b->mt_rows &&
            dst->mt_cols == dft_a->mt_cols && dft_a->mt_cols == dft_b->mt_cols);

    total = dst->mt_rows*dst->mt_cols;
    d = dst->mt_data.dt_fl;
    a = dft_a->mt_data.dt_fl;
    b = dft_b->mt_data.dt_fl;
    for (k = 0; k < total; k++, d += 2,a += 2, b += 2) {
        fv_complex_multiply(a[0], a[1], b[0], b[1], r, i);
        d[0] = r*total;
        d[1] = i*total;
    }
}
