#pragma once
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

#define GRAVITY_CONST (-9.81)
#define INITIAL_CELL_U_X (0)
#define INITIAL_CELL_U_Y (0)
#define INITIAL_CELL_P (0)

typedef enum {
    SIM_SUCCESS = 0,
    SIM_ERR,
    SIM_ERR_STARTUP,
    SIM_ERR_CONTROLLER_SYSTEM
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
    uint64_t nx, ny;
    float dt;
    float p_density;
    float w;
    float overrelaxation_const;
    uint64_t PSolver_Interations;
    struct timespec runTime;
    Advection_Scheme_t advectionScheme;
    PressureSolver_Scheme_t PsolverScene;
} sim_params_t;

typedef struct {
    float ux; // right is pos, left is negative
    float uy; // op is pos, down is neg
    float p;  // pressure
    CellMaterial_t type;
    uint64_t fluidNeighbors;

} Cell_t;

typedef struct {
    uint64_t nx, ny; // num of cells
    float* ux;
    float* uy;
    float* p;
    CellMaterial_t* type;
} SimSnap_t;

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

sim_err_t Sim_SimSnap_Yeild(SimSnap_t* pSnap);

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */
