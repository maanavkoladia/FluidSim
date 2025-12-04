// PressureSolver_CUDA.cu

#include <cuda_runtime.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// =============================
// Config
// =============================
#define NUMBER_OF_PSLOVE_ITERATIONS 16

// Match your enum value
#ifndef SOLID
#define SOLID 2
#endif

// =============================
// Device helpers
// =============================

__device__ __forceinline__
int clamp_int_d(int v, int minVal, int maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

__device__ __forceinline__
float clamp_float_d(float v, float minVal, float maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

__device__ __forceinline__
float clamp01_d(float v) {
    return clamp_float_d(v, 0.0, 1.0);
}

__device__ __forceinline__
float lerp_d(float a, float b, float t) {
    return a + (b - a) * t;
}

__device__ __forceinline__
int idx2D(int x, int y, int nx) {
    return x + y * nx;
}

__device__ __forceinline__
bool isSolid_d(const int* __restrict__ cellType, int x, int y,
               int nx, int ny) {
    int cx = clamp_int_d(x, 0, nx - 1);
    int cy = clamp_int_d(y, 0, ny - 1);
    return cellType[idx2D(cx, cy, nx)] == SOLID;
}

__device__ __forceinline__
float getPressure_d(const float* __restrict__ p,
                     const int* __restrict__ cellType,
                     int x, int y, int nx, int ny) {
    int cx = clamp_int_d(x, 0, nx - 1);
    int cy = clamp_int_d(y, 0, ny - 1);
    (void)cellType; // kept for symmetry; not needed here
    return p[idx2D(cx, cy, nx)];
}

// Flow flags (0 or 1), like your CPU helpers
__device__ __forceinline__
int flowTop_d(const int* cellType, int x, int y, int nx, int ny) {
    return isSolid_d(cellType, x + 0, y + 1, nx, ny) ? 0 : 1;
}
__device__ __forceinline__
int flowLeft_d(const int* cellType, int x, int y, int nx, int ny) {
    return isSolid_d(cellType, x - 1, y + 0, nx, ny) ? 0 : 1;
}
__device__ __forceinline__
int flowRight_d(const int* cellType, int x, int y, int nx, int ny) {
    return isSolid_d(cellType, x + 1, y + 0, nx, ny) ? 0 : 1;
}
__device__ __forceinline__
int flowBottom_d(const int* cellType, int x, int y, int nx, int ny) {
    return isSolid_d(cellType, x + 0, y - 1, nx, ny) ? 0 : 1;
}

__device__ __forceinline__
int fluidEdgeCount_d(const int* cellType, int x, int y, int nx, int ny) {
    int fTop    = flowTop_d   (cellType, x, y, nx, ny);
    int fLeft   = flowLeft_d  (cellType, x, y, nx, ny);
    int fRight  = flowRight_d (cellType, x, y, nx, ny);
    int fBottom = flowBottom_d(cellType, x, y, nx, ny);
    return fLeft + fRight + fTop + fBottom;
}

// Matches your velTerm() logic, but with clamping to avoid OOB on GPU
__device__ __forceinline__
float velTerm_d(const float* __restrict__ ux,
                 const float* __restrict__ uy,
                 const int* __restrict__ cellType,
                 int x, int y, int nx, int ny, float dt)
{
    // Clamp for safety; if your CPU relies on ghost cells instead,
    // you can remove/change these clamping calls to match exactly.
    int xt  = clamp_int_d(x + 0, 0, nx - 1);
    int yt  = clamp_int_d(y + 1, 0, ny - 1);
    int xl  = clamp_int_d(x + 0, 0, nx - 1);
    int yl  = clamp_int_d(y + 0, 0, ny - 1);
    int xr  = clamp_int_d(x + 1, 0, nx - 1);
    int yr  = clamp_int_d(y + 0, 0, ny - 1);
    int xb  = clamp_int_d(x + 0, 0, nx - 1);
    int yb  = clamp_int_d(y + 0, 0, ny - 1);

    float velocityTop    = uy[idx2D(xt, yt, nx)];
    float velocityLeft   = ux[idx2D(xl, yl, nx)];
    float velocityRight  = ux[idx2D(xr, yr, nx)];
    float velocityBottom = uy[idx2D(xb, yb, nx)];

    (void)cellType; // not used here, but kept in signature for symmetry

    return (velocityRight - velocityLeft + velocityTop - velocityBottom) / dt;
}

// =============================
// Jacobi pressure solve kernel
// =============================

__global__
void PressureSolveKernel(const float* __restrict__ pCurr,
                         float* __restrict__ pNext,
                         const float* __restrict__ ux,
                         const float* __restrict__ uy,
                         const int* __restrict__ cellType,
                         int nx, int ny,
                         float dt,
                         float rho,   // p_density
                         float w,     // physical width
                         float overrelax_const)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= nx || y >= ny) return;

    int idx = idx2D(x, y, nx);

    bool solid = isSolid_d(cellType, x, y, nx, ny);
    int edges  = fluidEdgeCount_d(cellType, x, y, nx, ny);

    float newPressure = 0.0;

    if (solid || edges == 0) {
        newPressure = 0.0;
    } else {
        float pTop    = getPressure_d(pCurr, cellType,
                                       x + 0, clamp_int_d(y + 1, 0, ny - 1),
                                       nx, ny) * flowTop_d(cellType, x, y, nx, ny);
        float pLeft   = getPressure_d(pCurr, cellType,
                                       clamp_int_d(x - 1, 0, nx - 1), y,
                                       nx, ny) * flowLeft_d(cellType, x, y, nx, ny);
        float pRight  = getPressure_d(pCurr, cellType,
                                       clamp_int_d(x + 1, 0, nx - 1), y,
                                       nx, ny) * flowRight_d(cellType, x, y, nx, ny);
        float pBottom = getPressure_d(pCurr, cellType,
                                       x, clamp_int_d(y - 1, 0, ny - 1),
                                       nx, ny) * flowBottom_d(cellType, x, y, nx, ny);

        float pressureSum = pRight + pLeft + pTop + pBottom;

        float vTerm = velTerm_d(ux, uy, cellType, x, y, nx, ny, dt);

        newPressure = (pressureSum - rho * w * vTerm) / (float)edges;
    }

    float oldPressure = pCurr[idx];
    pNext[idx] = oldPressure + (newPressure - oldPressure) * overrelax_const;
}

