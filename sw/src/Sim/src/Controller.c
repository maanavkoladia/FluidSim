#include "Controller.h"
#include "../inc/Sim.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include <cstdlib>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TASK_CONTROLLER_RET (NULL)
#define MASTER_MSG_LEN (64)
typedef struct {
    sim_params_t* runParams;
    char messageFromMaster[MASTER_MSG_LEN];
} ControllerArgs_t;

pthread_t controller_th;
AtomicFlag_t killFlag;

#define KILL_FLAG_SET (FLAG_SET)
#define KILL_FLAG_CLEAR (FLAG_CLEAR)

#define CHECK_FLAG_STATUS(killFlag)                                                                \
    do {                                                                                           \
        if (AtomicFlag_GetStatus(&killFlag) == KILL_FLAG_SET) {                                    \
            return TASK_CONTROLLER_RET;                                                            \
        }                                                                                          \
    } while (0)

// will free mem
static inline void CopyInArgs(void* pvArgsIn, char* msg, sim_params_t* pParamsOut) {
    ASSERT_COMMON(pvArgsIn, "Got NULL paamrs in");
    ASSERT_COMMON(pParamsOut, "Got NULL Params Out");
    ASSERT_COMMON(msg, "Got NULL message buf");
    ControllerArgs_t* pArgs = (ControllerArgs_t*)pvArgsIn;
    memcpy(pParamsOut, pArgs->runParams, sizeof(sim_params_t));
    strncpy(msg, pArgs->messageFromMaster, MASTER_MSG_LEN);
    free(pvArgsIn);
}

static inline sim_err_t AllocateCells(SimState_t* pSimStateBuf) {
    pSimStateBuf->cells = (Cell_t**)malloc(sizeof(Cell_t*) * pSimStateBuf->nx);
    FOR_LOOP_COMMON(i, pSimStateBuf->nx) {
        pSimStateBuf->cells[i] = (Cell_t*)malloc(sizeof(Cell_t) * pSimStateBuf->ny);
    }
    return SIM_SUCCESS;
}

static inline sim_err_t InitCellBoundaries(SimState_t* pSimStateBuf) {
    ASSERT_COMMON(pSimStateBuf, "PSimStatebuf is NULL");
    ASSERT_COMMON(pSimStateBuf->cells, "cells are null wtf");
    // set the bounds to 0
    FOR_LOOP_COMMON(i, pSimStateBuf->nx) {
        FOR_LOOP_COMMON(j, pSimStateBuf->ny) {
            pSimStateBuf->cells[i][j].type = FLUID;
            pSimStateBuf->cells[i][j].ux = INITIAL_CELL_U_X;
            pSimStateBuf->cells[i][j].uy = INITIAL_CELL_U_Y;
            pSimStateBuf->cells[i][j].p = INITIAL_CELL_P;
        }
    }

    FOR_LOOP_COMMON(i, pSimStateBuf->nx) {
        pSimStateBuf->cells[i][0].type = SOLID;
        pSimStateBuf->cells[i][pSimStateBuf->ny - 1].type = SOLID;
    }

    FOR_LOOP_COMMON(i, pSimStateBuf->ny) {
        pSimStateBuf->cells[0][i].type = SOLID;
        pSimStateBuf->cells[pSimStateBuf->nx - 1][i].type = SOLID;
    }
}

static inline sim_err_t InitSimState(sim_params_t* pParams, SimState_t** ppSimStateOut) {
    ASSERT_COMMON(pParams && ppSimStateOut, "Got NULL Args");
    ASSERT_COMMON(pParams->runTime.tv_nsec == 0, "Got an INVALIAD Run Time Args");
    SimState_t* pSimStateBuf = (SimState_t*)malloc(sizeof(SimState_t));
    if (!pSimStateBuf) {
        return SIM_ERR_CONTROLLER_SYSTEM;
    }

    // Copy shared parameters
    pSimStateBuf->dt = pParams->dt;
    pSimStateBuf->nx = pParams->nx + 2;
    pSimStateBuf->ny = pParams->ny + 2;
    pSimStateBuf->w = pParams->w;
    pSimStateBuf->overrelaxation_const = pParams->overrelaxation_const;

    pSimStateBuf->advectionScheme = pParams->advectionScheme;
    pSimStateBuf->PsolverScene = pParams->PsolverScene;
    pSimStateBuf->runTime = pParams->runTime;
    pSimStateBuf->p_density = pParams->p_density;
    pSimStateBuf->PSolver_Interations = pParams->PSolver_Interations;

    // Initialize remaining fields not in sim_params_t
    pSimStateBuf->totalTimeSteps = (double)pSimStateBuf->runTime.tv_sec / pSimStateBuf->dt;
    pSimStateBuf->timeStepCount = 0;
    pSimStateBuf->cells = NULL;
    ASSERT_COMMON_POSIX(AllocateCells(pSimStateBuf), "Failed to ALlocate the cell matrix");
    InitCellBoundaries(pSimStateBuf);
    // allocated cells
    *ppSimStateOut = pSimStateBuf;
    return SIM_SUCCESS;
}

static sim_err_t RunOnePassOver(void) {
    return SIM_SUCCESS;
}

void* Task_Controller(void* pvArgs) {
    LOG("Task_Controller Started Up");
    sim_params_t simParams;
    char msgBuf[MASTER_MSG_LEN];
    SimState_t* pSimState = NULL;
    CopyInArgs(pvArgs, msgBuf, &simParams);

    ASSERT_COMMON_POSIX(InitSimState(&simParams, &pSimState), "Failed to init simState Structure");
    while (1) {
        CHECK_FLAG_STATUS(killFlag);
        LOG("Ran TimeStep");
    }

    // timestep,
    // run sim interation, maybe pass in buffer that will be passed to opengl
    //
    return TASK_CONTROLLER_RET;
}

sim_err_t ControllerInit(sim_params_t* pParams) {
    ASSERT_COMMON(pParams, "Params is NULL");
    AtomicFlag_Init(&killFlag, "Controller Kill Flag", KILL_FLAG_CLEAR);
    ASSERT_COMMON_POSIX(pthread_create(&controller_th, NULL, Task_Controller, NULL),
                        "Failed to Launch controller thread");
    return SIM_SUCCESS;
}

sim_err_t ControllerStop(void) {
    AtomicFlag_UpdateStatus(&killFlag, KILL_FLAG_SET);
    ControllerJoin();
    return SIM_SUCCESS;
}

sim_err_t ControllerJoin(void) {
    ASSERT_COMMON_POSIX(pthread_join(controller_th, NULL), "Fialed to join controller thread");
    return SIM_SUCCESS;
}
