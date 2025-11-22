#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include "LOG.h"
#include "Render/inc/Renderer.h"
#include "Sim/inc/Sim.h"
#include "Transform/inc/Transform.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WAIT_TIME_S (3)

sim_params_t simParams;

int main(int argc, char** argv) {
    LOG("Fluid Sim Starting Up");

    ASSERT_COMMON_POSIX(Transform_Init(), "Faield to init Xform");
    ASSERT_COMMON_POSIX(Render_Init(), "Failed to launch render service");
    ASSERT_COMMON_POSIX(Sim_Init(&simParams), "Failed to Init Sim");
    ASSERT_COMMON_POSIX(Sim_Start(), "Failed to Start Sim");
    FOR_LOOP_COMMON(i, WAIT_TIME_S) {
        LOG("Slept for %d seconds", i + 1);
        sleep(1);
    }

    while (!Render_ShouldClose()) {
        
        Render_SwapBuffers();
        Render_PollEvents();
    }

    ASSERT_COMMON_POSIX(Sim_Stop(), "Failed to Stop Sim");
    ASSERT_COMMON_POSIX(Transform_Dtr(), "Failed to kill tranform service");
    ASSERT_COMMON_POSIX(Render_Dtr(), "Failed to kill renderer");

    LOG("Program Exited");
    return EXIT_SUCCESS;
}
