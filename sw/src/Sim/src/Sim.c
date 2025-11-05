#include "../inc/Sim.h"
#include "AtomicFlag.h"
#include "Controller.h"
#include "SimDebugUtils.h"
#include <stdlib.h>
#include <string.h>

static AtomicFlag_t InitSucessFlag;

static sim_params_t simParams_glob;

#define MFS_DEAD (FLAG_SET)
#define BRICKED_UP (FLAG_CLEAR)

// start the sim controller and equations tps, set the params
sim_err_t Sim_Init(sim_params_t* pParams) {
    memcpy(&simParams_glob, pParams, sizeof(sim_params_t));
#ifndef NDEBUG
    PrintSimParams(&simParams_glob);
    AtomicFlag_Init(&InitSucessFlag, "Sim Init Sucess Flag", FLAG_SET);
#endif
    return SIM_SUCCESS;
}

sim_err_t Sim_Start(void) {
#ifndef NDEBUG
    ASSERT_COMMON(AtomicFlag_GetStatus(&InitSucessFlag) == BRICKED_UP,
                  "Init Flag not set, ie init failed");
#endif
    ASSERT_COMMON_POSIX(ControllerInit(&simParams_glob), "Failed to Init the Sim Controller");
    return SIM_SUCCESS;
}

sim_err_t Sim_Join(void) {
#ifndef NDEBUG
    ASSERT_COMMON(AtomicFlag_GetStatus(&InitSucessFlag), "Init Flag not set, ie init failed");
#endif
    ASSERT_COMMON_POSIX(ControllerJoin(), "Failed to Join Controller");
    // join the TP and others
    return SIM_SUCCESS;
}

sim_err_t Sim_Stop(void) {
#ifndef NDEBUG
    ASSERT_COMMON(AtomicFlag_GetStatus(&InitSucessFlag), "Init Flag not set, ie init failed");
#endif
    // this is blocking point
    ASSERT_COMMON_POSIX(ControllerStop(), "Faild to Stop Sim Controller");
    LOG("Sim Exited");
    return SIM_SUCCESS;
}
