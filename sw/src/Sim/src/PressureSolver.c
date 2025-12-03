/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "PressureSolver.h"
#include "Assert_Common.h"
#include "Controller.h"
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
#define SUCCESSIVE_OVER_RELAXATION 1.7
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

static inline double GetPressure(Cell_t** cells, uint64_t x, uint64_t y) {

    return cells[x][y].p;
}

static inline double GetVelocityX(Cell_t** cells, uint64_t x, uint64_t y) {
    return cells[x][y].ux;
}

static inline double GetVelocityY(Cell_t** cells, uint64_t x, uint64_t y) {
    return cells[x][y].uy;
}

static inline bool IsSolid(Cell_t** cells, uint64_t x, uint64_t y) {
    return cells[x][y].type == SOLID;
}

void printCells(Cell_t** cells, uint64_t nx, uint64_t ny) {

    printf("========================================\n");
    printf("         Cell Field Debug Output\n");
    printf("========================================\n\n");

    /* ---------- Horizontal Velocity (ux) ---------- */
    printf("Horizontal Velocity (ux):\n\n");

    // Column headers (x-axis)
    printf("      ");
    for (uint64_t i = 0; i < nx; i++) {
        printf("%8lu ", i);
    }
    printf("\n");

    for (uint64_t j = 0; j < ny; j++) {
        printf("y=%-3lu ", j); // Row label
        for (uint64_t i = 0; i < nx; i++) {
            printf("%8.3f ", cells[i][j].ux);
        }
        printf("\n");
    }

    printf("\n");

    /* ---------- Pressure (p) ---------- */
    printf("Pressure (p):\n\n");

    // Column headers (x-axis)
    printf("      ");
    for (uint64_t i = 0; i < nx; i++) {
        printf("%8lu ", i);
    }
    printf("\n");

    for (uint64_t j = 0; j < ny; j++) {
        printf("y=%-3lu ", j);
        for (uint64_t i = 0; i < nx; i++) {
            printf("%8.3f ", cells[i][j].p);
        }
        printf("\n");
    }

    printf("\n========================================\n\n");
}

void PressureSolverCell(uint64_t x, uint64_t y, SimState_t* g_sim_state, Cell_t** current_cells,
                        Cell_t** cell_buffer) {

    uint64_t top_material = !IsSolid(current_cells, x, y - 1);
    uint64_t left_material = !IsSolid(current_cells, x - 1, y);
    uint64_t right_material = !IsSolid(current_cells, x + 1, y);
    uint64_t bottom_material = !IsSolid(current_cells, x, y + 1);
    uint64_t edge_count = top_material + left_material + right_material + bottom_material;
    if (IsSolid(current_cells, x, y) || (edge_count == 0)) {
        cell_buffer[x][y].p = 0;
        return;
    }
    double pressureTop = GetPressure(current_cells, x, y - 1) * top_material;
    double pressureBot = GetPressure(current_cells, x, y + 1) * bottom_material;
    double pressureLeft = GetPressure(current_cells, x - 1, y) * left_material;
    double pressureRight = GetPressure(current_cells, x + 1, y) * right_material;

    double velocityTop = GetVelocityY(current_cells, x, y - 1);
    double velocityBot = GetVelocityY(current_cells, x, y + 1);
    double velocityLeft = GetVelocityX(current_cells, x - 1, y);
    double velocityRight = GetVelocityX(current_cells, x + 1, y);

    double density = g_sim_state->p_density;
    double width = g_sim_state->w;

    double pressureSum = (pressureTop + pressureBot + pressureLeft + pressureRight);
    double initVelocityCalc = (velocityRight - velocityLeft + velocityTop - velocityBot);
    double deltaTime = g_sim_state->dt;
    double newPressure =
        (pressureSum - (density * width * (initVelocityCalc) / deltaTime)) / edge_count;
    double oldPressure = current_cells[x][y].p;
    cell_buffer[x][y].p = oldPressure + (newPressure - oldPressure) * SUCCESSIVE_OVER_RELAXATION;
}

sim_err_t PressureSolveIteration(SimState_t* sim_state, Cell_t** cells, Cell_t** buffer) {
    for (uint64_t i = 1; i < sim_state->nx - 1; i++) {
        for (uint64_t j = 1; j < sim_state->ny - 1; j++) {
            // printf("Vx: %d Vy: %d", i, j);
            PressureSolverCell(i, j, sim_state, cells, buffer);
        }
    }
    return SIM_SUCCESS;
}

void SyncVelocities(SimState_t* state) {
    Cell_t** curr = GetCellsInUse(state);
    Cell_t** next = GetCellNextInUse(state);

    FOR_LOOP_COMMON(i, state->nx) {
        FOR_LOOP_COMMON(j, state->ny) {
            next[i][j].ux = curr[i][j].ux;
            next[i][j].uy = curr[i][j].uy;
        }
    }
}

sim_err_t UpdateVelocities(Cell_t** cell_buffer, uint64_t nx, uint64_t ny, double k) {
    if (!cell_buffer) return SIM_ERR;
    for (uint64_t x = 0; x < nx - 1; x++) {
        for (uint64_t y = 0; y < ny - 1; y++) {

            if (IsSolid(cell_buffer, x, y)) {
                cell_buffer[x][y].ux = 0;
                cell_buffer[x][y].uy = 0;

            } else {
                double pressureRight = GetPressure(cell_buffer, x + 1, y);
                double pressureLeft = GetPressure(cell_buffer, x, y);
                cell_buffer[x][y].ux -= k * (pressureRight - pressureLeft);

                double pressureTop = GetPressure(cell_buffer, x, y + 1);
                double pressureBottom = GetPressure(cell_buffer, x, y);
                cell_buffer[x][y].uy -= k * (pressureTop - pressureBottom);
            }
        }
    }
    return SIM_SUCCESS;
}

sim_err_t PressureSolver(SimState_t* sim_state) {
    ASSERT_COMMON_NOT_NULL(sim_state);

    double k = sim_state->dt / (sim_state->p_density * sim_state->w);
    uint64_t size_x = sim_state->nx;
    uint64_t size_y = sim_state->ny;
    // LOG("Starting iteration loop");
    // printf("Cells Before pressure itteration");
    // printCells(GetCellsInUse(sim_state), size_x, size_y);
    SyncVelocities(sim_state);
    for (uint64_t i = 0; i < sim_state->PSolver_Interations; i++) {
        PressureSolveIteration(sim_state, GetCellsInUse(sim_state), GetCellNextInUse(sim_state));

        // Ping-pong: swap current and next
        // printCells(GetCellNextInUse(sim_state), size_x, size_y);
        Sim_State_SwapCellsInUse(sim_state);
    }
    // printf("Cells After pressure itteration\n");

    UpdateVelocities(GetCellsInUse(sim_state), size_x, size_y, k);
    // Update the flag so everyone else knows which buffer is active

    return SIM_SUCCESS;
}

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
