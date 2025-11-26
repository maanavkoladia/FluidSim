#pragma once

#include "../inc/Sim.h"
#include <stdbool.h>
#include "SimTypes.h"


sim_err_t PressureSolver(SimState_t* sim_state);


void PressureSolverCell(uint64_t x, uint64_t y, SimState_t* g_sim_state,Cell_t** current_cells , Cell_t** cell_buffer);

sim_err_t UpdateVelocities(Cell_t** cell_buffer, uint64_t nx,uint64_t ny, uint64_t k);


static inline double GetPressure(Cell_t** cells,uint64_t x, uint64_t y);
static inline double GetVelocityX(Cell_t** cells,uint64_t x, uint64_t y);
static inline double GetVelocityY(Cell_t** cells,uint64_t x, uint64_t y); 
static inline bool IsSolid(Cell_t** cells,uint64_t x, uint64_t y);