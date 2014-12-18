#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "fv_types.h"
#include "fv_time.h"

static fv_u32 fv_time_meter[FV_TIME_METER_MAX];

static fv_u32
fv_time_us(struct timeval *tv)
{
    return tv->tv_sec*1000000 + tv->tv_usec;
}

fv_u32
fv_time_sec(void)
{
    struct timeval  now;

    gettimeofday(&now, NULL);

    return fv_time_us(&now)/1000000;
}

void
fv_time_meter_set(fv_u32 id)
{
    struct timeval  now;

    if (id >= FV_TIME_METER_MAX) {
        return;
    }

    gettimeofday(&now, NULL);
    fv_time_meter[id] = fv_time_us(&now);
}

void
_fv_time_meter_get(fv_u32 id, fv_u32 thresh, const char *func, fv_s32 line)
{
    struct timeval  now;
    fv_u32          used;

    if (id >= FV_TIME_METER_MAX) {
        return;
    }

    gettimeofday(&now, NULL);

    used = fv_time_us(&now) - fv_time_meter[id];
    if (used >= thresh) {
        fprintf(stdout, "[%s, %d]Time-meter%d used %d ms, %d us\n", 
                func, line, id, used/1000, used);
    }
}
