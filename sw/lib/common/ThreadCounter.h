#ifndef THREAD_COUNTER_H 
#define THREAD_COUNTER_H
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "Assert_Common.h"
#include "LOG.h"

/* ================================================== */
/*                    ENUMS & TYPES                   */
/* ================================================== */

#define MAX_TAG_LEN (64)

typedef enum {
    THREAD_COUNTER_SUCCESS = 0,
    THREAD_COUNTER_FAIL,
    THREAD_COUNTER_FAIL_SYSTEM,
    THREAD_COUNTER_FAIL_INIT,
    THREAD_COUNTER_FAIL_INIT_INVALID_PARAMS,
    THREAD_COUNTER_FAIL_NULLPTR,
}ThreadCounter_err_t;

typedef struct ThreadCounter_t{
    atomic_uint_fast64_t threadCount;
    uint threadCount_UpperBound;
    _Atomic(void*) cbData;
    void(*fn_UpperCount_CB)(void*);
    void*(*fn_Alloc)(size_t);
    void (*fn_Free)(void*);
    char TAG[MAX_TAG_LEN];
}ThreadCounter_t;

/* ================================================== */
/*                 PUBLIC FUNCTION DEFINITIONS        */
/* ================================================== */

static inline ThreadCounter_err_t ThreadCounter_Init(
        ThreadCounter_t** ppThreadCounter_Out, 
        unsigned int threadCount_UpperBound,
        void* (*fn_AllocIn)(size_t),
        void  (*fn_FreeIn)(void*),
        void  (*fn_CBIn)(void*),
        void* cbDataIn,
        const char* TAG)
{
    // Defensive: clear out param on entry
    if (ppThreadCounter_Out) { *ppThreadCounter_Out = NULL; }

    ASSERT_COMMON(ppThreadCounter_Out,
                  "Null CounterOutPtr",
                  THREAD_COUNTER_FAIL_NULLPTR);

    ASSERT_COMMON(threadCount_UpperBound != 0,
                  "Invalid thread count upper bound",
                  THREAD_COUNTER_FAIL_INIT_INVALID_PARAMS);

    // Either both custom alloc/free provided, or neither
    ASSERT_COMMON(
        (fn_AllocIn && fn_FreeIn) || (!fn_AllocIn && !fn_FreeIn),
        "Either both user alloc/free must be defined or neither",
        THREAD_COUNTER_FAIL_INIT_INVALID_PARAMS
    );

    void* (*fn_Alloc_Local)(size_t) = fn_AllocIn ? fn_AllocIn : malloc;
    void  (*fn_Free_Local)(void*)   = fn_FreeIn  ? fn_FreeIn  : free;

    // Allocate the structure
    ThreadCounter_t* pThreadCounter =
        (ThreadCounter_t*)fn_Alloc_Local(sizeof(ThreadCounter_t));
    if (!pThreadCounter) {
        LOG("Failed to allocate ThreadCounter structure");
        return THREAD_COUNTER_FAIL_SYSTEM;
    }

    // Initialize fields
    atomic_store_explicit(&pThreadCounter->threadCount, 0, memory_order_relaxed);
    pThreadCounter->threadCount_UpperBound = threadCount_UpperBound;
    pThreadCounter->fn_Alloc       = fn_AllocIn ? fn_AllocIn : malloc;
    pThreadCounter->fn_Free        = fn_FreeIn  ? fn_FreeIn  : free;
    pThreadCounter->fn_UpperCount_CB = fn_CBIn;
    pThreadCounter->cbData         = cbDataIn;

    // Safe tag copy (bounds + NUL)
#if MAX_TAG_LEN > 0
    if (TAG) {
        size_t n = strnlen(TAG, MAX_TAG_LEN - 1);
        memcpy(pThreadCounter->TAG, TAG, n);
        pThreadCounter->TAG[n] = '\0';
    } else {
        pThreadCounter->TAG[0] = '\0';
    }
#endif

    *ppThreadCounter_Out = pThreadCounter; 
    return THREAD_COUNTER_SUCCESS;

}
static inline ThreadCounter_err_t ThreadCounter_Dtr(ThreadCounter_t* pThreadCounter){
    ASSERT_COMMON(pThreadCounter, "Got a null threadCounter", THREAD_COUNTER_FAIL_NULLPTR);
    pThreadCounter->fn_Free(pThreadCounter);
    return THREAD_COUNTER_SUCCESS;
}

static inline ThreadCounter_err_t ThreadCounter_Reset(ThreadCounter_t* p){
    ASSERT_COMMON(p, "Got a null threadCounter", THREAD_COUNTER_FAIL_NULLPTR);
    atomic_store_explicit(&p->threadCount, 0, memory_order_relaxed);
    return THREAD_COUNTER_SUCCESS;
}

static inline ThreadCounter_err_t ThreadCounter_Increment(ThreadCounter_t* p){
    ASSERT_COMMON(p, "Got a null threadCount", THREAD_COUNTER_FAIL_NULLPTR);

    uint64_t prev = atomic_fetch_add_explicit(&p->threadCount, 1, memory_order_relaxed);

    if (prev + 1 == p->threadCount_UpperBound && p->fn_UpperCount_CB) {
        // Ensure we see the latest cbData published by the controller
        void* arg = atomic_load_explicit(&p->cbData, memory_order_acquire); LOG("ThreadCounter CB Triggered: %s", p->TAG);
        p->fn_UpperCount_CB(arg);
    }
    return THREAD_COUNTER_SUCCESS;
}

static inline ThreadCounter_err_t ThreadCounter_CBData_Update(ThreadCounter_t* p, void* newCBData){
    ASSERT_COMMON(p, "Null counter", THREAD_COUNTER_FAIL_NULLPTR);
    atomic_store_explicit(&p->cbData, newCBData, memory_order_release);
    return THREAD_COUNTER_SUCCESS;
}

/* ================================================== */
/*                 PRIVATE FUNCTION DEFINITIONS       */
/* ================================================== */

#endif
