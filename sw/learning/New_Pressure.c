/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "PressureSolver.h"
#include "Assert_Common.h"
#include "Controller.h"
#include "ForLoop.h"
#include "LOG.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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


typedef struct {
    double flowLeft;
    double flowRight;
    double flowTop;
    double flowBottom;
    int   flowEdgeCount;
    bool  isSolid;
    double velocityTerm;
} PressureSolveData;

static bool FluidGrid_IsSolid(const SimState_t* g, int x, int y) {
    int cx = clamp_int(x, 0, g->nx - 1);
    int cy = clamp_int(y, 0, g->ny - 1);
    return g->CellBufs_Arr[g->cellBufInUse][cx][cy].type == SOLID;
}

static double FluidGrid_GetPressure(const SimState_t* g, int x, int y) {
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

static void FluidGrid_PreparePressureSolver(SimState_t* g) {
    double dt = FluidGrid_TimeStep(g);

    for (int x = 0; x < g->nx; x++) {
        for (int y = 0; y < g->ny; y++) {
            int flowTop    = FluidGrid_IsSolid(g, x + 0, y + 1) ? 0 : 1;
            int flowLeft   = FluidGrid_IsSolid(g, x - 1, y + 0) ? 0 : 1;
            int flowRight  = FluidGrid_IsSolid(g, x + 1, y + 0) ? 0 : 1;
            int flowBottom = FluidGrid_IsSolid(g, x + 0, y - 1) ? 0 : 1;
            int fluidEdgeCount = flowLeft + flowRight + flowTop + flowBottom;
            bool isSolid = FluidGrid_IsSolid(g, x, y);
            g->CellBufs_Arr[g->cellBufInUse][x][y].fluidNeighbors = fluidEdgeCount;
            double velocityTop    = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 1].uy;
            double velocityLeft   = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 0].ux;
            double velocityRight  = g->CellBufs_Arr[g->cellBufInUse][x + 1][y + 0].ux;
            double velocityBottom = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 0].uy;

            double velTerm = (velocityRight - velocityLeft + velocityTop - velocityBottom) / dt;

            PressureSolveData d;
            d.flowLeft      = (double)flowLeft;
            d.flowRight     = (double)flowRight;
            d.flowTop       = (double)flowTop;
            d.flowBottom    = (double)flowBottom;
            d.isSolid       = isSolid;
            d.flowEdgeCount = fluidEdgeCount;
            d.velocityTerm  = velTerm;

        }
    }
}
double velTerm(SimState_t* g, int x, int y){
       
            double velocityTop    = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 1].uy;
            double velocityLeft   = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 0].ux;
            double velocityRight  = g->CellBufs_Arr[g->cellBufInUse][x + 1][y + 0].ux;
            double velocityBottom = g->CellBufs_Arr[g->cellBufInUse][x + 0][y + 0].uy;

            return (velocityRight - velocityLeft + velocityTop - velocityBottom) / g->dt;
}



void PressureSolve(SimState_t* g){
    FOR_LOOP_COMMON(i,g->nx){
        FOR_LOOP_COMMON(j,g->ny){
            double newPressure;
            if((g->CellBufs_Arr[g->cellBufInUse][i][j].type == SOLID) || (g->CellBufs_Arr[g->cellBufInUse][i][j].fluidNeighbors == 0)){
                newPressure = 0;
            }else{
                double pressureTop    = g->CellBufs_Arr[g->cellBufInUse][i][ clamp_int(j + 1, 0, g->ny - 1) ].p * flowBottom(g,i,j);
                double pressureLeft   = g->CellBufs_Arr[g->cellBufInUse][ clamp_int(i - 1, 0, g->nx - 1) ][j].p * flowLeft(g,i,j);
                double pressureRight  = g->CellBufs_Arr[g->cellBufInUse][ clamp_int(i + 1, 0, g->nx - 1) ][j].p * flowRight(g,i,j);
                double pressureBottom = g->CellBufs_Arr[g->cellBufInUse][i][ clamp_int(j - 1, 0, g->ny - 1) ].p * flowBottom(g,i,j);

                double pressureSum = pressureRight + pressureLeft + pressureTop + pressureBottom;
                newPressure = (pressureSum - g->p_density * g->w * velTerm(g,i,j)) / g->CellBufs_Arr[g->cellBufInUse][i][j].fluidNeighbors;

            }

            double oldPressure = g->CellBufs_Arr[g->cellBufInUse][i][j].p;
            g->CellBufs_Arr[g->cellBufInUse][i][j].p = oldPressure + (newPressure - oldPressure) * g->overrelaxation_const;
        }
    }
}


void RunPressureSolver(SimState_t* pSimState){
    FluidGrid_PreparePressureSolver(pSimState);
    int num_iter = pSimState->PSolver_Interations;
    FOR_LOOP_COMMON(i,num_iter){
        PressureSolve(pSimState);
    }
}