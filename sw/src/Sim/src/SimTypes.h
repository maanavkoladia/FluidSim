#pragma once

#include "../inc/Sim.h"
#include <stdbool.h>
#include <stdint.h>

#define NUM_OF_CELL_BUFS (2)
typedef enum {
    USING_CELLS1 = 0,
    USING_CELLS2 = 1
} Cell_Buf_In_Use_t;

typedef struct {
    uint64_t nx, ny; // num of cells
    double dt;
    double p_density;
    double w; // phtosical width of cell
    double overrelaxation_const;
    uint64_t PSolver_Interations;
    struct timespec runTime;
    uint64_t totalTimeSteps; // numver of iterations, if INT MAX, then inf
    Advection_Scheme_t advectionScheme;
    PressureSolver_Scheme_t PsolverScene;
    uint64_t timeStepCount;
    Cell_t** CellBufs_Arr[NUM_OF_CELL_BUFS];
    Cell_Buf_In_Use_t cellBufInUse;
} SimState_t;
