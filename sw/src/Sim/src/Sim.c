#include "../inc/Sim.h"
#include "AtomicFlag.h"
#include "Controller.h"
#include <stdlib.h>
#include <string.h>

static AtomicFlag_t InitSucessFlag;

static sim_params_t simParams;

// start the sim controller and equations tps, set the params
sim_err_t SimInit(sim_params_t* pParams) {
    memcpy(&simParams, pParams, sizeof(sim_params_t));
#ifndef NDEBUG
    AtomicFlag_Init(&InitSucessFlag, "Sim Init Sucess Flag", FLAG_SET);
#endif
    return SIM_SUCCESS;
}

sim_err_t SimStart(void) {

#ifndef NDEBUG
    ASSERT_COMMON(AtomicFlag_GetStatus(&InitSucessFlag), "Init Flag not set, ie init failed");
#endif
    ASSERT_COMMON_POSIX(ControllerInit(), "Failed to Init the Sim Controller");
    return SIM_SUCCESS;
}

sim_err_t SimJoin(void) {
#ifndef NDEBUG
    ASSERT_COMMON(AtomicFlag_GetStatus(&InitSucessFlag), "Init Flag not set, ie init failed");
#endif
    ASSERT_COMMON_POSIX(ControllerJoin(), "Failed to Join Controller");
    // join the TP and others
    return SIM_SUCCESS;
}

sim_err_t SimStop(void) {
#ifndef NDEBUG
    ASSERT_COMMON(AtomicFlag_GetStatus(&InitSucessFlag), "Init Flag not set, ie init failed");
#endif
    // this is blocking point
    ASSERT_COMMON_POSIX(ControllerStop(), "Faild to Stop Sim Controller");
    LOG("Sim Exited");
    return SIM_SUCCESS;
}
