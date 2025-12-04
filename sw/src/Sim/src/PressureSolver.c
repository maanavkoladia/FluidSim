#include "PressureSolver.h"
#include "Assert_Common.h"
#include "Controller.h"
#include "ForLoop.h"
#include "LOG.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "omp.h"

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
#define NUMBER_OF_PSLOVE_ITERATIONS 16
#define SUCCESSIVE_OVER_RELAXATION 1.5
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

static inline float clamp_float(float v, float minVal, float maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

static inline int clamp_int(int v, int minVal, int maxVal) {
    if (v < minVal) return minVal;
    if (v > maxVal) return maxVal;
    return v;
}

static inline float clamp01(float v) {
    return clamp_float(v, 0.0f, 1.0f);
}

static inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

typedef struct {
    float flowLeft;
    float flowRight;
    float flowTop;
    float flowBottom;
    int   flowEdgeCount;
    bool  isSolid;
    float velocityTerm;
} PressureSolveData;

static bool FluidGrid_IsSolid(const SimState_t* g, int x, int y) {
    int cx = clamp_int(x, 0, g->nx - 1);
    int cy = clamp_int(y, 0, g->ny - 1);
    return g->CellBufs_Arr[g->cellBufInUse][cx][cy].type == SOLID;
}

static float FluidGrid_GetPressure(const SimState_t* g, int x, int y) {
    int cx = clamp_int(x, 0, g->nx - 1);
    int cy = clamp_int(y, 0, g->ny - 1);
    return g->CellBufs_Arr[g->cellBufInUse][cx][cy].p;
}
int flowTop(SimState_t* g, int x, int y){
    return FluidGrid_IsSolid(g, x + 0, y + 1) ? 0 : 1;
}
int flowLeft (SimState_t* g, int x, int y){
    return FluidGrid_IsSolid(g, x - 1, y + 0) ? 0 : 1;
}

int flowRight (SimState_t* g, int x, int y){
    return FluidGrid_IsSolid(g, x + 1, y + 0) ? 0 : 1;
}

int flowBottom (SimState_t* g, int x, int y){
    return FluidGrid_IsSolid(g, x + 0, y - 1) ? 0 : 1;
}
int fluidEdgeCount(SimState_t* g,int x, int y){
            int flowTop    = FluidGrid_IsSolid(g, x + 0, y + 1) ? 0 : 1;
            int flowLeft   = FluidGrid_IsSolid(g, x - 1, y + 0) ? 0 : 1;
            int flowRight  = FluidGrid_IsSolid(g, x + 1, y + 0) ? 0 : 1;
            int flowBottom = FluidGrid_IsSolid(g, x + 0, y - 1) ? 0 : 1;
             return flowLeft + flowRight + flowTop + flowBottom;
}



float velTerm(SimState_t* g, int x, int y){
       
            float velocityTop    = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 1].uy;
            float velocityLeft   = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 0].ux;
            float velocityRight  = g->CellBufs_Arr[g->cellBufInUse][x + 1][y + 0].ux;
            float velocityBottom = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 0].uy;

            return (velocityRight - velocityLeft + velocityTop - velocityBottom) / g->dt;
}

void SyncVeclocities(SimState_t* pSimState){
    Cell_t** curr = GetCellsInUse(pSimState);
    Cell_t** next = GetCellNextInUse(pSimState);

    FOR_LOOP_COMMON(i,pSimState->nx){
        FOR_LOOP_COMMON(j,pSimState->ny){
            next[i][j].ux = curr[i][j].ux;
            next[i][j].uy = curr[i][j].uy;
        }
    }
}

void SyncPressure(SimState_t* pSimState){
    Cell_t** curr = GetCellsInUse(pSimState);
    Cell_t** next = GetCellNextInUse(pSimState);

    FOR_LOOP_COMMON(i,pSimState->nx){
        FOR_LOOP_COMMON(j,pSimState->ny){
            next[i][j].p = curr[i][j].p;
        }
    }
}

void PressureSolve(SimState_t* g){
    Cell_t** current = GetCellsInUse(g);
    Cell_t** next_cells = GetCellNextInUse(g);
    uint64_t nx = g->nx;
    uint64_t ny = g->ny;
    SyncVeclocities(g);
    #pragma omp parallel for 
    FOR_LOOP_COMMON(i,nx){
        FOR_LOOP_COMMON(j,ny){
            float newPressure;
            if((FluidGrid_IsSolid(g,i,j)) || (fluidEdgeCount(g,i,j) == 0)){
                newPressure = 0;
            }else{
                float pressureTop    = current[i][ clamp_int(j + 1, 0, g->ny - 1) ].p * flowTop(g,i,j);
                float pressureLeft   = current[ clamp_int(i - 1, 0, g->nx - 1) ][j].p * flowLeft(g,i,j);
                float pressureRight  = current[ clamp_int(i + 1, 0, g->nx - 1) ][j].p * flowRight(g,i,j);
                float pressureBottom = current[i][ clamp_int(j - 1, 0, g->ny - 1) ].p * flowBottom(g,i,j);

                float pressureSum = pressureRight + pressureLeft + pressureTop + pressureBottom;
                newPressure = (pressureSum - g->p_density * g->w * velTerm(g,i,j)) / fluidEdgeCount(g,i,j);

            }
            float oldPressure = current[i][j].p;
            next_cells[i][j].p = oldPressure + (newPressure - oldPressure) * g->overrelaxation_const;
            
        }
    }
    Sim_State_SwapCellsInUse(g);
}

void FluidGrid_UpdateVelocities(SimState_t* g) {
    float dt = g->dt;
    float K = dt / (g->p_density * g->w);

    int vxWidth  = g->nx;
    int vxHeight = g->ny;
    int vyWidth  = g->nx;
    int vyHeight = g->ny;

    //SyncPressure(g);

    // Horizontal velocities
    for (int x = 0; x < vxWidth; x++) {
        for (int y = 0; y < vxHeight; y++) {
            if (FluidGrid_IsSolid(g, x, y) || FluidGrid_IsSolid(g, x - 1, y)) {
                g->CellBufs_Arr[g->cellBufInUse][x][y].ux = 0; //FORCE vel 0
                continue;
            }
            float pressureRight = FluidGrid_GetPressure(g, x,     y);
            float pressureLeft  = FluidGrid_GetPressure(g, x - 1, y);
            g->CellBufs_Arr[g->cellBufInUse][x][y].ux -= K * (pressureRight - pressureLeft);
        }
    }

    // Vertical velocities
    for (int x = 0; x < vyWidth; x++) {
        for (int y = 0; y < vyHeight; y++) {
            if (FluidGrid_IsSolid(g, x, y) || FluidGrid_IsSolid(g, x, y - 1)) {
                g->CellBufs_Arr[g->cellBufInUse][x][y].uy = 0; //FORCE vel 0
                continue;
            }
            float pressureTop    = FluidGrid_GetPressure(g, x, y);
            float pressureBottom = FluidGrid_GetPressure(g, x, y - 1);
            g->CellBufs_Arr[g->cellBufInUse][x][y].uy -= K * (pressureTop - pressureBottom);
        }
    }
}

sim_err_t RunPressureSolver(SimState_t* pSimState){
    int num_iter = pSimState->PSolver_Interations;
    FOR_LOOP_COMMON(i,num_iter){
        PressureSolve(pSimState);
    }
    FluidGrid_UpdateVelocities(pSimState);
    return SIM_SUCCESS;
}