// =============================
// Velocity update kernel
// =============================

__global__
void UpdateVelocitiesKernel(float* __restrict__ ux,
                            float* __restrict__ uy,
                            const float* __restrict__ p,
                            const int* __restrict__ cellType,
                            int nx, int ny,
                            float dt,
                            float rho,
                            float w)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= nx || y >= ny) return;

    int idx = idx2D(x, y, nx);

    float K = dt / (rho * w);

    // Horizontal velocities (ux)
    // This matches your CPU loops where ux[x,y] depends on p(x,y) and p(x-1,y)
    if (!isSolid_d(cellType, x, y, nx, ny) &&
        !isSolid_d(cellType, x - 1, y, nx, ny))
    {
        float pressureRight = getPressure_d(p, cellType, x,     y, nx, ny);
        float pressureLeft  = getPressure_d(p, cellType, x - 1, y, nx, ny);
        ux[idx] -= K * (pressureRight - pressureLeft);
    } else {
        ux[idx] = 0.0;
    }

    // Vertical velocities (uy)
    // uy[x,y] depends on p(x,y) and p(x,y-1)
    if (!isSolid_d(cellType, x, y, nx, ny) &&
        !isSolid_d(cellType, x, y - 1, nx, ny))
    {
        float pressureTop    = getPressure_d(p, cellType, x, y,     nx, ny);
        float pressureBottom = getPressure_d(p, cellType, x, y - 1, nx, ny);
        uy[idx] -= K * (pressureTop - pressureBottom);
    } else {
        uy[idx] = 0.0;
    }
}

