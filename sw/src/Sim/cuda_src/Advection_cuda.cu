// Advection_CUDA.cu

#include <cuda_runtime.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

// Match your enum
#ifndef SOLID
#    define SOLID 2
#endif

// ---------------------------------------------------------
// Basic math helpers / types (device-side)
// ---------------------------------------------------------

typedef struct {
    float x;
    float y;
} Vector2;

__device__ __forceinline__ Vector2 vec2_add_d(Vector2 a, Vector2 b) {
    Vector2 r = {a.x + b.x, a.y + b.y};
    return r;
}

__device__ __forceinline__ Vector2 vec2_sub_d(Vector2 a, Vector2 b) {
    Vector2 r = {a.x - b.x, a.y - b.y};
    return r;
}

__device__ __forceinline__ Vector2 vec2_scale_d(Vector2 v, float s) {
    Vector2 r = {v.x * s, v.y * s};
    return r;
}

__device__ __forceinline__ float clamp_float_d(float v, float minVal, float maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

__device__ __forceinline__ int clamp_int_d(int v, int minVal, int maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

__device__ __forceinline__ float clamp01_d(float v) {
    return clamp_float_d(v, 0.0, 1.0);
}

__device__ __forceinline__ float lerp_d(float a, float b, float t) {
    return a + (b - a) * t;
}

__device__ __forceinline__ int idx2D(int x, int y, int nx) {
    return x + y * nx; // x is fastest
}

// ---------------------------------------------------------
// Geometry helpers (device-side)
// mirrors your CPU logic
// ---------------------------------------------------------

__device__ __forceinline__ bool isSolid_d(const int* __restrict__ cellType, int x, int y, int nx,
                                          int ny) {
    int cx = clamp_int_d(x, 0, nx - 1);
    int cy = clamp_int_d(y, 0, ny - 1);
    return cellType[idx2D(cx, cy, nx)] == SOLID;
}

// Center of a cell in world space
__device__ __forceinline__ Vector2 CellCentre_d(int nx, int ny, float w, int x, int y) {
    float boundsSizeX = (float)nx * w;
    float boundsSizeY = (float)ny * w;

    float bottomLeftX = -boundsSizeX * 0.5;
    float bottomLeftY = -boundsSizeY * 0.5;

    Vector2 base = {bottomLeftX, bottomLeftY};
    Vector2 offset = {(x + 0.5f) * w, (y + 0.5f) * w};

    return vec2_add_d(base, offset);
}

// ux: left edge center
__device__ __forceinline__ Vector2 LeftEdgeCentre_d(int nx, int ny, float w, int x, int y) {
    Vector2 c = CellCentre_d(nx, ny, w, x, y);
    Vector2 off = {w * 0.5f, 0.0};
    return vec2_sub_d(c, off);
}

// uy: bottom edge center
__device__ __forceinline__ Vector2 BottomEdgeCentre_d(int nx, int ny, float w, int x, int y) {
    Vector2 c = CellCentre_d(nx, ny, w, x, y);
    Vector2 off = {0.0, w * 0.5f};
    return vec2_sub_d(c, off);
}

// ---------------------------------------------------------
// Bilinear sampling on a staggered grid (device-side)
// Horizontal samples from ux, vertical from uy
// ---------------------------------------------------------

__device__ float SampleBilinearEdgesHorizontal_d(const float* __restrict__ ux, int edgeCountX,
                                                 int edgeCountY, float cellSize, Vector2 worldPos) {
    float width = (float)(edgeCountX - 1) * cellSize;
    float height = (float)(edgeCountY - 1) * cellSize;

    float px = (worldPos.x + width * 0.5) / cellSize;  // [0, edgeCountX]
    float py = (worldPos.y + height * 0.5) / cellSize; // [0, edgeCountY]

    int left = clamp_int_d((int)px, 0, edgeCountX - 2);
    int bottom = clamp_int_d((int)py, 0, edgeCountY - 2);
    int right = left + 1;
    int top = bottom + 1;

    float xFrac = clamp01_d(px - (float)left);
    float yFrac = clamp01_d(py - (float)bottom);

    // indices into ux field
    int idxLT = idx2D(left, top, edgeCountX);
    int idxRT = idx2D(right, top, edgeCountX);
    int idxLB = idx2D(left, bottom, edgeCountX);
    int idxRB = idx2D(right, bottom, edgeCountX);

    float valueTop = lerp_d(ux[idxLT], ux[idxRT], xFrac);
    float valueBottom = lerp_d(ux[idxLB], ux[idxRB], xFrac);
    return lerp_d(valueBottom, valueTop, yFrac);
}

__device__ float SampleBilinearEdgesVertical_d(const float* __restrict__ uy, int edgeCountX,
                                               int edgeCountY, float cellSize, Vector2 worldPos) {
    float width = (float)(edgeCountX - 1) * cellSize;
    float height = (float)(edgeCountY - 1) * cellSize;

    float px = (worldPos.x + width * 0.5) / cellSize;  // [0, edgeCountX]
    float py = (worldPos.y + height * 0.5) / cellSize; // [0, edgeCountY]

    int left = clamp_int_d((int)px, 0, edgeCountX - 2);
    int bottom = clamp_int_d((int)py, 0, edgeCountY - 2);
    int right = left + 1;
    int top = bottom + 1;

    float xFrac = clamp01_d(px - (float)left);
    float yFrac = clamp01_d(py - (float)bottom);

    int idxLT = idx2D(left, top, edgeCountX);
    int idxRT = idx2D(right, top, edgeCountX);
    int idxLB = idx2D(left, bottom, edgeCountX);
    int idxRB = idx2D(right, bottom, edgeCountX);

    float valueTop = lerp_d(uy[idxLT], uy[idxRT], xFrac);
    float valueBottom = lerp_d(uy[idxLB], uy[idxRB], xFrac);
    return lerp_d(valueBottom, valueTop, yFrac);
}

__device__ Vector2 GetVelocityAtWorldPos_d(const float* __restrict__ ux,
                                           const float* __restrict__ uy, int nx, int ny, float w,
                                           Vector2 worldPos) {
    int vxWidth = nx;
    int vxHeight = ny;
    int vyWidth = nx;
    int vyHeight = ny;

    float velX = SampleBilinearEdgesHorizontal_d(ux, vxWidth, vxHeight, w, worldPos);
    float velY = SampleBilinearEdgesVertical_d(uy, vyWidth, vyHeight, w, worldPos);

    Vector2 v = {velX, velY};
    return v;
}

// ---------------------------------------------------------
// Advection kernels
// ---------------------------------------------------------

// Advect horizontal velocities (ux)
__global__ void AdvectVelocityHorizontalKernel(const float* __restrict__ uxCurr,
                                               const float* __restrict__ uyCurr,
                                               float* __restrict__ uxNext,
                                               const int* __restrict__ cellType, int nx, int ny,
                                               float dt, float w) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= nx || y >= ny) return;

    int idx = idx2D(x, y, nx);

    // Match CPU: if solid at (x-1, y) or (x, y), keep ux unchanged
    if (isSolid_d(cellType, x - 1, y, nx, ny) || isSolid_d(cellType, x, y, nx, ny)) {
        uxNext[idx] = uxCurr[idx];
        return;
    }

    Vector2 pos = LeftEdgeCentre_d(nx, ny, w, x, y);
    Vector2 vel = GetVelocityAtWorldPos_d(uxCurr, uyCurr, nx, ny, w, pos);
    Vector2 posPrev = vec2_sub_d(pos, vec2_scale_d(vel, dt));

    // Semi-Lagrangian sample: take velocity at back-traced position
    float newUX = GetVelocityAtWorldPos_d(uxCurr, uyCurr, nx, ny, w, posPrev).x;
    uxNext[idx] = newUX;
}

