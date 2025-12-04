// Renderer_OffScreen.c
#include "Renderer_OffScreen.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "LOG.h"
#include "unistd.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

#ifdef DISPLAY_COLORS
#    include "Renderer_ColorFrame.h"
#else
#    include "Renderer_RawFrames.h"
#endif

static AtomicFlag_t killFlag;
static pthread_t renderer_main_th;

// Forward declaration
static void Render_Draw(void);

// ----------------------------------------
// Frame drawing
// ----------------------------------------
static void Render_Draw(void) {
#ifdef DISPLAY_COLORS
    Render_ColorFrame_Process();
#else
    Render_RawFrame_Process();
#endif
}

// ----------------------------------------
// Simplified render loop - no OpenGL overhead
// ----------------------------------------
static void* Task_OffScreen_Buffering(void* pvArgs) {
    LOG("Offscreen renderer thread started (no GL, direct PNG writing)");

    // Initialize the color frame processing (just FIFO setup now)
#ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Init();
#else
    Render_RawFrameProcessing_Init();
#endif

    LOG("Render loop started - processing frames as they arrive");

    // Simple loop - just process frames as fast as they come in
    while (AtomicFlag_GetStatus(&killFlag) != FLAG_SET) {
        Render_Draw();

        // Optional: small sleep to prevent busy-waiting if no frames
        // Remove this if you want maximum throughput
        // usleep(100);
    }

    // Cleanup
#ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Dtr();
#else
    Render_RawFramesProcessing_Dtr();
#endif

    LOG("Render thread exiting");
    return NULL;
}

// ----------------------------------------
// Public API
// ----------------------------------------
render_err_t OffScreenRender_Init(void) {
    LOG("Renderer starting up (simplified, no GL)");
    AtomicFlag_Clear(&killFlag);

    ASSERT_COMMON_POSIX(pthread_create(&renderer_main_th, NULL, Task_OffScreen_Buffering, NULL),
                        "Failed to start renderer thread");

    LOG("Renderer init success");
    return RENDER_SUCCESS;
}

render_err_t OffScreenRender_Dtr(void) {
    LOG("Shutting down renderer");
    AtomicFlag_Set(&killFlag);
    pthread_join(renderer_main_th, NULL);

    LOG("Renderer destruction success");
    return RENDER_SUCCESS;
}
