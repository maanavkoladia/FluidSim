#include "AtomicFlag.h"
#include "ForLoop.h"
#include "LOG.h"
#include "Sim/inc/Sim.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Assert_Common.h"
#include "Renderer.h"

#define WAIT_TIME_S (3)

sim_params_t simParams;

int main(int argc, char** argv) {
    LOG("Fluid Sim Starting Up");

    // Initialize OpenGL
    if (init_opengl() != 0) {
        LOG("Failed to initialize OpenGL");
        return -1;
    }

    simParams.density = 1000.0;
    simParams.viscosity = 0.0;
    simParams.gravity[0] = 0.0;
    simParams.gravity[1] = -9.81;
    simParams.timestep = 0.001;
    simParams.cfl_number = 0.5;
    simParams.grid_resolution[0] = 64;
    simParams.grid_resolution[1] = 64;
    simParams.domain_size[0] = 1.0;
    simParams.domain_size[1] = 1.0;
    simParams.initial_velocity[0] = 0.0;
    simParams.initial_velocity[1] = 0.0;
    simParams.initial_surface_height = 0.3;

    ASSERT_COMMON_POSIX(SimInit(&simParams), "Failed to Init Sim");
    ASSERT_COMMON_POSIX(SimStart(), "Failed to Start Sim");

    // Main render loop
    while (!should_close()) {
        render_fluid();
        swap_buffers();
        poll_events();
        
        // Small delay to prevent 100% CPU usage
        usleep(16000);
    }

    FOR_LOOP_COMMON(i, WAIT_TIME_S) {
        LOG("Slept for %d seconds", i);
        sleep(1);
    }
    ASSERT_COMMON_POSIX(SimStop(), "Failed to Stop Sim");
    cleanup_opengl();

    LOG("Program Exited");
    return EXIT_SUCCESS;
}