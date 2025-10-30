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