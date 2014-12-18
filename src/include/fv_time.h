#ifndef __FV_TIME_H__
#define __FV_TIME_H__

enum {
    FV_TIME_METER1,
    FV_TIME_METER2,
    FV_TIME_METER3,
    FV_TIME_METER4,
    FV_TIME_METER5,
    FV_TIME_METER_MAX,
};

extern fv_u32 fv_time_sec(void);
extern void fv_time_meter_set(fv_u32 id);
extern void _fv_time_meter_get(fv_u32 id, fv_u32 thresh, 
            const char *func, fv_s32 line);

#define fv_time_meter_get(id, thresh) \
    _fv_time_meter_get(id, thresh, __FUNCTION__, __LINE__);

#endif
