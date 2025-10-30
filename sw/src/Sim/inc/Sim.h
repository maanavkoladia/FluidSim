#pragma once
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>
#include <time.h>

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

#define GRAVITY_CONST (-9.81)
#define FREQ (1 / 30)
#define OVERRELAXATION (1.9)

typedef enum {
    SIM_SUCCESS = 0,
    SIM_ERR,
    SIM_ERR_STARTUP
} sim_err_t;

typedef enum {
    SEMI_LAGRANGIAN
} Advection_Scheme_t;

typedef enum {
    GAUSS_SEIDEL,
    JACOBI,
    RED_BLACK_GAUSS_SEIDEL
} PressureSolver_Scheme_t;

typedef enum {
    AIR,
    FLUID,
    SOLID
} CellMaterial_t;

typedef struct {
    double u; // right is pos, left is negative
    double v; // op is pos, down is neg
    double p; // pressure
    CellMaterial_t type;
} Cell_t;

typedef struct {
    double dt;
    double nx, ny;      // num of cells
    double w;           // phtosical width of cell
    uint64_t timesteps; // numver of iterations, if INT MAX, then inf
    double overrelaxation_const;
    Advection_Scheme_t advectionScheme;
    PressureSolver_Scheme_t PsolverScene;
    struct timespec runTime;
    double p_density;
    Cell_t** cells;
} SimState_t;

typedef struct {
    uint64_t nx, ny;
    double dt;
    double w;
    Advection_Scheme_t advectionScheme;
    PressureSolver_Scheme_t PsolverScene;
    struct timespec runTime;
    double p_density;
} sim_params_t;

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

sim_err_t Sim_Init(sim_params_t* pParams);

sim_err_t Sim_Start(void);

sim_err_t Sim_Stop(void);

sim_err_t Sim_Join(void);

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */
