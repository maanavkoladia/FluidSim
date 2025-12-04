<<<<<<< HEAD
// gpu_fluid.cu (compile with nvcc)

#include <cuda_runtime.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "Sim.h"
#include "SimTypes.h"
#include "Control_cuda.h"
#include "Advection_cuda.h"
#include "Pressure_cuda.h"


static void Sim_State_SwapCellsInUse(SimState_t* pState) {
    ASSERT_COMMON_NOT_NULL(pState);
    pState->cellBufInUse = pState->cellBufInUse == USING_CELLS1 ? USING_CELLS2 : USING_CELLS1;
}

static Cell_t** GetCellsInUse(SimState_t* state) {
    ASSERT_COMMON_NOT_NULL(state);
    return state->CellBufs_Arr[state->cellBufInUse];
}

static Cell_t** GetCellNextInUse(SimState_t* state) {
    ASSERT_COMMON_NOT_NULL(state);
    return state->cellBufInUse == USING_CELLS1 ? state->CellBufs_Arr[USING_CELLS2]
                                               : state->CellBufs_Arr[USING_CELLS1];
}
#define CUDA_CHECK(call)                                             \
    do {                                                             \
        cudaError_t err = (call);                                    \
        if (err != cudaSuccess) {                                    \
            fprintf(stderr, "CUDA error %s:%d: %s\n",                \
                    __FILE__, __LINE__, cudaGetErrorString(err));    \
            exit(EXIT_FAILURE);                                      \
        }                                                            \
    } while (0)

#define FLAT_IDX(x,y,nx) ((int)((x) + (y) * (nx)))

static size_t cellsCount(int nx, int ny) {
    return (size_t)nx * (size_t)ny;
}

// ---------------------------------------------------------
// Init / shutdown
// ---------------------------------------------------------

