#pragma once
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */
typedef enum : uint8_t {
    SIM_SUCCESS = 0,
    SIM_ERR,
    SIM_ERR_STARTUP
} sim_err_t;

typedef enum {
    NO_SLIP,
    FREE_SLIP,

} BoundaryType_t;

typedef enum {
    ADVECT_SEMI_LAGRANGIAN,
    ADVECT_UPWIND,
    ADVECT_MACCORMACK
} AdvectionScheme_t;

typedef enum {
    PROJ_JACOBI,
    PROJ_PCG,
    PROJ_MULTIGRID
} ProjectionSolver_t;

typedef struct {
    // --- Physics ---
    double density;
    double viscosity;
    double gravity[2];

    // --- Simulation control ---
    double timestep;
    double cfl_number;

    // --- Grid / domain ---
    int grid_resolution[2];
    double domain_size[2];

    // --- Boundaries ---
    BoundaryType_t boundary_type;

    // --- Initial conditions ---
    double initial_velocity[2];
    double initial_surface_height;

    // --- Numerics ---
    AdvectionScheme_t advection_scheme;
    ProjectionSolver_t projection_solver;
    double projection_tolerance;
    int projection_max_iters;
} sim_params_t;
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

sim_err_t SimInit(sim_params_t* pParams);

sim_err_t SimStart(void);

sim_err_t SimStop(void);

sim_err_t SimJoin(void);

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */
