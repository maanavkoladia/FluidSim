#include "../../config.h"

#ifndef ON_REMOTE
#    include "../../Render/inc/Renderer.h"
#    include "Assert_Common.h"
#    include "AtomicFlag.h"
#    include "ForLoop.h"
#    include "FrameColors_Utils.h"
#    include <math.h>
#    include <pthread.h>
#    include <stdint.h>

transform_err_t Init_ColorFrame(Render_Frame_Colors_t** pFrameOut, uint64_t w, uint64_t h) {
    ASSERT_COMMON_NOT_NULL(pFrameOut);
    ASSERT_COMMON(w == RENDER_WINDOW_WIDTH, "Got invalid Color Fram Height");
    ASSERT_COMMON(h == RENDER_WINDOW_HEIGHT, "Got invalid Color Fram Width");
    Render_Frame_Colors_t* pFrameBuf =
        (Render_Frame_Colors_t*)malloc(sizeof(Render_Frame_Colors_t));

    pFrameBuf->height = h;
    pFrameBuf->width = w;
    pFrameBuf->colors = (Color_t*)malloc(sizeof(Color_t) * w * h);
#    ifndef NDEBUG
    ASSERT_COMMON_ALLOC(pFrameBuf->colors);
#    else
    if (!pFrameBuf->colors) {
        return TRANSFORM_ERR_SYSTEM;
    }
#    endif
    *pFrameOut = pFrameBuf;
    return TRANSFORM_SUCCESS;
}

transform_err_t TransForm_ColorFrameYeild(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    ASSERT_COMMON_NOT_NULL(pFrame->colors);
    free(pFrame->colors);
    free(pFrame);
    return TRANSFORM_SUCCESS;
}

static inline int IsSolidCell(SimSnap_t* pSnap, uint64_t fx, uint64_t fy, uint64_t scalingFactor) {
    double simX = (double)fx / (double)scalingFactor;
    double simY = (double)fy / (double)scalingFactor;

    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    // Clamp just in case
    if (cx >= pSnap->nx) cx = pSnap->nx - 1;
    if (cy >= pSnap->ny) cy = pSnap->ny - 1;

    // Access flattened array: index = cy * nx + cx
    return pSnap->type[cy * pSnap->nx + cx] == SOLID;
}

static inline double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

// Try to parallelize in CUDA once you get access to TACC
static inline double InterpolateUX(SimSnap_t* pSnap, uint64_t fx, uint64_t fy,
                                   uint64_t scalingFactor) {
    // Convert pixel coordinates (fx, fy) into continuous simulation space.
    // Example: if scalingFactor = 5, then pixel 12 maps to simX = 12/5 = 2.4
    double simX = (double)fx / (double)scalingFactor;
    double simY = (double)fy / (double)scalingFactor;

    // Base simulation cell indices (top-left of the interpolation square).
    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    // Fractional part inside the cell: range is [0, 1).
    // These determine the interpolation weights.
    double fxFrac = simX - (double)cx;
    double fyFrac = simY - (double)cy;

    uint64_t nx = pSnap->nx;
    uint64_t ny = pSnap->ny;

    // Neighbor cell indices (right and bottom neighbors).
    // If at the edge, clamp to avoid out-of-bounds.
    uint64_t cx1 = (cx + 1 < nx) ? cx + 1 : cx;
    uint64_t cy1 = (cy + 1 < ny) ? cy + 1 : cy;

    // Fetch the 4 surrounding simulation cells used for bilinear interpolation:
    //
    //   TL (cx,  cy)     TR (cx1, cy)
    //   BL (cx,  cy1)    BR (cx1, cy1)
    //
    // For flattened array: index = row * nx + col = cy * nx + cx
    float TL_ux = pSnap->ux[cy * nx + cx];
    float TR_ux = pSnap->ux[cy * nx + cx1];
    float BL_ux = pSnap->ux[cy1 * nx + cx];
    float BR_ux = pSnap->ux[cy1 * nx + cx1];

    // Interpolate horizontally between TL->TR and BL->BR.
    double top = lerp(TL_ux, TR_ux, fxFrac);
    double bottom = lerp(BL_ux, BR_ux, fxFrac);

    // Interpolate vertically between the two horizontal results.
    return lerp(top, bottom, fyFrac);
}

// Try to parallelize in CUDA once you get access to TACC
static inline double InterpolateUY(SimSnap_t* pSnap, uint64_t fx, uint64_t fy,
                                   uint64_t scalingFactor) {
    // Convert pixel coordinates (fx, fy) into continuous simulation space.
    double simX = (double)fx / (double)scalingFactor;
    double simY = (double)fy / (double)scalingFactor;

    // Base simulation cell indices (top-left of the interpolation square).
    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    // Fractional part inside the cell.
    double fxFrac = simX - (double)cx;
    double fyFrac = simY - (double)cy;

    uint64_t nx = pSnap->nx;
    uint64_t ny = pSnap->ny;

    // Neighbor indices (right and bottom), clamped at borders.
    uint64_t cx1 = (cx + 1 < nx) ? cx + 1 : cx;
    uint64_t cy1 = (cy + 1 < ny) ? cy + 1 : cy;

    // Fetch the 4 surrounding simulation cells:
    //
    //   TL (cx,  cy)     TR (cx1, cy)
    //   BL (cx,  cy1)    BR (cx1, cy1)
    //
    // For flattened array: index = row * nx + col = cy * nx + cx
    float TL_uy = pSnap->uy[cy * nx + cx];
    float TR_uy = pSnap->uy[cy * nx + cx1];
    float BL_uy = pSnap->uy[cy1 * nx + cx];
    float BR_uy = pSnap->uy[cy1 * nx + cx1];

    // Horizontal interpolation of UY along top and bottom rows.
    double top = lerp(TL_uy, TR_uy, fxFrac);
    double bottom = lerp(BL_uy, BR_uy, fxFrac);

    // Vertical interpolation between the two results.
    return lerp(top, bottom, fyFrac);
}

static Color_t VelocityColor(double ux, double uy) {
    float speed = sqrtf(ux * ux + uy * uy);

    // Normalize speed to [0, 1] range
    // Adjust the scaling factor (0.1f) to control sensitivity
    float t = fminf(speed * 0.1f, 1.0f);

    Color_t c;
    // Grayscale mapping: black (0,0,0) for zero speed, white (1,1,1) for high speed
    c.r = t;
    c.g = t;
    c.b = t;
    c.a = 1.0f;
    return c;
}

// Try to parallelize in CUDA once you get access to TACC
transform_err_t Snap2ColorFrame(SimSnap_t* pSnap, Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pSnap && pFrame && pFrame->colors);
    uint64_t scalingFactor = pFrame->width / pSnap->nx;

    for (uint64_t j = 0; j < pFrame->height; j++) {
        for (uint64_t i = 0; i < pFrame->width; i++) {

            // ---- SOLID CELL OVERRIDE ----
            if (IsSolidCell(pSnap, i, j, scalingFactor)) {
                pFrame->colors[j * pFrame->width + i] =
                    (Color_t){.r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f};
                continue;
            }

            // ---- NORMAL VELOCITY-BASED COLOR ----
            double ux = InterpolateUX(pSnap, i, j, scalingFactor);
            double uy = InterpolateUY(pSnap, i, j, scalingFactor);

            pFrame->colors[j * pFrame->width + i] = VelocityColor(ux, uy);
        }
    }

    return TRANSFORM_SUCCESS;
}

#endif
