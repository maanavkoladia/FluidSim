#include "PressureSolver.h"
#include "Assert_Common.h"
#include "Controller.h"
#include "ForLoop.h"
#include "LOG.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SUCCESSIVE_OVER_RELAXATION (1.0)

static inline double GetPressure(Cell_t** cells, uint64_t x, uint64_t y) {
    return cells[x][y].p;
}

static inline bool IsSolid(Cell_t** cells, uint64_t x, uint64_t y) {
    return cells[x][y].type == SOLID;
}

/* --- Helper to safely check if coordinate is solid (with bounds checking) --- */
static inline bool IsSolidSafe(Cell_t** cells, int64_t x, int64_t y, uint64_t nx, uint64_t ny) {
    if (x < 0 || y < 0 || (uint64_t)x >= nx || (uint64_t)y >= ny) {
        return true; // treat out-of-bounds as solid
    }
    return IsSolid(cells, (uint64_t)x, (uint64_t)y);
}

/* --- Pressure solver for a single cell (MAC grid convention) --- */
void PressureSolverCell(uint64_t x, uint64_t y, SimState_t* sim, Cell_t** cells, Cell_t** out) {
    ASSERT_COMMON_NOT_NULL(sim);
    ASSERT_COMMON_NOT_NULL(cells);
    ASSERT_COMMON_NOT_NULL(out);

    // If this cell is solid, pressure is zero
    if (IsSolid(cells, x, y)) {
        out[x][y].p = 0.0;
        return;
    }

    int64_t xi = (int64_t)x;
    int64_t yi = (int64_t)y;

    // Check neighboring cells for fluid/solid status
    bool flowTop = !IsSolidSafe(cells, xi, yi + 1, sim->nx, sim->ny);
    bool flowBottom = !IsSolidSafe(cells, xi, yi - 1, sim->nx, sim->ny);
    bool flowLeft = !IsSolidSafe(cells, xi - 1, yi, sim->nx, sim->ny);
    bool flowRight = !IsSolidSafe(cells, xi + 1, yi, sim->nx, sim->ny);

    unsigned edge_count = flowTop + flowBottom + flowLeft + flowRight;

    // If completely surrounded by solids, pressure is zero
    if (edge_count == 0) {
        out[x][y].p = 0.0;
        return;
    }

    // Gather neighbor pressures (zero if solid or out of bounds)
    double pT = 0.0, pB = 0.0, pL = 0.0, pR = 0.0;

    if (flowTop && yi + 1 < (int64_t)sim->ny) {
        pT = cells[x][yi + 1].p;
    }
    if (flowBottom && yi - 1 >= 0) {
        pB = cells[x][yi - 1].p;
    }
    if (flowLeft && xi - 1 >= 0) {
        pL = cells[xi - 1][y].p;
    }
    if (flowRight && xi + 1 < (int64_t)sim->nx) {
        pR = cells[xi + 1][y].p;
    }

    double pressureSum = pT + pB + pL + pR;

    // Get face velocities at cell edges (MAC grid)
    // For MAC grid:
    //   - ux[x][y] is velocity on the LEFT face of cell (x,y)
    //   - uy[x][y] is velocity on the BOTTOM face of cell (x,y)

    double velocityLeft = flowLeft ? cells[x][y].ux : 0.0;
    double velocityRight =
        flowRight ? (xi + 1 < (int64_t)sim->nx ? cells[xi + 1][y].ux : 0.0) : 0.0;
    double velocityBottom = flowBottom ? cells[x][y].uy : 0.0;
    double velocityTop = flowTop ? (yi + 1 < (int64_t)sim->ny ? cells[x][yi + 1].uy : 0.0) : 0.0;

    // Calculate velocity divergence term
    // div = (u_right - u_left) + (v_top - v_bottom)
    double velocityTerm = (velocityRight - velocityLeft) + (velocityTop - velocityBottom);

    double rho = sim->p_density;
    double dx = sim->w;
    double dt = sim->dt;

    if (dt == 0.0) dt = 1e-12; // defensive guard

    // Pressure equation: p_new = (sum_neighbors - rho * dx * div / dt) / edge_count
    double newP = (pressureSum - (rho * dx * velocityTerm / dt)) / (double)edge_count;

    // Apply Successive Over-Relaxation (SOR)
    double oldP = cells[x][y].p;
    out[x][y].p = oldP + (newP - oldP) * SUCCESSIVE_OVER_RELAXATION;
}

