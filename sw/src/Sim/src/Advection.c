#include "Advection.h"
#include "Assert_Common.h"
#include "Controller.h"
#include "ForLoop.h"
#include "LOG.h"
#include "SimTypes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Clamp a value between min and max
double clamp(double value, double minVal, double maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// Clamp a value between 0 and 1
double clamp01(double value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

// Linear interpolation: mix a and b by t
double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

double InterpolateVelocityX(SimState_t* sim_state, double world_pos_x, double world_pos_y) {
    ASSERT_COMMON_NOT_NULL(sim_state);
    uint64_t nx = sim_state->nx;
    uint64_t ny = sim_state->ny;
    uint64_t cell_size = sim_state->w;

    uint64_t width = (nx - 1) * cell_size;
    uint64_t height = (ny - 1) * cell_size;

    double posx = ((world_pos_x + width) / 2) * cell_size;
    double posy = ((world_pos_y + height) / 2) * cell_size;

    uint64_t left = clamp(posx, 0, nx - 2);
    uint64_t bottom = clamp(posy, 0, ny - 2);
    uint64_t right = left + 1;
    uint64_t top = bottom + 1;

    double x_fraction = clamp01(posx - left);
    double y_fraction = clamp01(posy - bottom);

    Cell_t** cells = GetCellsInUse(sim_state);

    double top_valueX = lerp(cells[left][top].ux, cells[right][top].ux, x_fraction);
    double bottom_valueX = lerp(cells[left][bottom].ux, cells[right][bottom].ux, x_fraction);
    double newX_velocity = lerp(bottom_valueX, top_valueX, y_fraction);

    return newX_velocity;
}

double InterpolateVelocityY(SimState_t* sim_state, double world_pos_x, double world_pos_y) {
    uint64_t nx = sim_state->nx;
    uint64_t ny = sim_state->ny;
    uint64_t cell_size = sim_state->w;

    uint64_t width = (nx - 1) * cell_size;
    uint64_t height = (ny - 1) * cell_size;

    double posx = ((world_pos_x + width) / 2) * cell_size;
    double posy = ((world_pos_y + height) / 2) * cell_size;

    uint64_t left = clamp(posx, 0, nx - 2);
    uint64_t bottom = clamp(posy, 0, ny - 2);
    uint64_t right = left + 1;
    uint64_t top = bottom + 1;

    double x_fraction = clamp01(posx - left);
    double y_fraction = clamp01(posy - bottom);

    Cell_t** cells = GetCellsInUse(sim_state);

    double top_valueY = lerp(cells[left][top].uy, cells[right][top].uy, x_fraction);
    double bottom_valueY = lerp(cells[left][bottom].uy, cells[right][bottom].uy, x_fraction);
    double newX_velocitY = lerp(bottom_valueY, top_valueY, y_fraction);

    return newX_velocitY;
}

static bool IsSolid(Cell_t* pCell) {
    ASSERT_COMMON_NOT_NULL(pCell);
    return pCell->fluidNeighbors == SOLID;
}

static Cell_t* GetCell(SimState_t* pSimState, uint64_t x, uint64_t y) {
    ASSERT_COMMON_NOT_NULL(pSimState);
    ASSERT_COMMON(x < pSimState->nx, "Got invliad X");
    ASSERT_COMMON(y < pSimState->ny, "Got invliad X");
    Cell_t** cells = GetCellsInUse(pSimState);
    return &cells[x][y];
}

static double GetLeftCellCenter(SimState_t* pState, uint64_t x, uint64_t y) {
    return pState->w * (x);
}

static double GetBottomEdgeCentre(SimState_t* pState, uint64_t x, uint64_t y) {
    return pState->w * (y);
}

static void AdvectX(SimState_t* pState) {
}

sim_err_t AdvectVelocity(SimState_t* state) {
    double cell_width = (state->nx - 1) * state->w;
    double cell_height = (state->ny - 1) * state->w;
    double cell_size = state->w;
    Cell_t** new_cells = GetCellNextInUse(state);
    Cell_t** pCurrCell = GetCellsInUse(state);
    for (uint64_t i = 0; i < state->nx; i++) {
        for (uint64_t j = 0; j < state->ny; j++) {
            // if (IsSolid(GetCell(state, i, j)) || IsSolid(GetCell(state, i - 1, j))) {
            //     new_cells[i][j] = pCurrCell[i][j];
            // }
            //  get world position x velocity
            double horizontal_worldX = cell_width / 2 + i * cell_size;
            double horizontal_worldY = cell_height / 2 + j * cell_size + cell_size / 2;

            // Biliner Interpol x velocity
            double horizontal_velX =
                InterpolateVelocityX(state, horizontal_worldX, horizontal_worldY);
            double horizontal_velY =
                InterpolateVelocityY(state, horizontal_worldX, horizontal_worldY);
            // calculate where velociy came from
            double prev_horizontal_worldX = horizontal_worldX - horizontal_velX * state->dt;
            double prev_horizontal_worldY = horizontal_worldY - horizontal_velY * state->dt;
            // Biliner Interpol the previous velocity as the new velocity
            new_cells[i][j].ux =
                InterpolateVelocityX(state, prev_horizontal_worldX, prev_horizontal_worldY);

            // get world position y velocity
            double vertical_worldX = cell_width / 2 + i * cell_size + cell_size / 2;
            double vertical_worldY = cell_height / 2 + j * cell_size;
            // Biliner Interpol y
            double vertical_velX = InterpolateVelocityX(state, vertical_worldX, vertical_worldY);
            double vertical_velY = InterpolateVelocityY(state, vertical_worldX, vertical_worldY);
            // calculate prev
            double prev_vertical_worldX = vertical_worldX - vertical_velX * state->dt;
            double prev_vertical_worldY = vertical_worldY - vertical_velY * state->dt;
            // Biliner interpol prev velocity as new
            new_cells[i][j].uy =
                InterpolateVelocityY(state, prev_vertical_worldX, prev_vertical_worldY);
        }
    }
    Sim_State_SwapCellsInUse(state);
    return SIM_SUCCESS;
}

// }

// void advenction(SimState_t* sim){

//     // int xtemp[sim->nx][sim->ny];

//     // int ytemp[sim->nx][sim->ny];

//     // for(int i = 0; i < sim->nx; i++){
//     //     for(int j = 0; j < sim->ny; j++){

//     //         int xposition = i * sim->w;
//     //         int yposition = j * sim->w;
//     //         int pos = xposition +yposition;

//     //         int xvel = sim->cells[i][j].u;
//     //         int yvel = sim->cells[i][j].v;
//     //         int vel = xvel + yvel;

//     //         int prevPos = pos - vel * sim->dt;

//     //     }
//     // }
// }
