// gpu_fluid.h
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "SimTypes.h"

typedef struct {
    int nx, ny;

    // Device arrays (SoA, double precision)
    double *d_pCurr, *d_pNext;
    double *d_uxCurr, *d_uxNext;
    double *d_uyCurr, *d_uyNext;
    int    *d_cellType;

    // Host scratch buffers for packing/unpacking
    double *h_p;
    double *h_ux;
    double *h_uy;
    int    *h_cellType;

    bool initialized;
} GPUFluidState;

// API
bool GPUFluid_Init(SimState_t* g, GPUFluidState* s);
sim_err_t RunFluidStep_GPU(SimState_t* g, GPUFluidState* s);
void GPUFluid_Shutdown(GPUFluidState* s);
