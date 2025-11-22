/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "../inc/Transform.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include "LFfifo.h"
#include <pthread.h>

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

#define TIME_TO_DIE (FLAG_SET)
#define KEEP_BREATHING_BUDDY (FLAG_CLEAR)

AtomicFlag_t killFlag;

#define SIM_SNAP_FIFO_CAPACITY ((1 << 10) - 1)
LF_Fifo_t* pSimSnapFifo = NULL;

pthread_t TransformService_th;

struct timespec timeOut = {.tv_nsec = 0, .tv_sec = 1};

/* ================================================== */
/*                 MACRO FUNC DEFINITIONS             */
/* ================================================== */

#define KILL_FLAG_CHECK(flag)                                                                      \
    do {                                                                                           \
        if (AtomicFlag_GetStatus(&(flag)) == TIME_TO_DIE) {                                        \
            return NULL;                                                                           \
        }                                                                                          \
    } while (0)

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

static transform_err_t GetRenderBuf(Render_Frame_t** pRenderOut, uint64_t cells) {
    // Validate inputs
    if (!pRenderOut || cells == 0) {
        return TRANSFORM_ERR_SYSTEM;
    }

    *pRenderOut = NULL;

    // Overflow guard (cells * sizeof(T))
    if (cells > (UINT64_MAX / sizeof(pressure_t)) || cells > (UINT64_MAX / sizeof(velocity_t))) {
        return TRANSFORM_ERR_SYSTEM;
    }

    Render_Frame_t* pBuf = (Render_Frame_t*)calloc(1, sizeof(Render_Frame_t));
    if (!pBuf) {
        return TRANSFORM_ERR_SYSTEM;
    }

    // Allocate pressure field
    pBuf->pressure = (pressure_t*)malloc(sizeof(pressure_t) * cells);
    if (!pBuf->pressure) {
        goto fail;
    }

    // Allocate X velocity field
    pBuf->ux = (velocity_t*)malloc(sizeof(velocity_t) * cells);
    if (!pBuf->ux) {
        goto fail;
    }

    // Allocate Y velocity field
    pBuf->uy = (velocity_t*)malloc(sizeof(velocity_t) * cells);
    if (!pBuf->uy) {
        goto fail;
    }

    *pRenderOut = pBuf;
    return TRANSFORM_SUCCESS;

fail:
    // free safely on partial construction
    free(pBuf->pressure);
    free(pBuf->ux);
    free(pBuf->uy);
    free(pBuf);
    return TRANSFORM_ERR_SYSTEM;
}

static transform_err_t ConvertSnapToRenderFrame(SimSnap_t* pSnap, Render_Frame_t* pRenderFrame) {
    ASSERT_COMMON(pSnap, "Got a NULL pSnap");
    ASSERT_COMMON(pRenderFrame, "Got a NULL pRenderFrame");
    pRenderFrame->nx = pSnap->nx;
    pRenderFrame->ny = pSnap->ny;

    FOR_LOOP_COMMON(i, pSnap->nx) {
        FOR_LOOP_COMMON(j, pSnap->ny) {
            pRenderFrame->ux[pSnap->nx * i + pSnap->ny] = pSnap->cells[i][j].ux;
            pRenderFrame->uy[pSnap->nx * i + pSnap->ny] = pSnap->cells[i][j].uy;
            pRenderFrame->pressure[pSnap->nx * i + pSnap->ny] = pSnap->cells[i][j].p;
        }
    }
    return TRANSFORM_SUCCESS;
}

static void* Task_TransformService(void* pvArgs) {
    (void)pvArgs;

    LOG("Task_TransformService Started Up");
    while (1) {

        KILL_FLAG_CHECK(killFlag);

        SimSnap_t* pSimSnap = NULL;
        Render_Frame_t* pRenderFrame = NULL;
        err_LF_Fifo_t r = LF_Fifo_TimedPop(pSimSnapFifo, &pSimSnap, &timeOut);

        if (r == LF_FIFO_FAIL_TIMED_POP) {
            continue;
        }

        // TODO: convert to Render_Frame_t
        // ConvertSnapToOpenGL(pSimSnap, ...);
        // TODO: send to renderer
        ASSERT_COMMON_POSIX(GetRenderBuf(&pRenderFrame, pSimSnap->nx * pSimSnap->ny),
                            "Failed to get render buf");
        ASSERT_COMMON_POSIX(ConvertSnapToRenderFrame(pSimSnap, pRenderFrame), "Faield to convert");
        ASSERT_COMMON_POSIX(Render_Send_Frame(pRenderFrame), "Fialed to send to rednered serive");
    }

    return NULL;
}

transform_err_t Transform_Init(void) {

    AtomicFlag_UpdateStatus(&killFlag, KEEP_BREATHING_BUDDY);

    ASSERT_COMMON_POSIX(LF_Fifo_Init(&pSimSnapFifo, SIM_SNAP_FIFO_CAPACITY),
                        "Failed to init the transform service snapshot FIFO");

    ASSERT_COMMON_POSIX(pthread_create(&TransformService_th, NULL, Task_TransformService, NULL),
                        "Failed to launch the Transform service thread");

    return TRANSFORM_SUCCESS;
}

transform_err_t Transform_Dtr(void) {

    AtomicFlag_UpdateStatus(&killFlag, TIME_TO_DIE);

    ASSERT_COMMON_POSIX(pthread_join(TransformService_th, NULL),
                        "Failed to join Transform service thread");

    ASSERT_COMMON_POSIX(LF_Fifo_Dtr(pSimSnapFifo), "Failed to destroy FIFO");

    return TRANSFORM_SUCCESS;
}

transform_err_t Transform_SendNewSimState(SimSnap_t* pSimState) {
    err_LF_Fifo_t r = LF_Fifo_TryPush(pSimSnapFifo, pSimState);
    return (r == LF_FIFO_SUCCESS) ? TRANSFORM_SUCCESS : TRANSFORM_ERR_SEND_FRAME;
}
