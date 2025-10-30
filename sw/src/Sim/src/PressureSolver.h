#pragma once

#include "../inc/Sim.h"

sim_err_t PressureSolver(SimState_t* pState);

void PressureSolverCell(int y, int x);

double getPressure(int x, int y);
