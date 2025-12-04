#include <cuda_runtime.h>

// ---------------------------------------------------------
// Host-side wrapper (similar to AdvectVelocity(SimState_t*))
// ---------------------------------------------------------

cudaError_t RunPressureSolverCUDA(int nx, int ny,
                                  float dt,
                                  float rho,
                                  float w,
                                  float overrelax_const,
                                  int num_iter,
                                  float* d_pCurr,
                                  float* d_pNext,
                                  float* d_ux,
                                  float* d_uy,
                                  int* d_cellType);