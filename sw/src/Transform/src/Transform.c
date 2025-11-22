/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "../inc/Transform.h"
#include "LFfifo"
/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

void* Task_TransformService(void) {

    while (1) {
    }

    return NULL;
}

transform_err_t Transform_Init(void) {

    return TRANSFORM_SUCCES;
}

transform_err_t Transform_Dtr(void) {
    return TRANSFORM_SUCCES;
}

transform_err_t Tranform_SendNewSimState(SimSnap_t* pSimState) {
    return TRANSFORM_SUCCES;
}
