#pragma once

#include "../inc/Sim.h"
#include "SimTypes.h"

sim_err_t ControllerInit(sim_params_t* pParams);
sim_err_t ControllerStop(void);
sim_err_t ControllerJoin(void);

void Sim_State_SwapCellsInUse(SimState_t* pState);
Cell_t** GetCellsInUse(SimState_t* state);

Cell_t** GetCellNextInUse(SimState_t* state);
