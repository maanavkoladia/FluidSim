#include "../../config.h"
#ifdef ON_REMOTE

#    include "../../Render/inc/Renderer.h"
#    include "../../Sim/inc/Sim.h"
#    include "FrameColors_Utils.h"
#    include <cuda_runtime.h>
#    include <device_launch_parameters.h>
#    include <math.h>
#    include <stdint.h>

// Flattened cell structure for GPU (since 2D arrays aren't GPU-friendly)
struct FlattenedCell {
    double ux;
    double uy;
    double p;
    int type;
    uint64_t fluidNeighbors;
};

// Device function: Linear interpolation
__device__ inline double lerp_device(double a, double b, double t) {
    return a + (b - a) * t;
}

// Device function: Interpolate UX
__device__ inline double InterpolateUX_Device(const FlattenedCell* cells, uint64_t nx, uint64_t ny,
                                              uint64_t fx, uint64_t fy, uint64_t scalingFactor) {
    double simX = (double)fx / (double)scalingFactor;
    double simY = (double)fy / (double)scalingFactor;

    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    double fxFrac = simX - (double)cx;
    double fyFrac = simY - (double)cy;

    uint64_t cx1 = (cx + 1 < nx) ? cx + 1 : cx;
    uint64_t cy1 = (cy + 1 < ny) ? cy + 1 : cy;

    // Access flattened array: row-major order (cells[y * nx + x])
    const FlattenedCell* TL = &cells[cy * nx + cx];
    const FlattenedCell* TR = &cells[cy * nx + cx1];
    const FlattenedCell* BL = &cells[cy1 * nx + cx];
    const FlattenedCell* BR = &cells[cy1 * nx + cx1];

    double top = lerp_device(TL->ux, TR->ux, fxFrac);
    double bottom = lerp_device(BL->ux, BR->ux, fxFrac);

    return lerp_device(top, bottom, fyFrac);
}

// Device function: Interpolate UY
__device__ inline double InterpolateUY_Device(const FlattenedCell* cells, uint64_t nx, uint64_t ny,
                                              uint64_t fx, uint64_t fy, uint64_t scalingFactor) {
    double simX = (double)fx / (double)scalingFactor;
    double simY = (double)fy / (double)scalingFactor;

    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    double fxFrac = simX - (double)cx;
    double fyFrac = simY - (double)cy;

    uint64_t nx_local = nx;
    uint64_t ny_local = ny;
    uint64_t cx1 = (cx + 1 < nx_local) ? cx + 1 : cx;
    uint64_t cy1 = (cy + 1 < ny_local) ? cy + 1 : cy;

    const FlattenedCell* TL = &cells[cy * nx + cx];
    const FlattenedCell* TR = &cells[cy * nx + cx1];
    const FlattenedCell* BL = &cells[cy1 * nx + cx];
    const FlattenedCell* BR = &cells[cy1 * nx + cx1];

    double top = lerp_device(TL->uy, TR->uy, fxFrac);
    double bottom = lerp_device(BL->uy, BR->uy, fxFrac);

    return lerp_device(top, bottom, fyFrac);
}

// Device function: Velocity to Color mapping
__device__ inline Color_t VelocityColor_Device(double ux, double uy) {
    float speed = sqrtf((float)(ux * ux + uy * uy));
    float t = fminf(speed * 0.1f, 1.0f);

    Color_t c;
    c.r = t;
    c.g = t;
    c.b = t;
    c.a = 1.0f;
    return c;
}

