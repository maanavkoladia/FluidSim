/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "PressureSolver.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "SimTypes.h"



static SimState_t* g_sim_state = NULL;

/* ================================================== */
/*                    enums & types                   */
/* ================================================== */

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
#define NUMBER_OF_PSLOVE_ITERATIONS 8
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

Cell_t** CreateCellsBuffer(uint nx, uint ny){
    Cell_t** return_val = NULL;
    return_val = malloc(sizeof(Cell_t*) * nx);
    for(int i = 0; i < nx; i++){
        return_val[i] = malloc(sizeof(Cell_t) * ny);
    }
    return return_val;
}

void PressureSolverCell(uint x, uint y, SimState_t* g_sim_state, Cell_t** cell_buffer) {

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
    cell_buffer[x][y].p =
        (pressureSum - (density * width * (initVelocityCalc) / deltaTime)) / 4;
}

sim_err_t PressureSolveIteration(SimState_t* sim_state, Cell_t** buffer){
    for (uint i = 1; i < sim_state->nx - 1; i++) {
            for (uint j = 1; j < sim_state->ny - 1; j++) {
                PressureSolverCell(i, j,sim_state,buffer);
            }
        }
    return SIM_SUCCESS;
}

void FreeCells(Cell_t **cells, uint nx) {
    for (uint i = 0; i < nx; i++)
        free(cells[i]);
    free(cells);
}

sim_err_t PressureSolver(SimState_t* sim_state) {
    if(sim_state == NULL){
        return SIM_ERR;
    }

    Cell_t** cell_buffer = CreateCellsBuffer(sim_state->nx,sim_state->ny);
    double k = (sim_state->p_density * sim_state->w) / sim_state->dt;
    uint size_x = sim_state->nx;
    uint size_y = sim_state->ny;
    
    for(int i = 0; i < NUMBER_OF_PSLOVE_ITERATIONS; i++){
        PressureSolveIteration(sim_state, cell_buffer);
        UpdateVelocities(cell_buffer,size_x, size_y, k);
    }
    FreeCells(sim_state->cells,size_x);
    sim_state->cells = cell_buffer;

    return SIM_SUCCESS;
}

sim_err_t UpdateVelocities(Cell_t** cell_buffer, uint nx,uint ny, uint k){
    if(!cell_buffer) return SIM_ERR;
    for(int x = 0; x < nx; x++){
        for(int y = 0; y < ny; y++){
            //Update Horizontal Velocity
            double pressureRight = GetPressure(x + 1, y);
            double pressureLeft = GetPressure(x - 1, y);
            cell_buffer[x][y].ux -= k * (pressureRight - pressureLeft);
            //Update Vertical Velocity
            double pressureTop = GetPressure(x, y - 1);
            double pressureBottom = GetPressure(x, y + 1);
            cell_buffer[x][y].uy -= k * (pressureRight - pressureLeft);
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
