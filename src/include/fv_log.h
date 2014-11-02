#ifndef __FV_LOG_H__
#define __FV_LOG_H__

#include <stdio.h>

#include "fv_types.h"

#define FV_LOG_PRINT_ERR(fmt, ...) \
    do {\
        fprintf(stderr, "Function: %s Line: %d ", __FUNCTION__,__LINE__); \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
    } while (0)


#define FV_LOG_PRINT(fmt, ...) \
    do {\
        if (fv_log_on) \
        { \
            fprintf(stdout, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

extern fv_bool fv_log_on;

extern void fv_log_enable(void);
extern void fv_log_disable(void);

#endif