// CUDA Kernel: Process each pixel in parallel
__global__ void Snap2ColorFrame_Kernel(const FlattenedCell* d_cells, uint64_t nx, uint64_t ny,
                                       Color_t* d_colors, uint64_t width, uint64_t height,
                                       uint64_t scalingFactor) {
    // Calculate pixel coordinates from thread/block indices
    uint64_t i = blockIdx.x * blockDim.x + threadIdx.x;
    uint64_t j = blockIdx.y * blockDim.y + threadIdx.y;

    // Bounds check
    if (i >= width || j >= height) {
        return;
    }

    // Interpolate velocities
    double ux = InterpolateUX_Device(d_cells, nx, ny, i, j, scalingFactor);
    double uy = InterpolateUY_Device(d_cells, nx, ny, i, j, scalingFactor);

    // Convert to color
    Color_t color = VelocityColor_Device(ux, uy);

    // Write to output (row-major: colors[y * width + x])
    d_colors[j * width + i] = color;
}

// Host function: Convert SimSnap to flattened array for GPU
static void FlattenSimSnap(SimSnap_t* pSnap, FlattenedCell* flatCells) {
    for (uint64_t i = 0; i < pSnap->nx; i++) {
        for (uint64_t j = 0; j < pSnap->ny; j++) {
            uint64_t idx = j * pSnap->nx + i; // row-major
            flatCells[idx].ux = pSnap->cells[i][j].ux;
            flatCells[idx].uy = pSnap->cells[i][j].uy;
            flatCells[idx].p = pSnap->cells[i][j].p;
            flatCells[idx].type = (int)pSnap->cells[i][j].type;
            flatCells[idx].fluidNeighbors = pSnap->cells[i][j].fluidNeighbors;
        }
    }
}

// CUDA wrapper function
transform_err_t Snap2ColorFrame(SimSnap_t* pSnap, Render_Frame_Colors_t* pFrame) {
    if (!pSnap || !pFrame || !pFrame->colors) {
        return TRANSFORM_ERR_SYSTEM;
    }

    uint64_t scalingFactor = pFrame->width / pSnap->nx;
    uint64_t numCells = pSnap->nx * pSnap->ny;
    uint64_t numPixels = pFrame->width * pFrame->height;

    // Allocate and copy cells to GPU
    FlattenedCell* h_cells = (FlattenedCell*)malloc(sizeof(FlattenedCell) * numCells);
    FlattenSimSnap(pSnap, h_cells);

    FlattenedCell* d_cells;
    Color_t* d_colors;

    cudaError_t err;

    // Allocate device memory
    err = cudaMalloc((void**)&d_cells, sizeof(FlattenedCell) * numCells);
    if (err != cudaSuccess) {
        free(h_cells);
        return TRANSFORM_ERR_SYSTEM;
    }

    err = cudaMalloc((void**)&d_colors, sizeof(Color_t) * numPixels);
    if (err != cudaSuccess) {
        cudaFree(d_cells);
        free(h_cells);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Copy cells to device
    err = cudaMemcpy(d_cells, h_cells, sizeof(FlattenedCell) * numCells, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cudaFree(d_cells);
        cudaFree(d_colors);
        free(h_cells);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Configure kernel launch parameters
    // Use 16x16 thread blocks
    dim3 blockSize(16, 16);
    dim3 gridSize((pFrame->width + blockSize.x - 1) / blockSize.x,
                  (pFrame->height + blockSize.y - 1) / blockSize.y);

    // Launch kernel
    Snap2ColorFrame_Kernel<<<gridSize, blockSize>>>(d_cells, pSnap->nx, pSnap->ny, d_colors,
                                                    pFrame->width, pFrame->height, scalingFactor);

    // Check for kernel launch errors
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        cudaFree(d_cells);
        cudaFree(d_colors);
        free(h_cells);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Wait for kernel to complete
    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        cudaFree(d_cells);
        cudaFree(d_colors);
        free(h_cells);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Copy results back to host
    err = cudaMemcpy(pFrame->colors, d_colors, sizeof(Color_t) * numPixels, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        cudaFree(d_cells);
        cudaFree(d_colors);
        free(h_cells);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Cleanup
    cudaFree(d_cells);
    cudaFree(d_colors);
    free(h_cells);

    return TRANSFORM_SUCCESS;
}

#endif
