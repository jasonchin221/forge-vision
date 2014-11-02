#ifndef __FV_APP_H__
#define __FV_APP_H__

#include "fv_opencv.h"

typedef struct _fv_app_proc_file_t {
    char    *pf_type;
    fv_bool pf_use_file;
    fv_s32  (*pf_proc_func)(char *, fv_proc_func);
} fv_app_proc_file_t; 

enum {
    FV_PROC_MODE_FORGEVISION,
    FV_PROC_MODE_OPENCV,
    FV_PROC_MODE_MAX,
};

typedef struct _fv_app_algorithm_t {
    char            *ag_name;
    fv_proc_func    ag_func[FV_PROC_MODE_MAX];
} fv_app_algorithm_t; 

#endif
