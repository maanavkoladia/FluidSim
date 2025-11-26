#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include "Render/inc/Renderer.h"
#include "Sim/inc/Sim.h"
#include "Transform/inc/Transform.h"
#include "config.h"
#include <unistd.h>

#define WAIT_TIME_S (3)

sim_params_t simParams;

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    LOG("Fluid Sim Starting Up");

#ifdef RUN_TRANSFORM
    ASSERT_COMMON_POSIX(Transform_Init(), "Faield to init Xform");
#endif

#ifdef RUN_RENDERER
    ASSERT_COMMON_POSIX(Render_Init(), "Failed to launch render service");
#endif

#ifdef RUN_SIM_ENGINE
    ASSERT_COMMON_POSIX(Sim_Init(&simParams), "Failed to Init Sim");
    ASSERT_COMMON_POSIX(Sim_Start(), "Failed to Start Sim");
#endif

    FOR_LOOP_COMMON(i, WAIT_TIME_S) {
        LOG("Slept for %d seconds", i + 1);
        sleep(1);
    }

#ifdef RUN_SIM_ENGINE
    ASSERT_COMMON_POSIX(Sim_Stop(), "Failed to Stop Sim");
#endif
#ifdef RUN_TRANSFORM
    ASSERT_COMMON_POSIX(Transform_Dtr(), "Failed to kill tranform service");
#endif
#ifdef RUN_RENDERER
    ASSERT_COMMON_POSIX(Render_Dtr(), "Failed to kill renderer");
#endif

    LOG("Program Exited");
    return EXIT_SUCCESS;
}