bool GPUFluid_Init(SimState_t* g, GPUFluidState* s) {
    assert(g);
    assert(s);

    int nx = (int)g->nx;
    int ny = (int)g->ny;
    s->nx = nx;
    s->ny = ny;

    size_t N       = cellsCount(nx, ny);
    size_t bytesD  = N * sizeof(float);
    size_t bytesI  = N * sizeof(int);

    // Host scratch
    s->h_p        = (float*)malloc(bytesD);
    s->h_ux       = (float*)malloc(bytesD);
    s->h_uy       = (float*)malloc(bytesD);
    s->h_cellType = (int*)   malloc(bytesI);

    if (!s->h_p || !s->h_ux || !s->h_uy || !s->h_cellType) {
        fprintf(stderr, "GPUFluid_Init: host malloc failed\n");
        return false;
    }

    // Device allocations
    CUDA_CHECK(cudaMalloc((void**)&s->d_pCurr,    bytesD));
    CUDA_CHECK(cudaMalloc((void**)&s->d_pNext,    bytesD));
    CUDA_CHECK(cudaMalloc((void**)&s->d_uxCurr,   bytesD));
    CUDA_CHECK(cudaMalloc((void**)&s->d_uxNext,   bytesD));
    CUDA_CHECK(cudaMalloc((void**)&s->d_uyCurr,   bytesD));
    CUDA_CHECK(cudaMalloc((void**)&s->d_uyNext,   bytesD));
    CUDA_CHECK(cudaMalloc((void**)&s->d_cellType, bytesI));

    // Initialize from current CPU sim state
    Cell_t** curr = g->CellBufs_Arr[g->cellBufInUse];
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            int idx       = FLAT_IDX(x, y, nx);
            s->h_p[idx]   = curr[x][y].p;
            s->h_ux[idx]  = curr[x][y].ux;
            s->h_uy[idx]  = curr[x][y].uy;
            s->h_cellType[idx] = (int)curr[x][y].type;  // SOLID/FLUID enum
        }
    }

    CUDA_CHECK(cudaMemcpy(s->d_pCurr,  s->h_p,        bytesD, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(s->d_uxCurr, s->h_ux,       bytesD, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(s->d_uyCurr, s->h_uy,       bytesD, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(s->d_cellType, s->h_cellType, bytesI, cudaMemcpyHostToDevice));

    s->initialized = true;
    return true;
}

void GPUFluid_Shutdown(GPUFluidState* s) {
    if (!s) return;

    if (s->d_pCurr)    cudaFree(s->d_pCurr);
    if (s->d_pNext)    cudaFree(s->d_pNext);
    if (s->d_uxCurr)   cudaFree(s->d_uxCurr);
    if (s->d_uxNext)   cudaFree(s->d_uxNext);
    if (s->d_uyCurr)   cudaFree(s->d_uyCurr);
    if (s->d_uyNext)   cudaFree(s->d_uyNext);
    if (s->d_cellType) cudaFree(s->d_cellType);

    free(s->h_p);
    free(s->h_ux);
    free(s->h_uy);
    free(s->h_cellType);

    *s = (GPUFluidState){0};
}

// ---------------------------------------------------------
// Upload/downlaod helpers
// ---------------------------------------------------------

static void GPUFluid_UploadFromSim(SimState_t* g, GPUFluidState* s) {
    int nx = s->nx;
    int ny = s->ny;
    size_t N      = cellsCount(nx, ny);
    size_t bytesD = N * sizeof(float);

    Cell_t** curr = GetCellsInUse(g);
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            int idx       = FLAT_IDX(x, y, nx);
            s->h_p[idx]   = curr[x][y].p;
            s->h_ux[idx]  = curr[x][y].ux;
            s->h_uy[idx]  = curr[x][y].uy;
            // cellType usually static; skip unless you edit solids at runtime
        }
    }

    CUDA_CHECK(cudaMemcpy(s->d_pCurr,  s->h_p,  bytesD, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(s->d_uxCurr, s->h_ux, bytesD, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(s->d_uyCurr, s->h_uy, bytesD, cudaMemcpyHostToDevice));
}

static void GPUFluid_DownloadToSim(SimState_t* g, GPUFluidState* s) {
    int nx = s->nx;
    int ny = s->ny;
    size_t N      = cellsCount(nx, ny);
    size_t bytesD = N * sizeof(float);

    CUDA_CHECK(cudaMemcpy(s->h_p,  s->d_pCurr,  bytesD, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(s->h_ux, s->d_uxCurr, bytesD, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(s->h_uy, s->d_uyCurr, bytesD, cudaMemcpyDeviceToHost));

    Cell_t** next = GetCellNextInUse(g);

    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            int idx        = FLAT_IDX(x, y, nx);
            next[x][y].p   = s->h_p[idx];
            next[x][y].ux  = s->h_ux[idx];
            next[x][y].uy  = s->h_uy[idx];
            // preserve material type; copy from curr if needed
        }
    }

    // Maintain your float-buffer semantics
    Sim_State_SwapCellsInUse(g);
}

// ---------------------------------------------------------
// GPU step = pressure solve + advection
// ---------------------------------------------------------

static cudaError_t GPUFluid_RunStepOnDevice(SimState_t* g, GPUFluidState* s) {
    int nx = s->nx;
    int ny = s->ny;

    float dt    = g->dt;
    float rho   = g->p_density;
    float w     = g->w;
    float over  = g->overrelaxation_const;
    int    iters = (int)g->PSolver_Interations;

    // 1) Pressure solve (Jacobi + update velocities)
    cudaError_t err = RunPressureSolverCUDA(
        nx, ny,
        dt,
        rho,
        w,
        over,
        iters,
        s->d_pCurr,
        s->d_pNext,
        s->d_uxCurr,
        s->d_uyCurr,
        s->d_cellType
    );
    if (err != cudaSuccess) return err;

    // 2) Advect velocities using themselves
    err = AdvectVelocityCUDA(
        nx, ny,
        dt,
        w,
        s->d_uxCurr,
        s->d_uyCurr,
        s->d_uxNext,
        s->d_uyNext,
        s->d_cellType
    );
    if (err != cudaSuccess) return err;

    // Swap advected into Curr
    float* tmp;
    tmp = s->d_uxCurr; s->d_uxCurr = s->d_uxNext; s->d_uxNext = tmp;
    tmp = s->d_uyCurr; s->d_uyCurr = s->d_uyNext; s->d_uyNext = tmp;

    return cudaSuccess;
}

// Public entry: one full GPU step
sim_err_t RunFluidStep_GPU(SimState_t* g, GPUFluidState* s) {
    if (!s->initialized) {
        if (!GPUFluid_Init(g, s)) {
            return SIM_ERR;
        }
    }

    GPUFluid_UploadFromSim(g, s);

    cudaError_t err = GPUFluid_RunStepOnDevice(g, s);
    if (err != cudaSuccess) {
        fprintf(stderr, "GPUFluid_RunStepOnDevice failed: %s\n",
                cudaGetErrorString(err));
        return SIM_ERR;
    }

    GPUFluid_DownloadToSim(g, s);
    return SIM_SUCCESS;
}
