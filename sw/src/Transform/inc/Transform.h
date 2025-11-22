#pragma once

#include "../../Render/inc/Renderer.h"
#include "../../Sim/inc/Sim.h"

typedef enum {
    TRANSFORM_SUCCES = 0,
    TRANSFORM_ERR_INIT,
    TRANSFORM_ERR_DTR,
    TRANSFORM_ERR_SEND_FRAME,
} transform_err_t;

transform_err_t Transform_Init(void);

transform_err_t Transform_Dtr(void);

transform_err_t Tranform_SendNewSimState(SimSnap_t* pSimState);
