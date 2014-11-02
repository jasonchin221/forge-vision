#ifndef __FV_TYPES_H__
#define __FV_TYPES_H__

#include <stdbool.h>
#include <stddef.h>

#define FV_OK               0
#define FV_ERROR            (-1)

#define fv_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/*
 * fv_container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 */
#define fv_container_of(ptr, type, member) ({          \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef unsigned long           fv_ulong;
typedef unsigned long long      fv_u64;
typedef unsigned int            fv_u32;
typedef unsigned short          fv_u16;
typedef unsigned char           fv_u8;

typedef long                    fv_long;
typedef long long               fv_s64;
typedef int                     fv_s32;
typedef short                   fv_s16;
typedef char                    fv_s8;

typedef bool                    fv_bool;

typedef struct _fv_point_t {
    fv_s32          pt_x;
    fv_s32          pt_y;
} fv_point_t;

typedef struct _fv_size_t {
    fv_s32          sz_width;
    fv_s32          sz_height;
} fv_size_t;

#endif
