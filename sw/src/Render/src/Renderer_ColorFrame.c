#include <sched.h>
#include <time.h>
#include <unistd.h>
#define GL_SILENCE_DEPRECATION
#include "../../../mpsLibC/common/Assert_Common.h"
#include "../../Transform/inc/Transform.h"
#include "../inc/Renderer.h"
#include "AtomicFlag.h"
#include "LFfifo.h"
#include "Renderer_ColorFrame.h"
#include <GLFW/glfw3.h>
#include <pthread.h>

static LF_Fifo_t* pColorFrameInFifo = NULL;

// -----------------------------------------------------------------------------
// Color Frame
// -----------------------------------------------------------------------------

static void draw_frame_colors(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->colors);

    int width = pFrame->width;
    int height = pFrame->height;

    glBegin(GL_QUADS);

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {

            int idx = y * width + x;
            Color_t* c = &pFrame->colors[idx];
            glColor3f(c->r, c->g, c->b);

            float x0 = -1.0f + (2.0f * x / width);
            float x1 = -1.0f + (2.0f * (x + 1) / width);
            float y0 = -1.0f + (2.0f * y / height);
            float y1 = -1.0f + (2.0f * (y + 1) / height);

            glVertex2f(x0, y0);
            glVertex2f(x1, y0);
            glVertex2f(x1, y1);
            glVertex2f(x0, y1);
        }
    }

    glEnd();
}

render_err_t Render_ColorFrame_Process(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    Render_Frame_Colors_t* pFrame = NULL;

    while (LF_Fifo_SpinPop(pColorFrameInFifo, &pFrame) == LF_FIFO_FAIL_TRY_POP) {
        sched_yield();
    }

    ASSERT_COMMON_NOT_NULL(pFrame);
    draw_frame_colors(pFrame);
    TransForm_ColorFrameYeild(pFrame);
    // LOG("One server run");
    return RENDER_SUCCESS;
}

render_err_t Render_Send_Frame_Colors(Render_Frame_Colors_t* pFrameIn) {
    ASSERT_COMMON_NOT_NULL(pFrameIn && pFrameIn->colors);
    ASSERT_COMMON(pFrameIn->height == RENDER_WINDOW_HEIGHT, "Hieght deosnt match");
    ASSERT_COMMON(pFrameIn->width == RENDER_WINDOW_WIDTH, "width deosnt match");
    // if (!pFrameIn || !pFrameIn->colors) return RENDER_FAIL;
    // if (pFrameIn->width <= 0 || pFrameIn->height <= 0) return RENDER_FAIL;

    // if (gCurrentFrameColors) TransForm_ColorFrameYeild(gCurrentFrameColors);
    while (LF_Fifo_TryPush(pColorFrameInFifo, pFrameIn) == LF_FIFO_FAIL_TRY_PUSH) {
        sched_yield();
    } // gCurrentFrame = pFrameIn;
    return RENDER_SUCCESS;
}

render_err_t Render_ColorFramesProcessing_Init(void) {
    ASSERT_COMMON_POSIX(LF_Fifo_Init(&pColorFrameInFifo, FRAME_IN_FIFO_SIZE),
                        "Failed to init the frame fifo");
    return RENDER_SUCCESS;
}

render_err_t Render_ColorFramesProcessing_Dtr(void) {
    ASSERT_COMMON_POSIX(LF_Fifo_Dtr(pColorFrameInFifo), "Faield to DTR fifo wft");
    return RENDER_SUCCESS;
}
