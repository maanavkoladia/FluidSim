#ifndef FRAMECOLORS_UTILS_H
#define FRAMECOLORS_UTILS_H

#include "../inc/Transform.h" // for transform_err_t
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

transform_err_t Snap2ColorFrame(SimSnap_t* pSnap, Render_Frame_Colors_t* pFrame);
transform_err_t Init_ColorFrame(Render_Frame_Colors_t** pFrameOut, uint64_t w, uint64_t h);
transform_err_t TransForm_ColorFrameYeild(Render_Frame_Colors_t* pFrame);

#ifdef __cplusplus
}
#endif

#endif // FRAMECOLORS_UTILS_H
