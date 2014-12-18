#ifndef __FV_BORDER_H__
#define __FV_BORDER_H__

typedef struct _fv_border_value_t {
    fv_u32      bv_type;
    fv_s32      (*bv_get_value)(fv_s32, fv_s32);
} fv_border_value_t;

typedef void (*fv_border_make_row_func)(void *, void *, fv_u32, fv_s32, 
            fv_s32, fv_u32, fv_s32,  fv_s32, fv_s32);

extern fv_s32 fv_border_get_value(fv_u32 border_type, 
            fv_s32 index, fv_s32 border);
extern fv_border_make_row_func fv_border_get_func(fv_u32 depth);

#endif
