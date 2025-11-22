#pragma once

typedef enum {
    TRANSFORM_SUCCES = 0,
    TRANSFORM_ERR,
} transform_err_t;

transform_err_t Transform_Init(void);

transform_err_t Transform_Dtr(void);

transform_err_t Tranform_SendNewSimState(void);