// Example host-side wrapper
// Assumes:
//  - d_pCurr, d_pNext, d_ux, d_uy, d_cellType are valid device pointers
//  - They already contain the current state
//  - After this function, d_pCurr holds the final pressure field

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
                                  int* d_cellType)
{
    dim3 block(16, 16);
    dim3 grid((nx + block.x - 1) / block.x,
              (ny + block.y - 1) / block.y);

    for (int i = 0; i < num_iter; ++i) {
        PressureSolveKernel<<<grid, block>>>(
            d_pCurr, d_pNext,
            d_ux, d_uy,
            d_cellType,
            nx, ny,
            dt, rho, w, overrelax_const
        );

        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) return err;

        // ping-pong pressures
        float* tmp = d_pCurr;
        d_pCurr = d_pNext;
        d_pNext = tmp;
    }

    // After Jacobi iterations, update velocities using final pressure d_pCurr
    UpdateVelocitiesKernel<<<grid, block>>>(
        d_ux, d_uy,
        d_pCurr,
        d_cellType,
        nx, ny,
        dt, rho, w
    );
    return cudaGetLastError();
}



#define BLOCK_X 16
#define BLOCK_Y 16

__global__
void PressureSolveKernel_tiled(const float* __restrict__ pCurr,
                               float* __restrict__ pNext,
                               const float* __restrict__ ux,
                               const float* __restrict__ uy,
                               const int* __restrict__ cellType,
                               int nx, int ny,
                               float dt,
                               float rho,   // p_density
                               float w,     // physical width
                               float overrelax_const)
{
    int gx = blockIdx.x * BLOCK_X + threadIdx.x; // global x
    int gy = blockIdx.y * BLOCK_Y + threadIdx.y; // global y

    int tx = threadIdx.x; // local x in block
    int ty = threadIdx.y; // local y in block

    __shared__ float sh_p[BLOCK_Y + 2][BLOCK_X + 2];

    // Helper lambda: clamp + index
    auto clamp_int_d = [] __device__ (int v, int lo, int hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    };

    auto idx2D = [] __device__ (int x, int y, int nx_) {
        return x + y * nx_;
    };

    // Load center cell into shared
    if (gx < nx && gy < ny) {
        int idx = idx2D(gx, gy, nx);
        sh_p[ty + 1][tx + 1] = pCurr[idx];
    }

    // Load halo cells (left/right/top/bottom).
    // We clamp at global boundaries.

    // Left halo
    if (tx == 0 && gx < nx && gy < ny) {
        int gxL = clamp_int_d(gx - 1, 0, nx - 1);
        int idxL = idx2D(gxL, gy, nx);
        sh_p[ty + 1][0] = pCurr[idxL];
    }

    // Right halo
    if (tx == BLOCK_X - 1 && gx < nx && gy < ny) {
        int gxR = clamp_int_d(gx + 1, 0, nx - 1);
        int idxR = idx2D(gxR, gy, nx);
        sh_p[ty + 1][BLOCK_X + 1] = pCurr[idxR];
    }

    // Bottom halo
    if (ty == 0 && gx < nx && gy < ny) {
        int gyB = clamp_int_d(gy - 1, 0, ny - 1);
        int idxB = idx2D(gx, gyB, nx);
        sh_p[0][tx + 1] = pCurr[idxB];
    }

    // Top halo
    if (ty == BLOCK_Y - 1 && gx < nx && gy < ny) {
        int gyT = clamp_int_d(gy + 1, 0, ny - 1);
        int idxT = idx2D(gx, gyT, nx);
        sh_p[BLOCK_Y + 1][tx + 1] = pCurr[idxT];
    }

    // Optional: corners if you ever need diagonals. For your current stencil, not needed.

    __syncthreads();

    if (gx >= nx || gy >= ny) return;

    int idx = idx2D(gx, gy, nx);

    // small helpers re-used from earlier answer:
    auto isSolid_d = [&] __device__ (int x, int y) {
        int cx = clamp_int_d(x, 0, nx - 1);
        int cy = clamp_int_d(y, 0, ny - 1);
        return cellType[idx2D(cx, cy, nx)] == SOLID;
    };

    auto flowTop_d = [&] __device__ (int x, int y) {
        return isSolid_d(x + 0, y + 1) ? 0 : 1;
    };
    auto flowLeft_d = [&] __device__ (int x, int y) {
        return isSolid_d(x - 1, y + 0) ? 0 : 1;
    };
    auto flowRight_d = [&] __device__ (int x, int y) {
        return isSolid_d(x + 1, y + 0) ? 0 : 1;
    };
    auto flowBottom_d = [&] __device__ (int x, int y) {
        return isSolid_d(x + 0, y - 1) ? 0 : 1;
    };

    auto fluidEdgeCount_d = [&] __device__ (int x, int y) {
        int fT = flowTop_d(x, y);
        int fL = flowLeft_d(x, y);
        int fR = flowRight_d(x, y);
        int fB = flowBottom_d(x, y);
        return fL + fR + fT + fB;
    };

    auto velTerm_d = [&] __device__ (int x, int y) {
        int xt  = clamp_int_d(x + 0, 0, nx - 1);
        int yt  = clamp_int_d(y + 1, 0, ny - 1);
        int xl  = clamp_int_d(x + 0, 0, nx - 1);
        int yl  = clamp_int_d(y + 0, 0, ny - 1);
        int xr  = clamp_int_d(x + 1, 0, nx - 1);
        int yr  = clamp_int_d(y + 0, 0, ny - 1);
        int xb  = clamp_int_d(x + 0, 0, nx - 1);
        int yb  = clamp_int_d(y + 0, 0, ny - 1);

        float velocityTop    = uy[idx2D(xt, yt, nx)];
        float velocityLeft   = ux[idx2D(xl, yl, nx)];
        float velocityRight  = ux[idx2D(xr, yr, nx)];
        float velocityBottom = uy[idx2D(xb, yb, nx)];

        return (velocityRight - velocityLeft + velocityTop - velocityBottom) / dt;
    };

    bool solid = isSolid_d(gx, gy);
    int edges  = fluidEdgeCount_d(gx, gy);

    float newPressure = 0.0;

    if (solid || edges == 0) {
        newPressure = 0.0;
    } else {
        // Now use shared memory for neighbor pressures:
        // local coords for this cell in shared tile
        int sx = tx + 1;
        int sy = ty + 1;

        float pCenter  = sh_p[sy    ][sx    ];
        float pTop     = sh_p[sy + 1][sx    ];
        float pBottom  = sh_p[sy - 1][sx    ];
        float pLeft    = sh_p[sy    ][sx - 1];
        float pRight   = sh_p[sy    ][sx + 1];

        // Respect flow flags like in CPU code
        int fT = flowTop_d   (gx, gy);
        int fL = flowLeft_d  (gx, gy);
        int fR = flowRight_d (gx, gy);
        int fB = flowBottom_d(gx, gy);

        float pressureTop    = pTop    * fT;
        float pressureLeft   = pLeft   * fL;
        float pressureRight  = pRight  * fR;
        float pressureBottom = pBottom * fB;

        float pressureSum = pressureRight + pressureLeft + pressureTop + pressureBottom;
        float vTerm = velTerm_d(gx, gy);

        newPressure = (pressureSum - rho * w * vTerm) / (float)edges;
    }

    float oldPressure = sh_p[ty + 1][tx + 1]; // same as pCurr[idx]
    pNext[idx] = oldPressure + (newPressure - oldPressure) * overrelax_const;
}
