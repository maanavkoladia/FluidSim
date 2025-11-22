#pragma once

#include "../inc/Sim.h"
#include <stdint.h>
#include <stdbool.h>

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
    Cell_t** cells1;
    Cell_t** cells2;
    bool using_cells1; // used to ping pong data betwwen cells 1 & 2

} SimState_t;

