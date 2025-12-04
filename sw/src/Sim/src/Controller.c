#include "Controller.h"
#include "../../Transform/inc/Transform.h"
#include "Advection.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "Control_cuda.h"
#include "ForLoop.h"
#include "LOG.h"
#include "PressureSolver.h"
#include "Sim.h"
#include "SimTypes.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define TASK_CONTROLLER_RET (NULL)

GPUFluidState gpu = {0};
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

static inline uint64_t FLAT_IDX(uint64_t x, uint64_t y, uint64_t nx) {
    return y * nx + x;
}

// will free mem
static inline void CopyInArgs(void* pvArgsIn, sim_params_t* pParamsOut) {
    ASSERT_COMMON(pvArgsIn, "Got NULL paamrs in");
    ASSERT_COMMON(pParamsOut, "Got NULL Params Out");
    ControllerArgs_t* pArgs = (ControllerArgs_t*)pvArgsIn;
    memcpy(pParamsOut, pArgs->runParams, sizeof(sim_params_t));
    free(pvArgsIn);
}

void PrintCellVel(SimState_t* sim) {
    Cell_t** temp = sim->CellBufs_Arr[sim->cellBufInUse];

    printf("Cell Horitontal Velocities\n");
    for (int i = 0; i < sim->nx; i++) {
        for (int j = 0; j < sim->ny; j++) {
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

Cell_t** GetCellsInUse(SimState_t* state) {
    ASSERT_COMMON_NOT_NULL(state);
    return state->CellBufs_Arr[state->cellBufInUse];
}

Cell_t** GetCellNextInUse(SimState_t* state) {
    ASSERT_COMMON_NOT_NULL(state);
    return state->cellBufInUse == USING_CELLS1 ? state->CellBufs_Arr[USING_CELLS2]
                                               : state->CellBufs_Arr[USING_CELLS1];
}
Cell_t** CreateCellsBuffer(uint64_t nx, uint64_t ny) {
    Cell_t** return_val = NULL;
    return_val = (Cell_t**)malloc(sizeof(Cell_t*) * nx);
    for (uint64_t i = 0; i < nx; i++) {
        return_val[i] = malloc(sizeof(Cell_t) * ny);
    }
    return return_val;
}
float* FlattenUX(Cell_t** cells, uint64_t nx, uint64_t ny) {
    assert(cells);

    float* ux = (float*)malloc(sizeof(float) * nx * ny);
    assert(ux);

    for (uint64_t y = 0; y < ny; y++) {
        for (uint64_t x = 0; x < nx; x++) {
            ux[FLAT_IDX(x, y, nx)] = cells[x][y].ux;
        }
    }
    return ux;
}
float* FlattenUY(Cell_t** cells, uint64_t nx, uint64_t ny) {
    assert(cells);

    float* uy = (float*)malloc(sizeof(float) * nx * ny);
    assert(uy);

    for (uint64_t y = 0; y < ny; y++) {
        for (uint64_t x = 0; x < nx; x++) {
            uy[FLAT_IDX(x, y, nx)] = cells[x][y].uy;
        }
    }
    return uy;
}
float* FlattenPressure(Cell_t** cells, uint64_t nx, uint64_t ny) {
    assert(cells);

    float* p = (float*)malloc(sizeof(float) * nx * ny);
    assert(p);

    for (uint64_t y = 0; y < ny; y++) {
        for (uint64_t x = 0; x < nx; x++) {
            p[FLAT_IDX(x, y, nx)] = cells[x][y].p;
        }
    }
    return p;
}

CellMaterial_t* FlattenType(Cell_t** cells, uint64_t nx, uint64_t ny) {
    assert(cells);

    CellMaterial_t* type = (CellMaterial_t*)malloc(sizeof(CellMaterial_t) * nx * ny);
    assert(type);

    for (uint64_t y = 0; y < ny; y++) {
        for (uint64_t x = 0; x < nx; x++) {
            type[FLAT_IDX(x, y, nx)] = cells[x][y].type;
        }
    }
    return type;
}

sim_err_t Sim_SimSnap_Yeild(SimSnap_t* pSnap) {
    ASSERT_COMMON(pSnap, "NULL snap yeild");
    // FreeCells(pSnap->cells, pSnap->nx);
    free(pSnap->p);
    free(pSnap->type);
    free(pSnap->ux);
    free(pSnap->uy);
    free(pSnap);

    // LOG("Freed Yeild Snap");
    return SIM_SUCCESS;
}

static inline sim_err_t AllocateCells(SimState_t* pSimStateBuf) {
    FOR_LOOP_COMMON(i, NUM_OF_CELL_BUFS) {
        pSimStateBuf->CellBufs_Arr[i] = CreateCellsBuffer(pSimStateBuf->nx, pSimStateBuf->ny);
    }
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
    ASSERT_COMMON(GetCellsInUse(pSimStateBuf), "cells are null wtf");
    // set the bounds to 0
    FOR_LOOP_COMMON(i, NUM_OF_CELL_BUFS) {
        InsertBounds(pSimStateBuf->CellBufs_Arr[i], pSimStateBuf->nx, pSimStateBuf->ny);
    }
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
    pSimStateBuf->totalTimeSteps = (float)pSimStateBuf->runTime.tv_sec / pSimStateBuf->dt;
    pSimStateBuf->timeStepCount = 0;
    pSimStateBuf->cellBufInUse = USING_CELLS1;
    ASSERT_COMMON_POSIX(AllocateCells(pSimStateBuf), "Failed to A Llocate the cell matrix");
    InitCellBoundaries(pSimStateBuf);
    // allocated cells
    *ppSimStateOut = pSimStateBuf;
    return SIM_SUCCESS;
}

void Sim_State_SwapCellsInUse(SimState_t* pState) {
    ASSERT_COMMON_NOT_NULL(pState);
    pState->cellBufInUse = pState->cellBufInUse == USING_CELLS1 ? USING_CELLS2 : USING_CELLS1;
}

SimSnap_t* CreateSimSnap(SimState_t* state) {
    SimSnap_t* res = malloc(sizeof(SimSnap_t));
    res->nx = state->nx;
    res->ny = state->ny;

    res->ux = FlattenUX(GetCellsInUse(state), state->nx, state->ny);
    res->uy = FlattenUY(GetCellsInUse(state), state->nx, state->ny);
    res->p = FlattenPressure(GetCellsInUse(state), state->nx, state->ny);
    res->type = FlattenType(GetCellsInUse(state), state->nx, state->ny);

    return res;
}

#include <math.h>

static void InjectVelocityCircleLeftEdge(SimState_t* sim,
                                         uint64_t radius, // in cells
                                         uint64_t offset, // cells from left edge
                                         float ux)        // max rightward velocity
{
    if (!sim) return;

    Cell_t** cells = GetCellsInUse(sim);
    if (!cells) return;

    uint64_t nx = sim->nx;
    uint64_t ny = sim->ny;
    if (nx == 0 || ny == 0) return;

    float cx = (float)offset;
    float cy = (float)(ny - 1) * 0.5;

    float r = (float)radius;

    // Inner radius: full velocity
    // Outer radius: fully faded to 0
    float r_inner = 0.6 * r; // tweak 0.5–0.8 to taste
    float r_outer = r;

    float r_outer2 = r_outer * r_outer;

    uint64_t start_x = (offset > radius) ? (offset - radius) : 0;
    uint64_t end_x = (offset + radius < nx) ? (offset + radius) : nx - 1;

    for (uint64_t y = 0; y < ny; ++y) {
        float dy = (float)y - cy;

        for (uint64_t x = start_x; x <= end_x; ++x) {
            float dx = (float)x - cx;
            float dist2 = dx * dx + dy * dy;

            // Outside the outer radius: zero injection
            if (dist2 > r_outer2) {
                continue;
            }

            float dist = sqrt(dist2);
            float weight;

            if (dist <= r_inner) {
                // Flat core: full strength
                weight = 1.0;
            } else {
                // Smooth falloff from r_inner to r_outer
                float t = (dist - r_inner) / (r_outer - r_inner); // 0..1
                if (t < 0.0) t = 0.0;
                if (t > 1.0) t = 1.0;

                // "smootherstep": 6t^5 - 15t^4 + 10t^3 (C^2 continuous)
                float t2 = t * t;
                float t3 = t2 * t;
                float smoother = 6.0 * t3 * t2 - 15.0 * t2 * t2 + 10.0 * t3;

                // 1 at inner radius, 0 at outer radius
                weight = 1.0 - smoother;
            }

            if (weight <= 0.0) continue;

            Cell_t* c = &cells[x][y];
            c->ux += ux * weight;
        }
    }
}

static void InjectVelocityRect(SimState_t* sim, uint64_t x0,
                               uint64_t y0,              // lower-left corner (inclusive)
                               uint64_t x1, uint64_t y1, // upper-right corner (exclusive)
                               float ux, float uy        // velocity to inject
) {
    if (!sim) return;

    Cell_t** cells = GetCellsInUse(sim);

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

void CreateSolidSquare(SimState_t* pState, uint64_t dim) {
    ASSERT_COMMON_NOT_NULL(pState);
    uint64_t midX = pState->nx / 2;
    uint64_t midY = pState->ny / 2;
    Cell_t** pCells = GetCellsInUse(pState);
    FOR_LOOP_COMMON(i, dim) {
        FOR_LOOP_COMMON(j, dim) {
            pCells[i + midX][j + midY].type = SOLID;
            pCells[i + midX][j + midY].ux = 0;
            pCells[i + midX][j + midY].uy = 0;
            pCells[i + midX][j + midY].p = 0;
        }
    }
}

void CreateSolidCircle(SimState_t* pState, uint64_t radius) {
    ASSERT_COMMON_NOT_NULL(pState);

    uint64_t midX = pState->nx / 2;
    uint64_t midY = pState->ny / 2;

    Cell_t** pCells = GetCellsInUse(pState);

    // Loop over a bounding box around the circle
    for (int64_t dy = -(int64_t)radius; dy <= (int64_t)radius; dy++) {
        for (int64_t dx = -(int64_t)radius; dx <= (int64_t)radius; dx++) {

            // Circle equation: x^2 + y^2 <= r^2
            if ((dx * dx + dy * dy) <= (int64_t)(radius * radius)) {

                int64_t x = (int64_t)midX + dx;
                int64_t y = (int64_t)midY + dy;

                // Bounds check (important near edges)
                if (x < 0 || y < 0 || x >= (int64_t)pState->nx || y >= (int64_t)pState->ny) {
                    continue;
                }

                pCells[x][y].type = SOLID;
                pCells[x][y].ux = 0.0;
                pCells[x][y].uy = 0.0;
                pCells[x][y].p = 0.0;
            }
        }
    }
}

static void InjectVelocityCenter(SimState_t* sim) {
    uint64_t cx = sim->nx / 2;
    uint64_t cy = sim->ny / 2;
    uint64_t half_size = 3; // size = 2*half_size
    InjectVelocityRect(sim, cx - half_size, cy - half_size, cx + half_size, cy + half_size, 1.0,
                       0.0 // example: rightward velocity
    );
}

// asummign right is postivie
static void InjectVelocity_LeftEdge_ToRight(SimState_t* pState, velocity_t vel) {
    ASSERT_COMMON_NOT_NULL(pState);
    Cell_t** pCurrCells = GetCellsInUse(pState);
    FOR_LOOP_COMMON(i, pState->ny) {
        pCurrCells[10][i].ux += vel;
    }
}

static sim_err_t RunOnePassOver(SimState_t* pSimState) {
    ASSERT_COMMON(pSimState, "Got a NULL Sim State");
    // run psolver
    // LOG("Starting PressureSolver Passover");
    ASSERT_COMMON_POSIX(RunPressureSolver(pSimState), "Something in pSolve shat itself");
    // run adection
    // Send SimSnap frame

    ASSERT_COMMON_POSIX(AdvectVelocity(pSimState), "Something in pSolve shat itself");
    SimSnap_t* single_snap = CreateSimSnap(pSimState);
    while (Transform_SendNewSimSnap(single_snap) != TRANSFORM_SUCCESS) {
    }
    // Sim_SimSnap_Yeild(single_snap);
    return SIM_SUCCESS;
}

static sim_err_t FreeSimState(SimState_t* pSimState) {
    ASSERT_COMMON(pSimState, "NULL Simstate when freeing");
    FOR_LOOP_COMMON(i, NUM_OF_CELL_BUFS) {
        FreeCells(pSimState->CellBufs_Arr[i], pSimState->nx);
    }
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

    // PrintCellVel(pSimState);
    // CreateSolidSquare(pSimState, 4);
    //CreateSolidCircle(pSimState, 13);

    while (1) {
        if (AtomicFlag_GetStatus(&killFlag) == KILL_FLAG_SET) {
            FreeSimState(pSimState);
            return TASK_CONTROLLER_RET;
        }
        // InjectVelocityCenter(pSimState);
        // InjectVelocity_LeftEdge_ToRight(pSimState, 1);
        uint64_t radius = 10;                                     // tweak as needed, in cells
        InjectVelocityCircleLeftEdge(pSimState, radius, 10, 0.1); // strong rightward inlet

// PrintCellVel(pSimState);
//  LOG("Ran TimeStep: %lu", cycleCount);
#ifndef ON_REMOTE
        ASSERT_COMMON_POSIX(RunOnePassOver(pSimState), "Failed on passover %lu", cycleCount);
#endif

#ifdef ON_REMOTE
        RunFluidStep_GPU(pSimState, &gpu);
        SimSnap_t* single_snap = CreateSimSnap(pSimState);
        while (Transform_SendNewSimSnap(single_snap) != TRANSFORM_SUCCESS) {
        }
#endif
        cycleCount++;
        // usleep(2);
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
