#include <stdio.h>
#include <sys/timeb.h>

#include "fv_types.h"
#include "fv_log.h"
#include "fv_debug.h"

fv_bool     fv_debug_on = 0;
fv_u32      fv_debug_count = 0;
char        *fv_debug_output_dir = NULL;
static fv_save_img_func fv_save_img;

static char *fv_format_postfix[FV_PIC_FORMAT_MAX] = {
    "jpg",
    "png",
    "bmp",
};

fv_s32
fv_gen_file_name(char *output, const char *file_name,
        const char *output_dir, fv_u32 output_buf_len, fv_u32 format)
{
    char            *buff;
    struct timeb    tb;
    fv_s32          offset = 0;
    fv_s32          buff_size;

    if (format >= FV_PIC_FORMAT_MAX) {
        FV_LOG_ERR("Unknow format %d!\n", format);
    }

    buff_size = output_buf_len;
    if (output_dir != NULL) {
        offset = snprintf(output, buff_size, "%s/", output_dir);
        buff_size -= offset;
        if (buff_size <= 0) {
            FV_LOG_ERR("Can't generate name, name is too long!\n");
        }
    }

    buff = &output[offset];
    offset = snprintf(buff, buff_size, "%s", file_name);
    buff_size -= offset;
    if (buff_size < 20) {
        FV_LOG_ERR("Can't generate name, name is too long!\n");
    }

    ftime(&tb);
    buff += offset;
    *buff++ = '-';
    offset = snprintf(buff, buff_size, "%d-%d", (int)tb.time, tb.millitm);
    buff += offset;
    buff_size -= offset;
    snprintf(buff, buff_size, ".%s", fv_format_postfix[format]);

    /* 
     * 睡眠至少1ms后再生成名字
     * 需要避免出现同名文件写覆盖的问题 
     */
    usleep(1000);

    return FV_OK;
}

void
fv_debug_save_img(char *file_name, fv_mat_t *mat)
{
    char            img_name[FV_DEBUG_MAX_FILENAME_LEN] = {};

    if (fv_save_img == NULL) {
        return;
    }

    if (fv_gen_file_name(img_name, file_name, 
        fv_debug_output_dir, sizeof(img_name), 
        FV_PIC_FORMAT_PNG) != FV_OK) {
        return;
    }

    fprintf(stdout, "Generat file %s\n", img_name);

    fv_save_img(img_name, mat);
}

void
fv_debug_set_print(fv_save_img_func func)
{
    fv_save_img = func;
}
