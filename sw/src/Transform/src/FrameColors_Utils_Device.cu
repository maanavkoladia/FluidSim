
#include "../../config.h"
#include "FrameColors_Utils.h"
#ifdef ON_REMOTE

#    include "../../Render/inc/Renderer.h"
#    include "../../Sim/inc/Sim.h"
#    include "Assert_Common.h"
#    include <cuda_runtime.h>
#    include <math.h>
#    include <stdint.h>

<<<<<<< HEAD
// Flattened cell structure for GPU (since 2D arrays aren't GPU-friendly)
struct FlattenedCell {
    float ux;
    float uy;
    float p;
    int type;
    uint64_t fluidNeighbors;
};

=======
>>>>>>> ba10a1acac661d9a0ec67a4e4bd525bb2f42d04f
// Device function: Linear interpolation
__device__ inline float lerp_device(float a, float b, float t) {
    return a + (b - a) * t;
}

// Device function: Check if cell is solid
__device__ inline int IsSolidCell_Device(const CellMaterial_t* type, uint64_t nx, uint64_t ny,
                                         uint64_t fx, uint64_t fy, uint64_t scalingFactor) {
    float simX = (float)fx / (float)scalingFactor;
    float simY = (float)fy / (float)scalingFactor;

    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    // Clamp just in case
    if (cx >= nx) cx = nx - 1;
    if (cy >= ny) cy = ny - 1;

    // Access flattened array: index = cy * nx + cx
    return type[cy * nx + cx] == SOLID;
}

