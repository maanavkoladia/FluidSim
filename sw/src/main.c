#define GL_SILENCE_DEPRECATION
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
// #include "Render/inc/Helpers.h"
#include "Render/inc/Renderer.h"
#include "Sim/inc/Sim.h"
#include "Transform/inc/Transform.h"
#include "config.h"
#include <unistd.h>

#if defined(__APPLE__)
#    include <OpenGL/gl.h>
#elif defined(__linux__)
#    include <GL/gl.h>
#endif

#include <time.h>

#define SIM_RUNTIME_S (60)

sim_params_t simParams = {.nx = 64,
                          .ny = 48,
                          .dt = 0.0167,                // simulation timestep in seconds
                          .p_density = 1.0,            // fluid density
                          .w = 0.05,                   // some simulation weight parameter
                          .overrelaxation_const = 0.9, // typical SOR relaxation factor
                          .PSolver_Interations = 150,  // number of iterations for pressure solver
                          .runTime = {0, 0},           // initialize to 0
                          .advectionScheme = SEMI_LAGRANGIAN,
                          .PsolverScene = JACOBI};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    LOG("Fluid Sim Starting Up");

#if defined(linux)
#    ifdef RUN_RENDERER
    ASSERT_COMMON_POSIX(Render_Init(), "Failed to launch render service");
#    endif
#endif

    // needs to be inited firt, bc sim needs to push into the transform fifo, ie sim depedns on
    // the runritme of the transform task
#ifdef RUN_TRANSFORM
    ASSERT_COMMON_POSIX(Transform_Init(), "Failed to init Transform");
#endif

#ifdef RUN_SIM_ENGINE
    ASSERT_COMMON_POSIX(Sim_Init(&simParams), "Failed to Init Sim");
    ASSERT_COMMON_POSIX(Sim_Start(), "Failed to Start Sim");
#endif

#if defined(__APPLE__)
#    ifdef RUN_RENDERER
    Render_Init();
#    endif
#else
    FOR_LOOP_COMMON(i, SIM_RUNTIME_S) {
        sleep(1);
        LOG("Slept for %d sec", i);
    }
#endif

    LOG("Beggning tear down");

#ifdef RUN_RENDERER
    ASSERT_COMMON_POSIX(Render_Dtr(), "Failed to kill renderer");
#endif

#ifdef RUN_TRANSFORM
    ASSERT_COMMON_POSIX(Transform_Dtr(), "Failed to kill transform service");
#endif

#ifdef RUN_SIM_ENGINE
    ASSERT_COMMON_POSIX(Sim_Stop(), "Failed to Stop Sim");
#endif

    LOG("Program Exited");
    return EXIT_SUCCESS;
}
