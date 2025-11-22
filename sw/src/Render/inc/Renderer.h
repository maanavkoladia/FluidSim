#pragma once

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
    pressure_t *pressure;
    int nx; //grid dim x
    int ny; //grid dim y
    velocity_t ux; //Velocity X
    velocity_t uy; //Velocity Y
} Render_Frame_t;

render_err_t Render_Init(void);

render_err_t Render_Dtr(void);

render_err_t Render_Send_Frame(Render_Frame_t* pFrameIn);

// render_err_t Render_Frame_Init(Render_Frame_t** pFrameOut, ...);
//
// render_err_t Render_Frame_Dtr(Render_Frame_t* pFrameIn);
