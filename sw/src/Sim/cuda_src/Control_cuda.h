// gpu_fluid.h
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "SimTypes.h"

typedef struct {
    int nx, ny;

    // Device arrays (SoA, float precision)
    float *d_pCurr, *d_pNext;
    float *d_uxCurr, *d_uxNext;
    float *d_uyCurr, *d_uyNext;
    int    *d_cellType;

    // Host scratch buffers for packing/unpacking
    float *h_p;
    float *h_ux;
    float *h_uy;
    int    *h_cellType;

    bool initialized;
} GPUFluidState;

// API
bool GPUFluid_Init(SimState_t* g, GPUFluidState* s);
sim_err_t RunFluidStep_GPU(SimState_t* g, GPUFluidState* s);
void GPUFluid_Shutdown(GPUFluidState* s);
