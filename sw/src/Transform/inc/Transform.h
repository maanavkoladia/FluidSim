#pragma once

#include "../../Render/inc/Renderer.h"
#include "../../Sim/inc/Sim.h"
#include <assert.h>

typedef enum {
    TRANSFORM_SUCCESS = 0,
    TRANSFORM_ERR_INIT,
    TRANSFORM_ERR_DTR,
    TRANSFORM_ERR_SEND_FRAME,
    TRANSFORM_ERR_SYSTEM,
} transform_err_t;

transform_err_t Transform_Init(void);

transform_err_t Transform_Dtr(void);

transform_err_t Transform_SendNewSimSnap(SimSnap_t* pSimSnap);

void Transform_SimEngine_WaitFor_Teardown(void);

void Transform_YeildRender_Frame_Colors(Render_Frame_Colors_t* pFrame);
