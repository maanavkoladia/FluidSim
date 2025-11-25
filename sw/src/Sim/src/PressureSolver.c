/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "PressureSolver.h"
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

static inline double GetPressure(Cell_t** cells, uint x, uint y) {

    return cells[x][y].p;
}

static inline double GetVelocityX(Cell_t** cells, uint x, uint y) {
    return cells[x][y].ux;
}

static inline double GetVelocityY(Cell_t** cells, uint x, uint y) {
    return cells[x][y].uy;
}

static inline bool IsSolid(Cell_t** cells, uint x, uint y) {
    return cells[x][y].type == SOLID;
}

void printCells(Cell_t** cells, uint nx, uint ny) {
    FOR_LOOP_COMMON(i, nx) {
        FOR_LOOP_COMMON(j, ny) {
            printf("Xv: %f \t", cells[i][j].ux);
            printf("Yv: %f \t", cells[i][j].uy);
            printf("Pressure: %f \t", cells[i][j].p);
        }
        printf("\n");
    }
}

void PressureSolverCell(uint x, uint y, SimState_t* g_sim_state, Cell_t** current_cells,
                        Cell_t** cell_buffer) {

    uint top_material = IsSolid(current_cells,x,y-1);
    uint left_material = IsSolid(current_cells,x-1,y);
    uint right_material = IsSolid(current_cells,x+1,y);
    uint bottom_material = IsSolid(current_cells,x,y+1);
    uint edge_count = top_material + left_material + right_material + bottom_material;
    if(IsSolid(current_cells,x,y) || (edge_count == 0)){
        cell_buffer[x][y].p = 0;
        return; 
    }
    double pressureTop = GetPressure(current_cells, x, y - 1) * top_material;
    double pressureBot = GetPressure(current_cells, x, y + 1) * bottom_material;
    double pressureLeft = GetPressure(current_cells, x - 1, y)* left_material;
    double pressureRight = GetPressure(current_cells, x + 1, y)* right_material;

    double velocityTop = GetVelocityY(current_cells, x, y - 1);
    double velocityBot = GetVelocityY(current_cells, x, y + 1);
    double velocityLeft = GetVelocityX(current_cells, x - 1, y);
    double velocityRight = GetVelocityX(current_cells, x + 1, y);

    double density = g_sim_state->p_density;
    double width = g_sim_state->w;

    double pressureSum = (pressureTop + pressureBot + pressureLeft + pressureRight);
    double initVelocityCalc = (velocityRight - velocityLeft + velocityTop - velocityBot);
    double deltaTime = g_sim_state->dt;
    cell_buffer[x][y].p = (pressureSum - (density * width * (initVelocityCalc) / deltaTime)) / edge_count;
}

sim_err_t PressureSolveIteration(SimState_t* sim_state, Cell_t** cells, Cell_t** buffer) {
    for (uint i = 1; i < sim_state->nx - 1; i++) {
        for (uint j = 1; j < sim_state->ny - 1; j++) {
            // printf("Vx: %d Vy: %d", i, j);
            PressureSolverCell(i, j, sim_state, cells, buffer);
        }
    }
    return SIM_SUCCESS;
}

sim_err_t UpdateVelocities(Cell_t** cell_buffer, uint nx, uint ny, uint k) {
    if (!cell_buffer) return SIM_ERR;
    for (uint x = 1; x < nx - 1; x++) {
        for (uint y = 1; y < ny - 1; y++) {
            // LOG("Updating horizonal V");
            // Update Horizontal Velocity
            // printf("X: %d, Y: %d", x, y);
            if(IsSolid(cell_buffer,x,y)){
                cell_buffer[x][y].ux = 0;
                cell_buffer[x][y].uy = 0

            }else{
            double pressureRight = GetPressure(cell_buffer, x + 1, y);
            double pressureLeft = GetPressure(cell_buffer, x - 1, y);
            cell_buffer[x][y].ux -= k * (pressureRight - pressureLeft);
            // LOG("Updating vertical V");
            // Update Vertical Velocity
            double pressureTop = GetPressure(cell_buffer, x, y - 1);
            double pressureBottom = GetPressure(cell_buffer, x, y + 1);
            cell_buffer[x][y].uy -= k * (pressureTop - pressureBottom);
            }
        }
    }
    return SIM_SUCCESS;
}

sim_err_t PressureSolver(SimState_t* sim_state) {
    if (sim_state == NULL) {
        return SIM_ERR;
    }

    Cell_t** current_cells = NULL;
    Cell_t** next_cells = NULL;

    if (sim_state->using_cells1) {
        current_cells = sim_state->cells1;
        next_cells = sim_state->cells2;
    } else {
        current_cells = sim_state->cells2;
        next_cells = sim_state->cells1;
    }

    double k = sim_state->dt /(sim_state->p_density * sim_state->w) ;
    uint size_x = sim_state->nx;
    uint size_y = sim_state->ny;
    // LOG("Starting iteration loop");
    for (int i = 0; i < NUMBER_OF_PSLOVE_ITERATIONS; i++) {

        PressureSolveIteration(sim_state, current_cells, next_cells);
        // Ping-pong: swap current and next
        Cell_t** tmp = current_cells;
        current_cells = next_cells;
        next_cells = tmp;
    }
    // LOG("Ended iteration loop");

    ASSERT_COMMON(next_cells, "NULL cells buffer");
    ASSERT_COMMON(current_cells, "NULL cells buffer");

    // LOG("Cells: \n");
    // printCells(next_cells,size_x,size_y);
    // LOG("Cells: \n");
    // printCells(current_cells,size_x,size_y);
    UpdateVelocities(next_cells, size_x, size_y, k);
    // Update the flag so everyone else knows which buffer is active
    if (current_cells == sim_state->cells1) {
        sim_state->using_cells1 = true;
    } else {
        sim_state->using_cells1 = false;
    }

    return SIM_SUCCESS;
}

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
