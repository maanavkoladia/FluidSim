#include "FrameRaw_Utils.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include <pthread.h>
#include <stdint.h>

/* ================================================== */
/*                      RENDER BUF API                */
/* ================================================== */
transform_err_t Init_RawFrame(Render_Frame_t** pFrameOut, uint64_t cells) {
    // Validate inputs
    if (!pFrameOut || cells == 0) {
        return TRANSFORM_ERR_SYSTEM;
    }

    *pFrameOut = NULL;

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

    *pFrameOut = pBuf;
    return TRANSFORM_SUCCESS;

fail:
    // free safely on partial construction
    free(pBuf->pressure);
    free(pBuf->ux);
    free(pBuf->uy);
    free(pBuf);
    return TRANSFORM_ERR_SYSTEM;
}

transform_err_t TransForm_RawFrameYeild(Render_Frame_t* pFrame) {
    ASSERT_COMMON(pFrame && pFrame->pressure && pFrame->ux && pFrame->uy, "Got NULL ptr");
    free(pFrame->pressure);
    free(pFrame->ux);
    free(pFrame->uy);
    free(pFrame);
    return TRANSFORM_SUCCESS;
}

transform_err_t Snap2RawFrame(SimSnap_t* pSnap, Render_Frame_t* pFrame) {
    ASSERT_COMMON(pSnap, "Got a NULL pSnap");
    ASSERT_COMMON(pFrame, "Got a NULL pRenderFrame");
    pFrame->nx = pSnap->nx;
    pFrame->ny = pSnap->ny;

    uint64_t nx = pSnap->nx;
    uint64_t ny = pSnap->ny;

    FOR_LOOP_COMMON(i, nx) {
        FOR_LOOP_COMMON(j, ny) {
            uint64_t idx = i * ny + j; // correct row-major index
            pFrame->ux[idx] = pSnap->cells[i][j].ux;
            pFrame->uy[idx] = pSnap->cells[i][j].uy;
            pFrame->pressure[idx] = pSnap->cells[i][j].p;
        }
    }
    return TRANSFORM_SUCCESS;
}
