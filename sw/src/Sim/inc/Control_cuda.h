// gpu_fluid.h
#ifndef CONTROL_CUDA_H
#define CONTROL_CUDA_H

#include "SimTypes.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int nx, ny;

    // Device arrays (SoA, float precision)
    float *d_pCurr, *d_pNext;
    float *d_uxCurr, *d_uxNext;
    float *d_uyCurr, *d_uyNext;
    int* d_cellType;

    // Host scratch buffers for packing/unpacking
    float* h_p;
    float* h_ux;
    float* h_uy;
    int* h_cellType;

    bool initialized;
} GPUFluidState;

// API
bool GPUFluid_Init(SimState_t* g, GPUFluidState* s);
sim_err_t RunFluidStep_GPU(SimState_t* g, GPUFluidState* s);
void GPUFluid_Shutdown(GPUFluidState* s);

#ifdef __cplusplus
}
#endif

#endif
