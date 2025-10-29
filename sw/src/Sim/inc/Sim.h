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
    double d; // diverganve

} Cell_t;

typedef struct {
    double gravity_const;
    double timeStep;
    double nx, ny;      // num of cells
    double h;           // phtosical width of cell
    uint64_t timesteps; // numver of iterations
    double overrelaxation;
} SimState_t;

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
