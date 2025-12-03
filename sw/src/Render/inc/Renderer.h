#pragma once

#include "../../config.h"
#include <stdint.h>

#define FRAME_IN_FIFO_SIZE ((1 << 10) - 1)
typedef enum {
    RENDER_SUCCESS = 0,
    RENDER_FAIL = 1,
} render_err_t;

typedef enum {
    RENDER_ALIVE,
    RENDER_DEAD
} render_status_t;

typedef double pressure_t;
typedef double velocity_t;

typedef struct {
    pressure_t* pressure;
    int nx;         // grid dim x
    int ny;         // grid dim y
    velocity_t* ux; // Velocity X
    velocity_t* uy; // Velocity Y
} Render_Frame_t;

typedef struct {
    float r, g, b, a;
} Color_t;

typedef struct {
    uint64_t width;
    uint64_t height;
    Color_t* colors; // flattened array: colors[y * width + x]
} Render_Frame_Colors_t;

render_err_t Render_Init(void);
render_err_t Render_Dtr(void);

render_err_t Render_Send_Raw_Frame(Render_Frame_t* pFrameIn);

render_err_t Render_Send_Frame_Colors(Render_Frame_Colors_t* pFrameIn);

// render_err_t Render_Frame_Init(Render_Frame_t** pFrameOut, ...);
//
// render_err_t Render_Frame_Dtr(Render_Frame_t* pFrameIn);
