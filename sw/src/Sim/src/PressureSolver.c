/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "PressureSolver.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

static SimState_t* g_sim_state = NULL;

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

static inline double GetPressure(uint x, uint y) {
    return g_sim_state->cells[x][y].p;
}

static inline double GetVelocityX(uint x, uint y) {
    return g_sim_state->cells[x][y].ux;
}

static inline double GetVelocityY(uint x, uint y) {
    return g_sim_state->cells[x][y].uy;
}

static inline bool IsSolid(uint x, uint y) {
    return g_sim_state->cells[x][y].type == SOLID;
}

void PressureSolverCell(uint x, uint y) {

    double pressureTop = getPressure(x, y - 1);
    double pressureBot = getPressure(x, y + 1);
    double pressureLeft = getPressure(x - 1, y);
    double pressureRight = getPressure(x + 1, y);

    double velocityTop = GetVelocityY(x, y - 1);
    double velocityBot = GetVelocityY(x, y + 1);
    double velocityLeft = GetVelocityX(x - 1, y);
    double velocityRight = GetVelocityX(x + 1, y);

    double density = g_sim_state->p_density;
    double width = g_sim_state->w;

    double pressureSum = (pressureTop + pressureBot + pressureLeft + pressureRight);
    double initVelocityCalc = (velocityRight - velocityLeft + velocityTop - velocityBot);
    double deltaTime = g_sim_state->dt;
    g_sim_state->cells[x][y].p =
        (pressureSum - (density * width * (initVelocityCalc) / deltaTime)) / 4;
}

sim_err_t PressureSolver(SimState_t* sim_state) {
    g_sim_state = sim_state;
    for (uint i = 1; i < sim_state->nx - 1; i++) {
        for (uint j = 1; j < sim_state->ny - 1; j++) {
            PressureSolverCell(i, j);
        }
    }
    return SIM_SUCCESS;
}

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
