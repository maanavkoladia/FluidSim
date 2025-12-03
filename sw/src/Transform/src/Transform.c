/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "../inc/Transform.h"
#include "../../config.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "HelperUtils.h"
#include "LFfifo.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#ifdef DISPLAY_COLORS
#    include "FrameColors_Utils.h"
#else
#    include "FrameRaw_Utils.h"
#endif

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

#define TIME_TO_DIE (FLAG_SET)
#define KEEP_BREATHING_BUDDY (FLAG_CLEAR)
#define TEARDOWN_FIFO_FLUSH_FAIL_COUNT (5)

static AtomicFlag_t killFlag;

#define SIM_SNAP_FIFO_CAPACITY ((1 << 12) - 1)
static LF_Fifo_t* pSimSnapFifo = NULL;

static pthread_t TransformService_th;

static struct timespec timeOut = {.tv_nsec = 0, .tv_sec = 1};

/* ================================================== */
/*                 MACRO FUNC DEFINITIONS             */
/* ================================================== */

#define KILL_FLAG_CHECK(flag)                                                                      \
    do {                                                                                           \
    } while (0)

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

static void SimSnap_FifoFlush(void) {
    SimSnap_t* pSimSnap = NULL;
    uint64_t popFailCount = 0;
    while (popFailCount <= TEARDOWN_FIFO_FLUSH_FAIL_COUNT) {
        if (LF_Fifo_TryPop(pSimSnapFifo, &pSimSnap) == LF_FIFO_SUCCESS) {
            Sim_SimSnap_Yeild(pSimSnap);
        } else { // fialed the pop
            popFailCount++;
        }
    }
    LOG("Done flushing the snap fifo");
}

static transform_err_t Task_TransformService_TearDown(void) {
    // flush the fifo
    SimSnap_FifoFlush();
    // all Sim Engine related task done now, signal it
    Transform_SimEngine_Signal_TeardownComplete();

    return TRANSFORM_SUCCESS;
}

static void* Task_TransformService(void* pvArgs) {
    (void)pvArgs;
    LOG("Task_TransformService Started Up");
    while (1) {
        if (AtomicFlag_GetStatus(&killFlag) == TIME_TO_DIE) {
            ASSERT_COMMON_POSIX(Task_TransformService_TearDown(), "Failed Tear Down");
            LOG("Transform TearDown Success");
            return NULL;
        }

        SimSnap_t* pSimSnap = NULL;
        err_LF_Fifo_t r = LF_Fifo_TimedPop(pSimSnapFifo, &pSimSnap, &timeOut);
        if (r == LF_FIFO_FAIL_TIMED_POP) {
            continue;
        }

#ifdef DISPLAY_COLORS
        // Ensure the render window is divisible by the simulation grid
        ASSERT_COMMON((RENDER_WINDOW_HEIGHT % pSimSnap->ny) == 0,
                      "RENDER_WINDOW_HEIGHT must be divisible by simulation ny: Val of op is %lu, "
                      "RENDER_WINDOW_HEIGHT = %u  SimSnap.ny = %lu",
                      RENDER_WINDOW_HEIGHT % pSimSnap->ny, RENDER_WINDOW_HEIGHT, pSimSnap->ny);

        ASSERT_COMMON(RENDER_WINDOW_WIDTH % pSimSnap->nx == 0,
                      "RENDER_WINDOW_WIDTH must be divisible by simulation nx");

        // Ensure square scaling factor (pixels per cell)
        ASSERT_COMMON((RENDER_WINDOW_HEIGHT / pSimSnap->ny) == (RENDER_WINDOW_WIDTH / pSimSnap->nx),
                      "Scaling factor mismatch: pixels per cell must be square");

        Render_Frame_Colors_t* pRenderFrame = NULL;
        ASSERT_COMMON_POSIX(
            Init_ColorFrame(&pRenderFrame, RENDER_WINDOW_WIDTH, RENDER_WINDOW_HEIGHT),
            "Failed to get render buf");
        ASSERT_COMMON_POSIX(Snap2ColorFrame(pSimSnap, pRenderFrame), "Faield to convert");
        ASSERT_COMMON_POSIX(Render_Send_Frame_Colors(pRenderFrame),
                            "Fialed to send to rednered serive");
        // ASSERT_COMMON_POSIX(TransForm_ColorFrameYeild(pRenderFrame), "Fialed to free render
        // frame");
        ASSERT_COMMON_POSIX(Sim_SimSnap_Yeild(pSimSnap), "Aint no way");

#else
        Render_Frame_t* pRenderFrame = NULL;
        ASSERT_COMMON_POSIX(Init_RawFrame(&pRenderFrame, pSimSnap->nx * pSimSnap->ny),
                            "Failed to get render buf");
        ASSERT_COMMON_POSIX(Snap2RawFrame(pSimSnap, pRenderFrame), "Faield to convert");
        ASSERT_COMMON_POSIX(Render_Send_Raw_Frame(pRenderFrame),
                            "Fialed to send to rednered serive");
        // ASSERT_COMMON_POSIX(TransForm_RawFrameYeild(pRenderFrame), "Fialed to free render
        // frame");
        ASSERT_COMMON_POSIX(Sim_SimSnap_Yeild(pSimSnap), "Aint no way");
#endif
    }

    return NULL;
}

/* ================================================== */
/*                 PUBLIC API                         */
/* ================================================== */

transform_err_t Transform_Init(void) {

    AtomicFlag_UpdateStatus(&killFlag, KEEP_BREATHING_BUDDY);

    ASSERT_COMMON_POSIX(LF_Fifo_Init(&pSimSnapFifo, SIM_SNAP_FIFO_CAPACITY),
                        "Failed to init the transform service snapshot FIFO");

    ASSERT_COMMON_POSIX(pthread_create(&TransformService_th, NULL, Task_TransformService, NULL),
                        "Failed to launch the Transform service thread");
    LOG("Tranform Service Init Probably Success");
    return TRANSFORM_SUCCESS;
}

transform_err_t Transform_Dtr(void) {

    AtomicFlag_UpdateStatus(&killFlag, TIME_TO_DIE);

    ASSERT_COMMON_POSIX(pthread_join(TransformService_th, NULL),
                        "Failed to join Transform service thread");

    ASSERT_COMMON_POSIX(LF_Fifo_Dtr(pSimSnapFifo), "Failed to destroy FIFO");

    LOG("Tranform Service Dtr Probably Success");
    return TRANSFORM_SUCCESS;
}

transform_err_t Transform_SendNewSimSnap(SimSnap_t* pSimSnap) {
    err_LF_Fifo_t r = LF_Fifo_TryPush(pSimSnapFifo, pSimSnap);
    return (r == LF_FIFO_SUCCESS) ? TRANSFORM_SUCCESS : TRANSFORM_ERR_SEND_FRAME;
}

void Transform_YeildRender_Frame_Colors(Render_Frame_Colors_t* pFrame) {
    if (pFrame) {
        TransForm_ColorFrameYeild(pFrame);
    }
}
