#define GL_SILENCE_DEPRECATION
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include "inc/Renderer.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SIM_RUNTIME_S (30)
#define NX 480
#define NY 480

velocity_t ux_g = -0.03;
velocity_t uy_g = -0.03;
// velocity_t growth = .01;

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
            frame->ux[idx] = ux_g;                      // horizontal gradient
            frame->uy[idx] = uy_g;                      // vertical gradient
            frame->pressure[idx] = (pressure_t)(i + j); // sum, easy to visualize
        }
    }
    // ux_g += growth;
    // uy_g += growth;
}

int Init_ColorFrame(Render_Frame_Colors_t** pFrameOut, uint64_t w, uint64_t h) {
    ASSERT_COMMON_NOT_NULL(pFrameOut);
    ASSERT_COMMON(w == RENDER_WINDOW_WIDTH, "Got invalid Color Fram Height");
    ASSERT_COMMON(h == RENDER_WINDOW_HEIGHT, "Got invalid Color Fram Width");
    Render_Frame_Colors_t* pFrameBuf =
        (Render_Frame_Colors_t*)malloc(sizeof(Render_Frame_Colors_t));

    pFrameBuf->height = h;
    pFrameBuf->width = w;
    pFrameBuf->colors = (Color_t*)malloc(sizeof(Color_t) * w * h);
#ifndef NDEBUG
    ASSERT_COMMON_ALLOC(pFrameBuf->colors);
#else
    if (!pFrameBuf->colors) {
        return TRANSFORM_ERR_SYSTEM;
    }
#endif
    *pFrameOut = pFrameBuf;
    return EXIT_SUCCESS;
}

void TransForm_ColorFrameYeild(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    ASSERT_COMMON_NOT_NULL(pFrame->colors);
    free(pFrame->colors);
    free(pFrame);
}

void ScrollColorsLeft(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    ASSERT_COMMON_NOT_NULL(pFrame->colors);

    uint64_t width = pFrame->width;
    uint64_t height = pFrame->height;

    // Temporary array to hold the first column
    Color_t* firstCol = (Color_t*)malloc(sizeof(Color_t) * height);
    if (!firstCol) return;

    // Copy the first column
    FOR_LOOP_COMMON(j, height) {
        firstCol[j] = pFrame->colors[j * width + 0];
    }

    // Shift all columns left
    FOR_LOOP_COMMON(i, width - 1) {
        FOR_LOOP_COMMON(j, height) {
            pFrame->colors[j * width + i] = pFrame->colors[j * width + (i + 1)];
        }
    }

    // Wrap the first column to the last
    FOR_LOOP_COMMON(j, height) {
        pFrame->colors[j * width + (width - 1)] = firstCol[j];
    }

    free(firstCol);
}
void CreateFakeFirstColorFrame(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    ASSERT_COMMON_NOT_NULL(pFrame->colors);

    uint64_t width = pFrame->width;
    uint64_t height = pFrame->height;

    FOR_LOOP_COMMON(i, width) {
        FOR_LOOP_COMMON(j, height) {
            float tX = (float)i / (float)(width - 1);  // normalized 0..1
            float tY = (float)j / (float)(height - 1); // normalized 0..1

            Color_t color;
            color.r = tX;        // red increases left → right
            color.g = tY;        // green increases bottom → top
            color.b = 1.0f - tX; // blue decreases left → right
            color.a = 1.0f;      // fully opaque

            pFrame->colors[j * width + i] = color;
        }
    }
}

Render_Frame_Colors_t* CopyColorFrame(Render_Frame_Colors_t* src) {
    if (!src) return NULL;

    Render_Frame_Colors_t* copy = malloc(sizeof(Render_Frame_Colors_t));
    if (!copy) return NULL;

    copy->width = src->width;
    copy->height = src->height;
    copy->colors = malloc(sizeof(Color_t) * src->width * src->height);

    if (!copy->colors) {
        free(copy);
        return NULL;
    }

    memcpy(copy->colors, src->colors, sizeof(Color_t) * src->width * src->height);
    return copy;
}

void ColorTestLoop(void) {
    Render_Frame_Colors_t* frame = NULL;

    // Allocate frame ONCE before loop
    ASSERT_COMMON_POSIX(Init_ColorFrame(&frame, NX, NY), "Failed to init frame");
    CreateFakeFirstColorFrame(frame);

    for (int frameNum = 0;; frameNum++) {
        if (frameNum > 0) {
            ScrollColorsLeft(frame);
        }

        // **COPY** the frame before sending
        Render_Frame_Colors_t* frameCopy = CopyColorFrame(frame);
        if (!frameCopy) {
            LOG("Failed to copy frame %d", frameNum);
            break;
        }

        Render_Send_Frame_Colors(frameCopy);
        // frameCopy is now owned by renderer

        // usleep(50000);
    }

    // Clean up the master frame
    TransForm_ColorFrameYeild(frame);
}

void RawTestLoop(void) {
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
        // sleep(1);
        usleep(50000);
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

    ColorTestLoop();

    if (Render_Dtr() != 0) {
        LOG("Failed to destroy renderer");
    }

    LOG("Program Exited");
    return EXIT_SUCCESS;
}
