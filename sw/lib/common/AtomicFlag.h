#ifndef ATOMIC_FLAG_H
#define ATOMIC_FLAG_H

#include <stdbool.h>
#include "Assert_Common.h"
#include <stdatomic.h>
#include <string.h>

/* ================================================== */
/*                    ENUMS & TYPES                   */
/* ================================================== */
#define ATOMIC_FLAG_API static inline

#define MAX_TAG_LEN (32)

typedef enum {
    FLAG_CLEAR = 0,
    FLAG_SET = 1,
} FlagStatus_t;

typedef struct{
    atomic_uint_fast64_t atomicFlag;
    char TAG[MAX_TAG_LEN];
} AtomicFlag_t;

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

ATOMIC_FLAG_API void AtomicFlag_Init(AtomicFlag_t* flag,
                                     const char* tag_in,
                                     FlagStatus_t startingStatus)
{
    if (tag_in) {
        size_t len = strnlen(tag_in, MAX_TAG_LEN - 1);
        memcpy(flag->TAG, tag_in, len);
        flag->TAG[len] = '\0';
    } else {
        flag->TAG[0] = '\0';
    }

    atomic_store(&flag->atomicFlag, startingStatus);
}

ATOMIC_FLAG_API void AtomicFlag_Clear(AtomicFlag_t* flag){
    atomic_store(&flag->atomicFlag, FLAG_CLEAR);
}

ATOMIC_FLAG_API void AtomicFlag_Set(AtomicFlag_t* flag){
    atomic_store(&flag->atomicFlag, FLAG_SET);
}

ATOMIC_FLAG_API FlagStatus_t AtomicFlag_GetStatus(AtomicFlag_t* flag){
    return (FlagStatus_t)atomic_load(&flag->atomicFlag);
}

#endif // BARRIER_H
