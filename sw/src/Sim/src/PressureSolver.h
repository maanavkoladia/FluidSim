#pragma once

#include "../inc/Sim.h"

sim_err_t PressureSolver(SimState_t* sim_state);

void PressureSolverCell(uint x, uint y, SimState_t* g_sim_state, Cell_t** cell_buffer);

double GetPressure(int x, int y);

sim_err_t UpdateVelocities(Cell_t** cell_buffer, uint nx,uint ny, uint k);

SimSnap_t* CreateCellsBuffer(uint nx, uint ny);


static inline double GetPressure(uint x, uint y);
static inline double GetVelocityX(uint x, uint y);
static inline double GetVelocityY(uint x, uint y); 
static inline bool IsSolid(uint x, uint y);
