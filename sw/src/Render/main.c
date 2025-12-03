#define GL_SILENCE_DEPRECATION
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include "inc/Renderer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SIM_RUNTIME_S 5
#define NX 50
#define NY 50

/* -------------------------------------------------
   Allocate a Render_Frame and initialize fields
------------------------------------------------- */
int Init_RawFrame(Render_Frame_t** pFrameOut, uint64_t totalCells) {
    if (!pFrameOut || totalCells == 0) return EXIT_FAILURE;

    *pFrameOut = NULL;

    Render_Frame_t* frame = (Render_Frame_t*)calloc(1, sizeof(Render_Frame_t));
    if (!frame) return EXIT_FAILURE;

    frame->pressure = (pressure_t*)calloc(totalCells, sizeof(pressure_t));
    frame->ux = (velocity_t*)calloc(totalCells, sizeof(velocity_t));
    frame->uy = (velocity_t*)calloc(totalCells, sizeof(velocity_t));

    if (!frame->pressure || !frame->ux || !frame->uy) {
        free(frame->pressure);
        free(frame->ux);
        free(frame->uy);
        free(frame);
        return EXIT_FAILURE;
    }

    *pFrameOut = frame;
    return EXIT_SUCCESS;
}

/* -------------------------------------------------
   Free a Render_Frame safely
------------------------------------------------- */
void Destroy_RawFrame(Render_Frame_t* frame) {
    if (!frame) return;
    free(frame->pressure);
    free(frame->ux);
    free(frame->uy);
    free(frame);
}
void TransForm_RawFrameYeild(Render_Frame_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    Destroy_RawFrame(pFrame);
}

void TransForm_ColorFrameYeild(Render_Frame_Colors_t* pFrame) {
    LOG("Not yet impleted ");
}
/* -------------------------------------------------
   Fill frame with deterministic "fake" values
   (use simple functions so it's easy to debug)
------------------------------------------------- */
void CreateFakeFrame(Render_Frame_t* frame) {
    ASSERT_COMMON_NOT_NULL(frame);

    int nx = frame->nx;
    int ny = frame->ny;

    FOR_LOOP_COMMON(i, nx) {
        FOR_LOOP_COMMON(j, ny) {
            int idx = j + i * ny;
            frame->ux[idx] = (velocity_t)(i);           // horizontal gradient
            frame->uy[idx] = (velocity_t)(j);           // vertical gradient
            frame->pressure[idx] = (pressure_t)(i + j); // sum, easy to visualize
        }
    }
}

/* -------------------------------------------------
   Main loop: generate and send frames to renderer
------------------------------------------------- */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    LOG("Fluid Sim Starting Up");

    /* Initialize renderer */
    if (Render_Init() != 0) {
        LOG("Failed to launch render service");
        return EXIT_FAILURE;
    }

    for (int frameNum = 0; frameNum < SIM_RUNTIME_S; frameNum++) {
        Render_Frame_t* frame = NULL;

        if (Init_RawFrame(&frame, NX * NY) != EXIT_SUCCESS) {
            LOG("Failed to allocate frame %d", frameNum);
            break;
        }

        frame->nx = NX;
        frame->ny = NY;

        CreateFakeFrame(frame);

        if (Render_Send_Frame(frame) != 0) {
            LOG("Failed to send frame %d", frameNum);
        } else {
            LOG("Sent frame %d successfully", frameNum);
        }

        // Destroy_RawFrame(frame);
        usleep(500 * 1000); // 0.5 sec delay per frame for debug
    }

    if (Render_Dtr() != 0) {
        LOG("Failed to destroy renderer");
    }

    LOG("Program Exited");
    return EXIT_SUCCESS;
}
