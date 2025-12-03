#include "Controller.h"
#include "../../Transform/inc/Transform.h"
#include "../inc/Sim.h"
#include "Advection.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include "LOG.h"
#include "PressureSolver.h"
#include "SimTypes.h"
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TASK_CONTROLLER_RET (NULL)

typedef struct {
    sim_params_t* runParams;
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

void FreeCells(Cell_t** cells, uint64_t nx);

// will free mem
static inline void CopyInArgs(void* pvArgsIn, sim_params_t* pParamsOut) {
    ASSERT_COMMON(pvArgsIn, "Got NULL paamrs in");
    ASSERT_COMMON(pParamsOut, "Got NULL Params Out");
    ControllerArgs_t* pArgs = (ControllerArgs_t*)pvArgsIn;
    memcpy(pParamsOut, pArgs->runParams, sizeof(sim_params_t));
    free(pvArgsIn);
}

void PrintCellVel(SimState_t* sim){
    Cell_t** temp = sim->using_cells1 ? sim->cells1 : sim->cells2;

    printf("Cell Horitontal Velocities\n");
    for(int i = 0; i < sim->nx; i++){
        for(int j = 0; j < sim->ny; j++){
            printf("%f ", temp[i][j].ux);
        }
        printf("\n");
    }
}

void FreeCells(Cell_t** cells, uint64_t nx) {
    for (uint64_t i = 0; i < nx; i++)
        free(cells[i]);
    free(cells);
}

static void CopyCells(Cell_t** dst, Cell_t** src, uint64_t nx, uint64_t ny) {
    for (uint64_t x = 0; x < nx; x++) {
        memcpy(dst[x], src[x], sizeof(Cell_t) * ny);
    }
}

static Cell_t** GetCells(SimState_t* state) {
    if (!state) return NULL;

    if (state->using_cells1) {
        return state->cells1;
    } else {
        return state->cells2;
    }
}

Cell_t** CreateCellsBuffer(uint64_t nx, uint64_t ny) {
    Cell_t** return_val = NULL;
    return_val = (Cell_t**)malloc(sizeof(Cell_t*) * nx);
    for (uint64_t i = 0; i < nx; i++) {
        return_val[i] = malloc(sizeof(Cell_t) * ny);
    }
    return return_val;
}

sim_err_t Sim_SimSnap_Yeild(SimSnap_t* pSnap) {
    ASSERT_COMMON(pSnap, "NULL snap yeild");
    FreeCells(pSnap->cells, pSnap->nx);
    free(pSnap);
    // LOG("Freed Yeild Snap");
    return SIM_SUCCESS;
}

static inline sim_err_t AllocateCells(SimState_t* pSimStateBuf) {
    pSimStateBuf->cells1 = CreateCellsBuffer(pSimStateBuf->nx, pSimStateBuf->ny);
    pSimStateBuf->cells2 = CreateCellsBuffer(pSimStateBuf->nx, pSimStateBuf->ny);
    return SIM_SUCCESS;
}

sim_err_t InsertBounds(Cell_t** init_cells, uint64_t nx, uint64_t ny) {
    FOR_LOOP_COMMON(i, nx) {
        FOR_LOOP_COMMON(j, ny) {
            init_cells[i][j].type = FLUID;
            init_cells[i][j].ux = INITIAL_CELL_U_X;
            init_cells[i][j].uy = INITIAL_CELL_U_Y;
            init_cells[i][j].p = INITIAL_CELL_P;
        }
    }
    FOR_LOOP_COMMON(i, nx) {
        init_cells[i][0].type = SOLID;
        init_cells[i][ny - 1].type = SOLID;
    }

    FOR_LOOP_COMMON(i, ny) {
        init_cells[0][i].type = SOLID;
        init_cells[nx - 1][i].type = SOLID;
    }

    return SIM_SUCCESS;
}

static inline sim_err_t InitCellBoundaries(SimState_t* pSimStateBuf) {
    ASSERT_COMMON(pSimStateBuf, "PSimStatebuf is NULL");
    ASSERT_COMMON(GetCells(pSimStateBuf), "cells are null wtf");
    // set the bounds to 0
    InsertBounds(pSimStateBuf->cells1, pSimStateBuf->nx, pSimStateBuf->ny);
    InsertBounds(pSimStateBuf->cells2, pSimStateBuf->nx, pSimStateBuf->ny);
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
    pSimStateBuf->nx = pParams->nx;
    pSimStateBuf->ny = pParams->ny;
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

SimSnap_t* CreateSimSnap(SimState_t* state) {
    SimSnap_t* res = malloc(sizeof(SimSnap_t));
    res->nx = state->nx;
    res->ny = state->ny;
    res->cells = CreateCellsBuffer(res->nx, res->ny);
    CopyCells(res->cells, GetCells(state), res->nx, res->ny);
    return res;
}

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

void InjectVelocityRect(SimState_t* sim, uint64_t x0, uint64_t y0, // lower-left corner (inclusive)
                        uint64_t x1, uint64_t y1,                  // upper-right corner (exclusive)
                        double ux, double uy                       // velocity to inject
) {
    if (!sim) return;

    Cell_t** cells = sim->using_cells1 ? sim->cells1 : sim->cells2;

    // Clamp bounds to grid
    if (x1 > sim->nx) x1 = sim->nx;
    if (y1 > sim->ny) y1 = sim->ny;

    for (uint64_t y = y0; y < y1; y++) {
        for (uint64_t x = x0; x < x1; x++) {
            Cell_t* c = &cells[y][x];
            c->ux += ux;
            c->uy += uy;
        }
    }
}

void InjectVelocityCenter(SimState_t* sim) {
    uint64_t cx = sim->nx / 2;
    uint64_t cy = sim->ny / 2;
    uint64_t half_size = 2; // size = 2*half_size
    InjectVelocityRect(sim, cx - half_size, cy - half_size, cx + half_size, cy + half_size, 30.0,
                       0.0 // example: rightward velocity
    );
}

static sim_err_t RunOnePassOver(SimState_t* pSimState) {
    ASSERT_COMMON(pSimState, "Got a NULL Sim State");
    // run psolver
    // LOG("Starting PressureSolver Passover");
    ASSERT_COMMON_POSIX(PressureSolver(pSimState), "Something in pSolve shat itself");

    // run adection
    
    ASSERT_COMMON_POSIX(AdvectVelocity(pSimState), "Something in pSolve shat itself");

    // Send SimSnap frame
    SimSnap_t* single_snap = CreateSimSnap(pSimState);
    while (Transform_SendNewSimSnap(single_snap) != TRANSFORM_SUCCESS) {
    }
    // Sim_SimSnap_Yeild(single_snap);
    return SIM_SUCCESS;
}

static sim_err_t FreeSimState(SimState_t* pSimState) {
    ASSERT_COMMON(pSimState, "NULL Simstate when freeing");
    FreeCells(pSimState->cells1, pSimState->nx);
    FreeCells(pSimState->cells2, pSimState->nx);
    free(pSimState);
    return SIM_SUCCESS;
}
static void* Task_Controller(void* pvArgs) {
    LOG("Task_Controller Started Up");
    sim_params_t simParams;
    SimState_t* pSimState = NULL;
    uint64_t cycleCount = 0;
    CopyInArgs(pvArgs, &simParams);
    // Inject velocity
    ASSERT_COMMON_POSIX(InitSimState(&simParams, &pSimState), "Failed to init simState Structure");
    PrintCellVel(pSimState);
    InjectVelocityCenter(pSimState);
    while (1) {
        if (AtomicFlag_GetStatus(&killFlag) == KILL_FLAG_SET) {
            FreeSimState(pSimState);
            return TASK_CONTROLLER_RET;
        }
        PrintCellVel(pSimState);
        // LOG("Ran TimeStep: %lu", cycleCount);
        ASSERT_COMMON_POSIX(RunOnePassOver(pSimState), "Failed on passover %llu", cycleCount);
        cycleCount++;
        // sleep(1);
    }

    // timestep,
    // run sim interation, maybe pass in buffer that will be passed to opengl
    //
    return TASK_CONTROLLER_RET;
}

sim_err_t ControllerInit(sim_params_t* pParams) {
    ASSERT_COMMON(pParams, "Params is NULL");
    AtomicFlag_Init(&killFlag, "Controller Kill Flag", KILL_FLAG_CLEAR);

    ControllerArgs_t* pArgsBuf = (ControllerArgs_t*)malloc(sizeof(ControllerArgs_t));
    pArgsBuf->runParams = pParams;
    ASSERT_COMMON_POSIX(pthread_create(&controller_th, NULL, Task_Controller, pArgsBuf),
                        "Failed to Launch controller thread");
    return SIM_SUCCESS;
}

sim_err_t ControllerStop(void) {
    AtomicFlag_UpdateStatus(&killFlag, KILL_FLAG_SET);
    ControllerJoin();
    // now there shouldnt be buffers going to the tranform thread any moreA
    Transform_SimEngine_WaitFor_Teardown();
    LOG("Done waiting got tranform to return all of the sim_snap_bufs");
    return SIM_SUCCESS;
}

sim_err_t ControllerJoin(void) {
    ASSERT_COMMON_POSIX(pthread_join(controller_th, NULL), "Fialed to join controller thread");
    return SIM_SUCCESS;
}
