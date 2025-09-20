#ifndef BUMP_H
#define BUMP_H

#include <stddef.h>
#include <stdlib.h>
#include "../../common/Assert_Common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#define ERR_MSG_BUF_SIZE (64)
#define BUMP_API static inline

typedef enum {
    BUMP_SUCCESS = 0,
    BUMP_FAIL,
    BUMP_FAIL_INIT,
    BUMP_FAIL_ALLOC,
    BUMP_FAIL_RESET,
    BUMP_FAIL_FREE,
}BumpAllocator_err;

typedef struct BumpAllocator{
    uint8_t* memPool;
    size_t poolSize;
    uint8_t* currPoolPtr;
    size_t currBytesAllocated;
    char errBuf[ERR_MSG_BUF_SIZE];
} BumpAllocator_t;

BUMP_API BumpAllocator_t* Bump_Init(size_t bytes){
    BumpAllocator_t* arena = (BumpAllocator_t*)malloc(sizeof(BumpAllocator_t));
    if(!arena){ return NULL; }

    uint8_t* poolBuf= (uint8_t*)malloc(bytes);

    if(!poolBuf){
        free(arena);
        return NULL;
    }

    arena->memPool = poolBuf;
    arena->poolSize = bytes;
    arena->currPoolPtr = poolBuf;
    arena->currBytesAllocated = 0;
    return arena;
}

BUMP_API void* Bump_Alloc(BumpAllocator_t* arena, size_t bytes){
    ASSERT_COMMON(arena != NULL, "Null Arena", NULL);

    //out of mem check
    if(arena->currBytesAllocated + bytes >= arena->poolSize){
        snprintf(arena->errBuf, sizeof(arena->errBuf), 
                "Out of memory: requested %zu, available %zu", 
                bytes, arena->poolSize - arena->currBytesAllocated);
        return NULL;
    }

    uint8_t* ptrOut = arena->currPoolPtr;
    arena->currPoolPtr+=bytes;
    arena->currBytesAllocated+=bytes;
    arena->errBuf[0] = '\0';
    return ptrOut;
}

BUMP_API size_t Bump_GetArenaSize(BumpAllocator_t* arena){
    ASSERT_COMMON(arena != NULL, "Null Arena", BUMP_FAIL);
    return arena->poolSize;    
}

BUMP_API size_t Bump_GetBytesInUse(BumpAllocator_t* arena){
    ASSERT_COMMON(arena != NULL, "Null Arena", BUMP_FAIL);
    return arena->currBytesAllocated;
}

BUMP_API size_t Bump_GetBytesRemaining(BumpAllocator_t* arena){
    ASSERT_COMMON(arena != NULL, "Null Arena", BUMP_FAIL);
    return arena->poolSize - arena->currBytesAllocated;
}

BUMP_API BumpAllocator_err Bump_Reset(BumpAllocator_t* arena){
    ASSERT_COMMON(arena != NULL, "Null Arena", BUMP_FAIL_RESET);
    arena->currBytesAllocated = 0;
    arena->currPoolPtr = arena->memPool;
    return BUMP_SUCCESS;
}

BUMP_API BumpAllocator_err Bump_Free(BumpAllocator_t* arena){
    ASSERT_COMMON(arena != NULL, "Null Arena", BUMP_FAIL_FREE);
    free(arena->memPool);
    free(arena);
    return BUMP_SUCCESS;
}

BUMP_API char* Bump_GetErrMsg(BumpAllocator_t* arena){
    return arena->errBuf;
}

#ifdef __cplusplus
}
#endif

#endif
