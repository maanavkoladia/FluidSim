#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include "LOG.h"
#include "Render/inc/Renderer.h"
#include "Sim/inc/Sim.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// #include "Renderer.h"

#define WAIT_TIME_S (3)

sim_params_t simParams;

int main(int argc, char** argv) {
    LOG("Fluid Sim Starting Up");

    ASSERT_COMMON_POSIX(Sim_Init(&simParams), "Failed to Init Sim");
    ASSERT_COMMON_POSIX(Sim_Start(), "Failed to Start Sim");

    FOR_LOOP_COMMON(i, WAIT_TIME_S) {
        LOG("Slept for %d seconds", i);
        sleep(1);
    }
    ASSERT_COMMON_POSIX(Sim_Stop(), "Failed to Stop Sim");

    LOG("Program Exited");
    return EXIT_SUCCESS;
}
