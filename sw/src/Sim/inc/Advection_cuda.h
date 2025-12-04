#pragma once
// ---------------------------------------------------------
// Host-side wrapper (similar to AdvectVelocity(SimState_t*))
// ---------------------------------------------------------
#include <cuda_runtime.h>

cudaError_t AdvectVelocityCUDA(
    int nx, int ny,
    float dt,
    float w,
    const float* d_uxCurr,
    const float* d_uyCurr,
    float* d_uxNext,
    float* d_uyNext,
    const int* d_cellType);