#include "../../config.h"
#ifndef OFF_SCREEN_RENDERING
#    include <sched.h>
#    include <time.h>
#    include <unistd.h>
#    define GL_SILENCE_DEPRECATION
#    include "../../../mpsLibC/common/Assert_Common.h"
#    include "../../Transform/inc/Transform.h"
#    include "../inc/Renderer.h"
#    include "AtomicFlag.h"
#    include "LFfifo.h"
#    include "Renderer_ColorFrame.h"
#    include <GLFW/glfw3.h>
#    include <pthread.h>

static LF_Fifo_t* rawFrameInFifo = NULL;

// -----------------------------------------------------------------------------
// Grid
// -----------------------------------------------------------------------------
static void draw_grid(Render_Frame_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    int nx = pFrame->nx;
    int ny = pFrame->ny;

    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);

    for (int i = 0; i <= nx; i++) {
        float x = -1.0f + (2.0f * i / nx);
        glVertex2f(x, -1.0f);
        glVertex2f(x, 1.0f);
    }

    for (int j = 0; j <= ny; j++) {
        float y = -1.0f + (2.0f * j / ny);
        glVertex2f(-1.0f, y);
        glVertex2f(1.0f, y);
    }

    glEnd();
}

// -----------------------------------------------------------------------------
// Velocity Arrows
// -----------------------------------------------------------------------------
static void draw_velocities(Render_Frame_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->ux && pFrame->uy);

    int nx = pFrame->nx;
    int ny = pFrame->ny;

    float arrow_scale = 1.0f; // increased for visibility

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINES);

    // ---- Ux ----
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int idx = j * nx + i; // row-major again

            float ux = (float)pFrame->ux[idx];

            float x = -1.0f + (2.0f * i / nx);
            float y = -1.0f + (2.0f * (j + 0.5f) / ny);

            glVertex2f(x, y);
            glVertex2f(x + ux * arrow_scale, y);
        }
    }

    // ---- Uy ----
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int idx = j * nx + i;

            float uy = (float)pFrame->uy[idx];

            float x = -1.0f + (2.0f * (i + 0.5f) / nx);
            float y = -1.0f + (2.0f * j / ny);

            LOG("Sample velocity: ux=%.2f uy=%.2f", pFrame->ux[idx], pFrame->uy[idx]);

            glVertex2f(x, y);
            glVertex2f(x, y + uy * arrow_scale);
        }
    }

    glEnd();
    glLineWidth(1.0f);
}

// -----------------------------------------------------------------------------
// Pressure Field
// -----------------------------------------------------------------------------
static void draw_pressure(Render_Frame_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->pressure);

    int nx = pFrame->nx;
    int ny = pFrame->ny;

    double pMin = pFrame->pressure[0];
    double pMax = pMin;

    /* --- Compute min/max, ignoring exact zero if you want solid walls to be black --- */
    for (int i = 1; i < nx * ny; i++) {
        double p = pFrame->pressure[i];
        if (p < pMin) pMin = p;
        if (p > pMax) pMax = p;
    }

    double pRange = (pMax - pMin) < 1e-10 ? 1.0 : (pMax - pMin);

    glBegin(GL_QUADS);

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {

            int idx = j * nx + i;
            double p = pFrame->pressure[idx];

            float r, g, b;

            if (p == 0.0) {
                /* --- ZERO PRESSURE → BLACK --- */
                r = g = b = 0.0f;

            } else {
                /* --- NORMALIZED COLOR MAP (red to blue) --- */
                double t = (p - pMin) / pRange;
                if (t < 0) t = 0;
                if (t > 1) t = 1;

                r = (float)t;
                g = 0.0f;
                b = 1.0f - (float)t;
            }

            glColor3f(r, g, b);

            float x0 = -1.0f + (2.0f * i / nx);
            float x1 = -1.0f + (2.0f * (i + 1) / nx);
            float y0 = -1.0f + (2.0f * j / ny);
            float y1 = -1.0f + (2.0f * (j + 1) / ny);

            glVertex2f(x0, y0);
            glVertex2f(x1, y0);
            glVertex2f(x1, y1);
            glVertex2f(x0, y1);
        }
    }

    glEnd();
}

// -----------------------------------------------------------------------------
// PUBLIC DRAW ENTRYPOINT (WITH IFDEF SWITCH)
// -----------------------------------------------------------------------------

render_err_t Render_RawFrame_Process(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    Render_Frame_t* pFrame = NULL;

    while (LF_Fifo_SpinPop(rawFrameInFifo, &pFrame) == LF_FIFO_FAIL_TRY_POP) {
        sched_yield();
    }

    ASSERT_COMMON_NOT_NULL(pFrame);
    draw_pressure(pFrame);
    draw_velocities(pFrame);
    draw_grid(pFrame);
    TransForm_RawFrameYeild(pFrame);
    // LOG("One server run");
    return RENDER_SUCCESS;
}

render_err_t Render_Send_Raw_Frame(Render_Frame_t* pFrameIn) {
    ASSERT_COMMON_NOT_NULL(pFrameIn);
    // LOG("Rxed a raw frame");
    // if (!pFrameIn) return RENDER_FAIL;
    // ASSERT_COMMON_POSIX(LF_Fifo_SpinPush(frameInFifo, pFrameIn), "Fialed to push from into
    // fifo");
    while (LF_Fifo_TryPush(rawFrameInFifo, pFrameIn) == LF_FIFO_FAIL_TRY_PUSH) {
        sched_yield();
    } // gCurrentFrame = pFrameIn;
    return RENDER_SUCCESS;
}

render_err_t Render_RawFrameProcessing_Init(void) {
    ASSERT_COMMON_POSIX(LF_Fifo_Init(&rawFrameInFifo, FRAME_IN_FIFO_SIZE),
                        "Failed to init the frame fifo");
    return RENDER_SUCCESS;
}

render_err_t Render_RawFramesProcessing_Dtr(void) {
    ASSERT_COMMON_POSIX(LF_Fifo_Dtr(rawFrameInFifo), "Fialed ot dtr fifo");
    return RENDER_SUCCESS;
}

#endif
