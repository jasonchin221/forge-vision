#include <opencv/cv.h>  
#include <opencv/highgui.h>

#include "fv_types.h"
#include "fv_log.h"
#include "fv_core.h"

fv_s32 
fv_lk_optical_flow(IplImage *cv_img, fv_bool image)
{
    fv_image_t      *img;

    img = fv_convert_image(cv_img);
    if (img == NULL) {
        FV_LOG_ERR("Convert image faield!\n");
    }

    fv_release_image(&img);

    return FV_OK;
}
