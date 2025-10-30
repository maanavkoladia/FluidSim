#pragma once
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

#define GRAVITY_CONST (-9.81)
#define FREQ (30)
#define OVERRELAXATION (1.9)

typedef enum {
    SIM_SUCCESS = 0,
    SIM_ERR,
    SIM_ERR_STARTUP
} sim_err_t;

typedef struct {
    double u; // right is pos, left is negative
    double v; // op is pos, down is neg
    double p;   // pressure at cell center
    double phi; // free surface / level set
    double density;
    double viscosity;
    uint8_t solid; // boundary/solid flag

} Cell_t;

typedef struct {
    double gravity_const;
    double timeStep;
    double nx, ny;      // num of cells
    double h;           // phtosical width of cell
    uint64_t timesteps; // numver of iterations
    double overrelaxation;
} SimState_t;

typedef struct {
    double density;
    double viscosity;
    double gravity[2];
    double timestep;
    double cfl_number;
    int grid_resolution[2];
    double domain_size[2];
    double initial_velocity[2];
    double initial_surface_height;
} sim_params_t;

typedef struct {
    double currTimeStep;
    int nx, ny;
    double dx, dy;
    Cell_t* cells;
    double dt;
    double max_velocity;
    double total_kinetic_energy;
    double fluid_volume;
} sim_state_t;



/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

// sim_err_t SimInit(sim_params_t* pParams);
sim_err_t SimInit(sim_params_t* pParams);

sim_err_t SimStart(void);

sim_err_t SimStop(void);

sim_err_t SimJoin(void);

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */
