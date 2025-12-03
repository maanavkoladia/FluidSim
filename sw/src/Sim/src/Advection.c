#include "Assert_Common.h"
#include "Controller.h"
#include "ForLoop.h"
#include "LOG.h"
#include <math.h> // for floor()
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// --- helpers ---

// Clamp a double value between min and max (keeps your original semantics)
double clamp_double(double value, double minVal, double maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// Clamp a value between 0 and 1
double clamp01(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}

// Integer clamp helper for indices
static inline uint64_t clamp_int64(uint64_t v, uint64_t lo, uint64_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Linear interpolation: mix a and b by t
double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

/*
 * NOTE about grid conventions used here:
 * - We follow the same mapping used in the Unity/C# code:
 *     physical domain in X runs from -width/2 .. +width/2 where width = (cellCountX) * cellSize
 * - px = (worldX + width/2) / cellSize -> gives position in units of cells (0 .. cellCountX)
 * - When sampling between grid nodes we use floor(px) as left index and fractional part as xFrac.
 *
 * The original C code had incorrect multiplication and wrong offsets; those are fixed below.
 */

// Interpolate X-velocity. This function assumes the velocity field components are stored
// in the Cell_t grid in whatever layout your program uses. The original code used
// cells[left][top].ux etc; we preserve that access pattern but fix the coordinate math.
// If your velocities are stored on a staggered array (u array size = nx+1 etc) you should
// adapt the indexing accordingly.
double InterpolateVelocityX(SimState_t* sim_state, double world_pos_x, double world_pos_y) {
    ASSERT_COMMON_NOT_NULL(sim_state);

    // number of cell samples in x/y (cellCountX, cellCountY)
    uint64_t nx = sim_state->nx;
    uint64_t ny = sim_state->ny;
    double cell_size = (double)sim_state->w; // cell size in world units (may be integer in struct)

    // Physical width/height of domain (same as C# width = cellCountX * cellSize)
    double width = (double)nx * cell_size;
    double height = (double)ny * cell_size;

    // Convert world position to grid-relative floating indices (px in [0, nx])
    // If world_pos_x runs from -width/2 .. +width/2, this maps to [0 .. nx]
    double px = (world_pos_x + width * 0.5) / cell_size;
    double py = (world_pos_y + height * 0.5) / cell_size;

    // floor to get integer "left" and "bottom" indices
    int64_t left_i = (int64_t)floor(px);
    int64_t bottom_j = (int64_t)floor(py);

    // clamp indices to valid interpolation range [0, nx-2], [0, ny-2]
    // make sure we don't underflow when converting to unsigned types
    if (left_i < 0) left_i = 0;
    if (bottom_j < 0) bottom_j = 0;
    if (left_i > (int64_t)nx - 2) left_i = (int64_t)nx - 2;
    if (bottom_j > (int64_t)ny - 2) bottom_j = (int64_t)ny - 2;

    uint64_t left = (uint64_t)left_i;
    uint64_t bottom = (uint64_t)bottom_j;
    uint64_t right = left + 1;
    uint64_t top = bottom + 1;

    // fractional part inside the cell
    double x_fraction = clamp01(px - (double)left);
    double y_fraction = clamp01(py - (double)bottom);

    Cell_t** cells = GetCellsInUse(sim_state);

    // bilinear interpolation
    double top_valueX = lerp(cells[left][top].ux, cells[right][top].ux, x_fraction);
    double bottom_valueX = lerp(cells[left][bottom].ux, cells[right][bottom].ux, x_fraction);
    double newX_velocity = lerp(bottom_valueX, top_valueX, y_fraction);

    return newX_velocity;
}

double InterpolateVelocityY(SimState_t* sim_state, double world_pos_x, double world_pos_y) {
    ASSERT_COMMON_NOT_NULL(sim_state);

    uint64_t nx = sim_state->nx;
    uint64_t ny = sim_state->ny;
    double cell_size = (double)sim_state->w;

    double width = (double)nx * cell_size;
    double height = (double)ny * cell_size;

    double px = (world_pos_x + width * 0.5) / cell_size;
    double py = (world_pos_y + height * 0.5) / cell_size;

    int64_t left_i = (int64_t)floor(px);
    int64_t bottom_j = (int64_t)floor(py);

    if (left_i < 0) left_i = 0;
    if (bottom_j < 0) bottom_j = 0;
    if (left_i > (int64_t)nx - 2) left_i = (int64_t)nx - 2;
    if (bottom_j > (int64_t)ny - 2) bottom_j = (int64_t)ny - 2;

    uint64_t left = (uint64_t)left_i;
    uint64_t bottom = (uint64_t)bottom_j;
    uint64_t right = left + 1;
    uint64_t top = bottom + 1;

    double x_fraction = clamp01(px - (double)left);
    double y_fraction = clamp01(py - (double)bottom);

    Cell_t** cells = GetCellsInUse(sim_state);

    double top_valueY = lerp(cells[left][top].uy, cells[right][top].uy, x_fraction);
    double bottom_valueY = lerp(cells[left][bottom].uy, cells[right][bottom].uy, x_fraction);
    double newY_velocity = lerp(bottom_valueY, top_valueY, y_fraction);

    return newY_velocity;
}

/*
 * AdvectVelocity corrected:
 * - Use consistent world positions and backtrace.
 * - For horizontal (U) component we sample at left-edge centers (x at i*cell + 0, y at j*cell +
 * 0.5*cell)
 * - For vertical (V) component we sample at bottom-edge centers (x at i*cell + 0.5*cell, y at
 * j*cell + 0)
 * - We then backtrace by dt using the interpolated velocity at that edge position.
 *
 * This follows the MAC / staggered sampling semantics from the C# code.
 *
 * NOTE: The code below assumes that GetCellsInUse provides a grid of Cell_t with ux/uy fields
 * where the layout matches how InterpolateVelocityX/Y expect to find values.
 * If your program actually stores u and v on different arrays (e.g. u is (nx+1,ny)), adapt indexing
 * accordingly.
 */

sim_err_t AdvectVelocity(SimState_t* state) {
    ASSERT_COMMON_NOT_NULL(state);

    double cell_size = (double)state->w;
    uint64_t nx = state->nx;
    uint64_t ny = state->ny;

    // physical domain extents (same convention as interpolation helpers)
    double width = (double)nx * cell_size;
    double height = (double)ny * cell_size;

    // new cell grid to store advected velocities
    Cell_t** new_cells = GetCellNextInUse(state);

    // Iterate over cells — advect U and V separately using appropriate sample points
    for (uint64_t i = 0; i < nx; i++) {
        for (uint64_t j = 0; j < ny; j++) {

            // -------------------------
            // Advect horizontal (U) at left-edge center:
            // left-edge X coordinate = (i * cell_size) + (-width/2)
            // but to place the edge at the left of cell i we consider:
            // worldX_u = -width/2 + i * cell_size;   // left edge of cell i
            // worldY_u = -height/2 + (j + 0.5) * cell_size; // center vertically
            // -------------------------
            double world_u_x = -width * 0.5 + (double)i * cell_size;
            double world_u_y = -height * 0.5 + ((double)j + 0.5) * cell_size;

            // sample velocity at the edge position (bilinear from current state)
            double u_vel_x =
                InterpolateVelocityX(state, world_u_x, world_u_y); // x-component at this edge
            double u_vel_y = InterpolateVelocityY(
                state, world_u_x, world_u_y); // y-component at this edge (needed for backtrace)

            // backtrace: where did the fluid at this edge come from?
            double prev_u_world_x = world_u_x - u_vel_x * state->dt;
            double prev_u_world_y = world_u_y - u_vel_y * state->dt;

            // sample the X-component from the previous location and store into the corresponding
            // place. We write the sampled U into the cell's ux. Depending on your storage
            // convention you might instead need to store into an edge-array; this keeps the
            // original Cell_t layout. If face is outside domain due to backtrace, Interpolate
            // functions clamp.
            new_cells[i][j].ux = InterpolateVelocityX(state, prev_u_world_x, prev_u_world_y);

            // -------------------------
            // Advect vertical (V) at bottom-edge center:
            // worldX_v = -width/2 + (i + 0.5) * cell_size; // center horizontally
            // worldY_v = -height/2 + j * cell_size;        // bottom edge of cell j
            // -------------------------
            double world_v_x = -width * 0.5 + ((double)i + 0.5) * cell_size;
            double world_v_y = -height * 0.5 + (double)j * cell_size;

            double v_vel_x = InterpolateVelocityX(state, world_v_x, world_v_y);
            double v_vel_y = InterpolateVelocityY(state, world_v_x, world_v_y);

            double prev_v_world_x = world_v_x - v_vel_x * state->dt;
            double prev_v_world_y = world_v_y - v_vel_y * state->dt;

            new_cells[i][j].uy = InterpolateVelocityY(state, prev_v_world_x, prev_v_world_y);

            // NOTE: If some cells are solid and you want to preserve boundary behavior,
            // copy velocities directly for those cells here (or zero them). Example:
            // if (CellIsSolid(state, i, j)) { new_cells[i][j].ux = 0.0; new_cells[i][j].uy = 0.0; }
            // But since we don't know your exact solid-check API in this snippet, leave that to
            // you.
        }
    }

    // swap the buffers used by your simulation (existing API call)
    Sim_State_SwapCellsInUse(state);
    return SIM_SUCCESS;
}
