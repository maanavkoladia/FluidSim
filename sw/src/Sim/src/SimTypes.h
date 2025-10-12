#pragma once

#include "../inc/Sim.h"
#include <stdint.h>

typedef struct {
    double u;      // horizontal velocity at left face
    double v;      // vertical velocity at bottom face
    double p;      // pressure at cell center
    double phi;    // free surface / level set
    uint8_t solid; // boundary/solid flag
} cell_t;

typedef struct {
    double currTimeStep;
    int nx, ny;    // grid size
    double dx, dy; // cell spacing
    cell_t* cells; // 2D array of cell structs

    // --- Temporary / computed values per timestep ---
    double dt;                   // timestep (adaptive, based on CFL)
    double max_velocity;         // max(|u|, |v|) across all cells
    double total_kinetic_energy; // sum over cells
    double fluid_volume;         // volume of fluid (phi < 0)
} sim_state_t;
