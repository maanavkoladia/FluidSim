/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "../inc/Transform.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "LFfifo.h"
#include <pthread.h>

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

#define TIME_TO_DIE (FLAG_SET)
#define KEEP_BREATHING_BUDDY (FLAG_CLEAR)

AtomicFlag_t killFlag;

#define SIM_SNAP_FIFO_CAPACITY ((1 << 10) - 1)
LF_Fifo_t* pSimSnapFifo = NULL;

pthread_t TransformService_th;
struct timespec timeOut = {
    .tv_nsec = 0,
    .tv_sec = 1,
};

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */
#define KILL_FLAG_CHECK(flag)                                                                      \
    do {                                                                                           \
        if (AtomicFlag_GetStatus(&flag) == TIME_TO_DIE) {                                          \
            return NULL;                                                                           \
        }                                                                                          \
        while (0)

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

static transform_err_t ConvertSnapToOpenGL(SimSnap_t* pSnap, Render_Frame_t* pRenderFrame) {
    return TRANSFORM_SUCCES;
}

static void* Task_TransformService(void* pvArgs) {
    LOG("Task_TransformService Started Up");
    while (1) {
        KILL_FLAG_CHECK(killFlag);
        SimSnap_t* pSimSnap = NULL;
        err_LF_Fifo_t r = LF_Fifo_TimedPop(pSimSnapFifo, &pSimSnap, &timeOut);
        if (r == LF_FIFO_FAIL_TIMED_POP) {
            continue;
        }
        // no convert to andres's thing
        // then return the ptr back
        // then call render send routine
    }

    return NULL;
}

transform_err_t Transform_Init(void) {
    // init the fifo
    ASSERT_COMMON_POSIX(AtomicFlag_UpdateStatus(&killFlag, KEEP_BREATHING_BUDDY),
                        "fialde to init atomic flag");
    ASSERT_COMMON_POSIX(LF_Fifo_Init(&pSimSnapFifo, SIM_SNAP_FIFO_CAPACITY),
                        "Failed to init the transfown service snapshot fifo");

    ASSERT_COMMON_POSIX(pthread_create(&TransformService_th, NULL, Task_TransformService, NULL),
                        "Fialed ot launch the xform service thread");

    // init the thread
    return TRANSFORM_SUCCES;
}

transform_err_t Transform_Dtr(void) {
    AtomicFlag_UpdateStatus(&killFlag, TIME_TO_DIE);
    ASSERT_COMMON_POSIX(pthread_join(TransformService_th, NULL), "failed to koin thread");
    // free the fifo
    ASSERT_COMMON_POSIX(LF_Fifo_Dtr(pSimSnapFifo), "fialed to dtr fifo");
    return TRANSFORM_SUCCES;
}

transform_err_t Tranform_SendNewSimState(SimSnap_t* pSimState) {
    err_LF_Fifo_t r = LF_Fifo_TryPush(pSimSnapFifo, pSimState);
    return r == LF_FIFO_SUCCESS ? TRANSFORM_SUCCES : TRANSFORM_ERR_SEND_FRAME;
}
