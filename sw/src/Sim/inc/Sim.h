#pragma once

#include <stdint.h>

typedef enum : uint8_t {
    SIM_SUCCESS = 0,
    SIM_ERR,
    SIM_ERR_STARTUP
} sim_err_t;

typedef struct {
    // --- Physics ---
    double density;    // kg/m^3
    double viscosity;  // Pa·s
    double gravity[2]; // gx, gy in m/s^2

    // --- Simulation control ---
    double timestep;   // dt in seconds
    double cfl_number; // CFL safety factor

    // --- Grid / domain ---
    int grid_resolution[2]; // Nx, Ny, number of cells in x and y
    double domain_size[2];  // width, height in meters, real world size of the
                            // sim in X, Y
    double cell_size[2];    // dx, optional, physical size of the cell relative to
                            // real world

    // --- Boundaries ---
    char boundary_type[32]; // "no_slip", "free_slip", etc.

    // --- Initial conditions ---
    double initial_velocity[2];    // u0, v0
    double initial_surface_height; // y position of water level

    // --- Numerics ---
    char advection_scheme[32];   // "semi_lagrangian", "upwind"
    char projection_solver[32];  // "pcg", "jacobi", "multigrid"
    double projection_tolerance; // solver tolerance
    int projection_max_iters;    // max iterations for solver
} sim_params_t;

sim_err_t SimInit(sim_params_t* pParams);

sim_err_t SimStart(void);

sim_err_t SimStop(void);

sim_err_t SimJoin(void);
