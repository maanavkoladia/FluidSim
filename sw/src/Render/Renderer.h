#pragma once
#include "../Sim/inc/Sim.h"

int init_opengl(void);
void cleanup_opengl(void);
void render_fluid(void);
int should_close(void);
void swap_buffers(void);
void poll_events(void);
void set_sim_state(sim_state_t* state);  // Add this
void render_grid(int nx, int ny);