// Device function: Interpolate UX
<<<<<<< HEAD
__device__ inline float InterpolateUX_Device(const FlattenedCell* cells, uint64_t nx, uint64_t ny,
                                             uint64_t fx, uint64_t fy, uint64_t scalingFactor) {
=======
__device__ inline float InterpolateUX_Device(const float* ux, uint64_t nx, uint64_t ny, uint64_t fx,
                                             uint64_t fy, uint64_t scalingFactor) {
>>>>>>> ba10a1acac661d9a0ec67a4e4bd525bb2f42d04f
    float simX = (float)fx / (float)scalingFactor;
    float simY = (float)fy / (float)scalingFactor;

    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    float fxFrac = simX - (float)cx;
    float fyFrac = simY - (float)cy;

    uint64_t cx1 = (cx + 1 < nx) ? cx + 1 : cx;
    uint64_t cy1 = (cy + 1 < ny) ? cy + 1 : cy;

    // Access flattened array: row-major order (index = cy * nx + cx)
    float TL_ux = ux[cy * nx + cx];
    float TR_ux = ux[cy * nx + cx1];
    float BL_ux = ux[cy1 * nx + cx];
    float BR_ux = ux[cy1 * nx + cx1];

<<<<<<< HEAD
    float top = lerp_device(TL->ux, TR->ux, fxFrac);
    float bottom = lerp_device(BL->ux, BR->ux, fxFrac);
=======
    float top = lerp_device(TL_ux, TR_ux, fxFrac);
    float bottom = lerp_device(BL_ux, BR_ux, fxFrac);
>>>>>>> ba10a1acac661d9a0ec67a4e4bd525bb2f42d04f

    return lerp_device(top, bottom, fyFrac);
}

// Device function: Interpolate UY
<<<<<<< HEAD
__device__ inline float InterpolateUY_Device(const FlattenedCell* cells, uint64_t nx, uint64_t ny,
                                             uint64_t fx, uint64_t fy, uint64_t scalingFactor) {
=======
__device__ inline float InterpolateUY_Device(const float* uy, uint64_t nx, uint64_t ny, uint64_t fx,
                                             uint64_t fy, uint64_t scalingFactor) {
>>>>>>> ba10a1acac661d9a0ec67a4e4bd525bb2f42d04f
    float simX = (float)fx / (float)scalingFactor;
    float simY = (float)fy / (float)scalingFactor;

    uint64_t cx = (uint64_t)simX;
    uint64_t cy = (uint64_t)simY;

    float fxFrac = simX - (float)cx;
    float fyFrac = simY - (float)cy;

    uint64_t cx1 = (cx + 1 < nx) ? cx + 1 : cx;
    uint64_t cy1 = (cy + 1 < ny) ? cy + 1 : cy;

    // Access flattened array: row-major order (index = cy * nx + cx)
    float TL_uy = uy[cy * nx + cx];
    float TR_uy = uy[cy * nx + cx1];
    float BL_uy = uy[cy1 * nx + cx];
    float BR_uy = uy[cy1 * nx + cx1];

<<<<<<< HEAD
    float top = lerp_device(TL->uy, TR->uy, fxFrac);
    float bottom = lerp_device(BL->uy, BR->uy, fxFrac);
=======
    float top = lerp_device(TL_uy, TR_uy, fxFrac);
    float bottom = lerp_device(BL_uy, BR_uy, fxFrac);
>>>>>>> ba10a1acac661d9a0ec67a4e4bd525bb2f42d04f

    return lerp_device(top, bottom, fyFrac);
}

// Device function: Velocity to Color mapping
__device__ inline Color_t VelocityColor_Device(float ux, float uy) {
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
__global__ void Snap2ColorFrame_Kernel(const float* d_ux, const float* d_uy,
                                       const CellMaterial_t* d_type, uint64_t nx, uint64_t ny,
                                       Color_t* d_colors, uint64_t width, uint64_t height,
                                       uint64_t scalingFactor) {
    // Calculate pixel coordinates from thread/block indices
    uint64_t i = blockIdx.x * blockDim.x + threadIdx.x;
    uint64_t j = blockIdx.y * blockDim.y + threadIdx.y;

    // Bounds check
    if (i >= width || j >= height) {
        return;
    }

    // Check if solid cell
    if (IsSolidCell_Device(d_type, nx, ny, i, j, scalingFactor)) {
        d_colors[j * width + i] = (Color_t){.r = 0.0f, .g = 1.0f, .b = 0.0f, .a = 1.0f};
        return;
    }

    // Interpolate velocities
<<<<<<< HEAD
    float ux = InterpolateUX_Device(d_cells, nx, ny, i, j, scalingFactor);
    float uy = InterpolateUY_Device(d_cells, nx, ny, i, j, scalingFactor);
=======
    float ux = InterpolateUX_Device(d_ux, nx, ny, i, j, scalingFactor);
    float uy = InterpolateUY_Device(d_uy, nx, ny, i, j, scalingFactor);
>>>>>>> ba10a1acac661d9a0ec67a4e4bd525bb2f42d04f

    // Convert to color
    Color_t color = VelocityColor_Device(ux, uy);

    // Write to output (row-major: colors[y * width + x])
    d_colors[j * width + i] = color;
}

// CUDA wrapper function
transform_err_t Snap2ColorFrame(SimSnap_t* pSnap, Render_Frame_Colors_t* pFrame) {
    if (!pSnap || !pFrame || !pFrame->colors) {
        return TRANSFORM_ERR_SYSTEM;
    }

    uint64_t scalingFactor = pFrame->width / pSnap->nx;
    uint64_t numCells = pSnap->nx * pSnap->ny;
    uint64_t numPixels = pFrame->width * pFrame->height;

    float* d_ux;
    float* d_uy;
    CellMaterial_t* d_type;
    Color_t* d_colors;

    cudaError_t err;

    // Allocate device memory
    err = cudaMalloc((void**)&d_ux, sizeof(float) * numCells);
    if (err != cudaSuccess) {
        return TRANSFORM_ERR_SYSTEM;
    }

    err = cudaMalloc((void**)&d_uy, sizeof(float) * numCells);
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        return TRANSFORM_ERR_SYSTEM;
    }

    err = cudaMalloc((void**)&d_type, sizeof(CellMaterial_t) * numCells);
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        return TRANSFORM_ERR_SYSTEM;
    }

    err = cudaMalloc((void**)&d_colors, sizeof(Color_t) * numPixels);
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        cudaFree(d_type);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Copy data to device
    err = cudaMemcpy(d_ux, pSnap->ux, sizeof(float) * numCells, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        cudaFree(d_type);
        cudaFree(d_colors);
        return TRANSFORM_ERR_SYSTEM;
    }

    err = cudaMemcpy(d_uy, pSnap->uy, sizeof(float) * numCells, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        cudaFree(d_type);
        cudaFree(d_colors);
        return TRANSFORM_ERR_SYSTEM;
    }

    err =
        cudaMemcpy(d_type, pSnap->type, sizeof(CellMaterial_t) * numCells, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        cudaFree(d_type);
        cudaFree(d_colors);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Configure kernel launch parameters
    // Use 16x16 thread blocks
    dim3 blockSize(16, 16);
    dim3 gridSize((pFrame->width + blockSize.x - 1) / blockSize.x,
                  (pFrame->height + blockSize.y - 1) / blockSize.y);

    // Launch kernel
    Snap2ColorFrame_Kernel<<<gridSize, blockSize>>>(d_ux, d_uy, d_type, pSnap->nx, pSnap->ny,
                                                    d_colors, pFrame->width, pFrame->height,
                                                    scalingFactor);

    // Check for kernel launch errors
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        cudaFree(d_type);
        cudaFree(d_colors);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Wait for kernel to complete
    err = cudaDeviceSynchronize();
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        cudaFree(d_type);
        cudaFree(d_colors);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Copy results back to host
    err = cudaMemcpy(pFrame->colors, d_colors, sizeof(Color_t) * numPixels, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        cudaFree(d_ux);
        cudaFree(d_uy);
        cudaFree(d_type);
        cudaFree(d_colors);
        return TRANSFORM_ERR_SYSTEM;
    }

    // Cleanup
    cudaFree(d_ux);
    cudaFree(d_uy);
    cudaFree(d_type);
    cudaFree(d_colors);

    return TRANSFORM_SUCCESS;
}

transform_err_t Init_ColorFrame(Render_Frame_Colors_t** pFrameOut, uint64_t w, uint64_t h) {
    ASSERT_COMMON_NOT_NULL(pFrameOut);
    ASSERT_COMMON(w == RENDER_WINDOW_WIDTH, "Got invalid Color Fram Height");
    ASSERT_COMMON(h == RENDER_WINDOW_HEIGHT, "Got invalid Color Fram Width");
    Render_Frame_Colors_t* pFrameBuf =
        (Render_Frame_Colors_t*)malloc(sizeof(Render_Frame_Colors_t));

    pFrameBuf->height = h;
    pFrameBuf->width = w;
    pFrameBuf->colors = (Color_t*)malloc(sizeof(Color_t) * w * h);
#    ifndef NDEBUG
    ASSERT_COMMON_ALLOC(pFrameBuf->colors);
#    else
    if (!pFrameBuf->colors) {
        return TRANSFORM_ERR_SYSTEM;
    }
#    endif
    *pFrameOut = pFrameBuf;
    return TRANSFORM_SUCCESS;
}

transform_err_t TransForm_ColorFrameYeild(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    ASSERT_COMMON_NOT_NULL(pFrame->colors);
    free(pFrame->colors);
    free(pFrame);
    return TRANSFORM_SUCCESS;
}

#endif