/* --- Perform one Gauss-Seidel / SOR iteration over all cells --- */
sim_err_t PressureSolveIteration(SimState_t* sim_state, Cell_t** cells, Cell_t** buffer) {
    ASSERT_COMMON_NOT_NULL(sim_state);
    ASSERT_COMMON_NOT_NULL(cells);
    ASSERT_COMMON_NOT_NULL(buffer);

    // Iterate over ALL interior cells (not boundary cells)
    // Boundaries should be solid anyway, but we iterate 0 to nx-1, 0 to ny-1
    for (uint64_t x = 0; x < sim_state->nx; ++x) {
        for (uint64_t y = 0; y < sim_state->ny; ++y) {
            PressureSolverCell(x, y, sim_state, cells, buffer);
        }
    }

    return SIM_SUCCESS;
}

/* --- Copy pressure from buffer back to cells (after each iteration) --- */
void CopyPressureBack(SimState_t* state) {
    Cell_t** curr = GetCellsInUse(state);
    Cell_t** next = GetCellNextInUse(state);

    FOR_LOOP_COMMON(i, state->nx) {
        FOR_LOOP_COMMON(j, state->ny) {
            curr[i][j].p = next[i][j].p;
        }
    }
}

/* --- Update face velocities from solved pressure field --- */
sim_err_t UpdateVelocities(SimState_t* sim_state) {
    ASSERT_COMMON_NOT_NULL(sim_state);

    Cell_t** cells = GetCellsInUse(sim_state);
    if (!cells) return SIM_ERR;

    if (sim_state->nx < 2 || sim_state->ny < 2) return SIM_ERR;

    double k = sim_state->dt / (sim_state->p_density * sim_state->w);
    uint64_t nx = sim_state->nx;
    uint64_t ny = sim_state->ny;

    // Update horizontal velocities (ux)
    // ux[x][y] is on the left face of cell (x,y)
    // This velocity is affected by pressure difference between cells (x-1) and (x)
    for (uint64_t x = 1; x < nx; x++) {
        for (uint64_t y = 0; y < ny; y++) {
            // Skip if either adjacent cell is solid
            if (IsSolid(cells, x, y) || IsSolid(cells, x - 1, y)) {
                cells[x][y].ux = 0.0;
                continue;
            }

            double pressureRight = GetPressure(cells, x, y);
            double pressureLeft = GetPressure(cells, x - 1, y);
            cells[x][y].ux -= k * (pressureRight - pressureLeft);
        }
    }

    // Update vertical velocities (uy)
    // uy[x][y] is on the bottom face of cell (x,y)
    // This velocity is affected by pressure difference between cells (y-1) and (y)
    for (uint64_t x = 0; x < nx; x++) {
        for (uint64_t y = 1; y < ny; y++) {
            // Skip if either adjacent cell is solid
            if (IsSolid(cells, x, y) || IsSolid(cells, x, y - 1)) {
                cells[x][y].uy = 0.0;
                continue;
            }

            double pressureTop = GetPressure(cells, x, y);
            double pressureBottom = GetPressure(cells, x, y - 1);
            cells[x][y].uy -= k * (pressureTop - pressureBottom);
        }
    }

    // Set boundary velocities to zero
    for (uint64_t x = 0; x < nx; x++) {
        cells[x][0].uy = 0.0;
        if (ny > 0) cells[x][ny - 1].uy = 0.0;
    }

    for (uint64_t y = 0; y < ny; y++) {
        cells[0][y].ux = 0.0;
        if (nx > 0) cells[nx - 1][y].ux = 0.0;
    }

    return SIM_SUCCESS;
}

/* --- Top-level pressure solver driver --- */
sim_err_t PressureSolver(SimState_t* sim_state) {
    ASSERT_COMMON_NOT_NULL(sim_state);

    // Ensure reasonable grid
    if (sim_state->nx < 2 || sim_state->ny < 2) return SIM_ERR;

    // Determine iteration count
    uint64_t iterations = sim_state->PSolver_Interations;

    // Run pressure solver iterations
    for (uint64_t it = 0; it < iterations; ++it) {
        // Solve pressure (reads from current, writes to next)
        PressureSolveIteration(sim_state, GetCellsInUse(sim_state), GetCellNextInUse(sim_state));

        // Swap buffers so next iteration reads the updated pressure
        Sim_State_SwapCellsInUse(sim_state);
    }

    // Update velocities based on final pressure field
    UpdateVelocities(sim_state);

    return SIM_SUCCESS;
}
