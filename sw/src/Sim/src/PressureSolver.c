/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "PressureSolver.h"
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

double getPressure(int x, int y) {
    return g_sim_state->cells[x][y].p;
}

double getVelocity(int x, int y) {
    return g_sim_state->cells[x][y].u;
}

void PressureSolverCell(int x, int y) {
    double pressureTop = getPressure(x, y - 1);
    double pressureBot = getPressure(x, y + 1);
    double pressureLeft = getPressure(x - 1, y);
    double pressureRight = getPressure(x + 1, y);

    double velocityTop = getVelocity(x, y - 1);
    double velocityBot = getVelocity(x, y + 1);
    double velocityLeft = getVelocity(x + 1, y);
    double velocityRight = getVelocity(x - 1, y);

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
    for (int i = 1; i < sim_state->nx - 1; i++) {
        for (int j = 1; j < sim_state->ny - 1; j++) {
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
