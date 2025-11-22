#pragma once

#include "../inc/Sim.h"
#include <stdbool.h>
#include "SimTypes.h"


sim_err_t PressureSolver(SimState_t* sim_state);

void PressureSolverCell(uint x, uint y, SimState_t* g_sim_state,Cell_t** current_cells , Cell_t** cell_buffer);

sim_err_t UpdateVelocities(Cell_t** cell_buffer, uint nx,uint ny, uint k);

SimSnap_t* CreateCellsBuffer(uint nx, uint ny);


static inline double GetPressure(Cell_t** cells,uint x, uint y);
static inline double GetVelocityX(Cell_t** cells,uint x, uint y);
static inline double GetVelocityY(Cell_t** cells,uint x, uint y); 
static inline bool IsSolid(Cell_t** cells,uint x, uint y);