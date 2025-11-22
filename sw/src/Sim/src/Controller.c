#include "Controller.h"
#include "../inc/Sim.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include "SimTypes.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../Transform/inc/Transform.h"

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

Cell_t** GetCells(SimState_t* state){
    if(!state)return NULL;

    if(state->using_cells1){
        return state->cells1;
    }else{
        return state->cells2;
    }
}


Cell_t** CreateCellsBuffer(uint nx, uint ny){
    Cell_t** return_val = NULL;
    return_val = (Cell_t**)malloc(sizeof(Cell_t*) * nx);
    for(uint i = 0; i < nx; i++){
        return_val[i] = malloc(sizeof(Cell_t) * ny);
    }
    return return_val;
}

static inline sim_err_t AllocateCells(SimState_t* pSimStateBuf) {
    pSimStateBuf->cells1 = CreateCellsBuffer(pSimStateBuf->nx,pSimStateBuf->ny);
    pSimStateBuf->cells2 = CreateCellsBuffer(pSimStateBuf->nx,pSimStateBuf->ny);
    return SIM_SUCCESS;
}

sim_err_t InsertBounds(Cell_t** init_cells, uint nx, uint ny){
        FOR_LOOP_COMMON(i,nx) {
        FOR_LOOP_COMMON(j,ny) {
            init_cells[i][j].type = FLUID;
            init_cells[i][j].ux = INITIAL_CELL_U_X;
            init_cells[i][j].uy = INITIAL_CELL_U_Y;
            init_cells[i][j].p = INITIAL_CELL_P;
        }
    }
    FOR_LOOP_COMMON(i,nx) {
        init_cells[i][0].type = SOLID;
        init_cells[i][ny - 1].type = SOLID;
    }

    FOR_LOOP_COMMON(i,ny) {
        init_cells[0][i].type = SOLID;
        init_cells[nx - 1][i].type = SOLID;
    }

    return SIM_SUCCESS;
}

static inline sim_err_t InitCellBoundaries(SimState_t* pSimStateBuf) {
    ASSERT_COMMON(pSimStateBuf, "PSimStatebuf is NULL");
    ASSERT_COMMON(GetCells(pSimStateBuf), "cells are null wtf");
    // set the bounds to 0
    InsertBounds(pSimStateBuf->cells1,pSimStateBuf->nx,pSimStateBuf->ny);
    InsertBounds(pSimStateBuf->cells2,pSimStateBuf->nx,pSimStateBuf->ny);
    return SIM_SUCCESS;

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
    pSimStateBuf->cells1 = NULL;
    pSimStateBuf->cells2 = NULL;
    pSimStateBuf->using_cells1 = true;
    ASSERT_COMMON_POSIX(AllocateCells(pSimStateBuf), "Failed to ALlocate the cell matrix");
    InitCellBoundaries(pSimStateBuf);
    // allocated cells
    *ppSimStateOut = pSimStateBuf;
    return SIM_SUCCESS;
}
void FreeCells(Cell_t **cells, uint nx) {
    for (uint i = 0; i < nx; i++)
        free(cells[i]);
    free(cells);
}


void CopyCells(Cell_t **dst, Cell_t **src, uint nx, uint ny) {
    for (uint x = 0; x < nx; x++) {
        memcpy(dst[x], src[x], sizeof(Cell_t) * ny);
    }
}



 SimSnap_t* CreateSimSnap(SimState_t* state){
    SimSnap_t* res = malloc(sizeof(SimSnap_t));
    res->nx = state->nx;
    res->ny = state->ny;
    res->cells = CreateCellsBuffer(res->nx,res->ny);
    CopyCells(res->cells,GetCells(state),res->nx,res->ny);
    return res;
 }

static sim_err_t RunOnePassOver(SimState_t* pSimState) {
    ASSERT_COMMON(pSimState, "Got a NULL Sim State");
    // run psolver
    // run adection
    SimSnap_t* single_snap = CreateSimSnap(pSimState); 
    Transform_SendNewSimSnap(single_snap);
    return SIM_SUCCESS;
}

static void* Task_Controller(void* pvArgs) {
    LOG("Task_Controller Started Up");
    sim_params_t simParams;
    char msgBuf[MASTER_MSG_LEN];
    SimState_t* pSimState = NULL;
    uint64_t cycleCount = 0;
    CopyInArgs(pvArgs, msgBuf, &simParams);

    ASSERT_COMMON_POSIX(InitSimState(&simParams, &pSimState), "Failed to init simState Structure");
    while (1) {
        CHECK_FLAG_STATUS(killFlag);
        LOG("Ran TimeStep: %lu", cycleCount);
        ASSERT_COMMON_POSIX(RunOnePassOver(pSimState), "Fialed on passover %lu", cycleCount);
        cycleCount++;
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
