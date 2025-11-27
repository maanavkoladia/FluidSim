#pragma once

#include "../inc/Transform.h"

transform_err_t Init_RawFrame(Render_Frame_t** pFrameOut, uint64_t cells);
transform_err_t TransForm_RawFrameYeild(Render_Frame_t* pFrame);
transform_err_t Snap2RawFrame(SimSnap_t* pSnap, Render_Frame_t* pFrame);
