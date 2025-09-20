#ifndef SAFEMEM_H
#define SAFEMEM_H
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdlib.h>
#include <stddef.h>
#include <Assert_Common.h>

/* ================================================== */
/*                      DEFINES                       */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION PROTOTYPES                */
/* ================================================== */

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */

#ifndef NDEBUG
#define ERR_MALLOC_CUSTOM(ptr, bytes, allocFn, errMsg, errRet)  \
    do {                                                        \
        (ptr) = (void*) allocFn((bytes));                               \
        if ((ptr) == NULL) {                                    \
            LOG("XMALLOC FAILED: %s", (errMsg));                \
            return (errRet);                                    \
        }                                                       \
    } while (0)
#else
#define ERR_MALLOC_CUSTOM(ptr, bytes, allocFn, errMsg, errRet)  \
    do {                                                        \
        (ptr) = (void*) allocFn((bytes));                        \
        if ((ptr) == NULL) {                                    \
            /* Optional LOG("malloc failed"); */                \
            return (errRet);                                    \
        }                                                       \
    } while (0)
#endif

#ifndef NDEBUG
#define ERR_MALLOC(ptr, bytes, errMsg, errRet)                  \
    do {                                                        \
        (ptr) = xMalloc((bytes));                               \
        if ((ptr) == NULL) {                                    \
            LOG("XMALLOC FAILED: %s", (errMsg));                \
            return (errRet);                                    \
        }                                                       \
    } while (0)
#else
#define ERR_MALLOC(ptr, bytes, errMsg, errRet)                  \
    do {                                                        \
        (ptr) = (void*) malloc((bytes));                        \
        if ((ptr) == NULL) {                                    \
            /* Optional LOG("malloc failed"); */                \
            return (errRet);                                    \
        }                                                       \
    } while (0)
#endif

#ifndef NDEBUG
#define ERR_REALLOC(ptr, bytes, errMsg, errRet)                     \
    do {                                                            \
        void* _ptrBuf = xRealloc((ptr), (bytes));                   \
        if (_ptrBuf == NULL) {                                      \
            LOG("XREALLOC FAILED: %s", (errMsg));                   \
            return (errRet);                                        \
        } else {                                                    \
            (ptr) = _ptrBuf;                                        \
        }                                                           \
    } while (0)
#else
#define ERR_REALLOC(ptr, bytes, errMsg, errRet)                     \
    do {                                                            \
        (ptr) = realloc((ptr), (bytes));                            \
        if ((ptr) == NULL) {                                        \
            return (errRet);                                        \
        }                                                           \
    } while (0)
#endif

#ifndef NDEBUG
#define ERR_FREE(ptr, errMsg, errRet)                 \
    do {                                              \
        if (xFree(ptr)) {                             \
            LOG("Free Failed: %s", errMsg);           \
            return (errRet);                          \
        }                                             \
    } while (0)
#else
#define ERR_FREE(ptr, errMsg, errRet)                 \
    do {                                              \
        if (xFree(ptr)) {                             \
            return (errRet);                          \
        }                                             \
    } while (0)
#endif
/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
static inline void* xMalloc(size_t bytes){
    void* ptr = malloc(bytes);
    ASSERT_COMMON(ptr != NULL, "Malloc Failed", NULL);
    return ptr;
}

static inline void* xRealloc(void* ptr, size_t bytes){
    void* ptrBuf = realloc(ptr, bytes);
    ASSERT_COMMON(ptrBuf != NULL, "Realloc Failed", NULL);
    return ptrBuf; 
}

static inline int xFree(void* ptr){
    ASSERT_COMMON(ptr != NULL, "Free Failed", EXIT_FAILURE);
    free(ptr);
    return EXIT_SUCCESS;
}


#endif 