// Advect vertical velocities (uy)
__global__ void AdvectVelocityVerticalKernel(const float* __restrict__ uxCurr,
                                             const float* __restrict__ uyCurr,
                                             float* __restrict__ uyNext,
                                             const int* __restrict__ cellType, int nx, int ny,
                                             float dt, float w) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= nx || y >= ny) return;

    int idx = idx2D(x, y, nx);

    // Match CPU: if solid at (x, y-1) or (x, y), keep uy unchanged
    if (isSolid_d(cellType, x, y - 1, nx, ny) || isSolid_d(cellType, x, y, nx, ny)) {
        uyNext[idx] = uyCurr[idx];
        return;
    }

    Vector2 pos = BottomEdgeCentre_d(nx, ny, w, x, y);
    Vector2 vel = GetVelocityAtWorldPos_d(uxCurr, uyCurr, nx, ny, w, pos);
    Vector2 posPrev = vec2_sub_d(pos, vec2_scale_d(vel, dt));

    float newUY = GetVelocityAtWorldPos_d(uxCurr, uyCurr, nx, ny, w, posPrev).y;
    uyNext[idx] = newUY;
}

// ---------------------------------------------------------
// Host-side wrapper (similar to AdvectVelocity(SimState_t*))
// ---------------------------------------------------------

cudaError_t AdvectVelocityCUDA(int nx, int ny, float dt, float w, const float* d_uxCurr,
                               const float* d_uyCurr, float* d_uxNext, float* d_uyNext,
                               const int* d_cellType) {
    dim3 block(16, 16);
    dim3 grid((nx + block.x - 1) / block.x, (ny + block.y - 1) / block.y);

    // Horizontal advection: uxCurr/uyCurr -> uxNext
    AdvectVelocityHorizontalKernel<<<grid, block>>>(d_uxCurr, d_uyCurr, d_uxNext, d_cellType, nx,
                                                    ny, dt, w);
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) return err;

    // Vertical advection: uxCurr/uyCurr -> uyNext
    AdvectVelocityVerticalKernel<<<grid, block>>>(d_uxCurr, d_uyCurr, d_uyNext, d_cellType, nx, ny,
                                                  dt, w);
    err = cudaGetLastError();
    if (err != cudaSuccess) return err;

    return cudaSuccess;
}
