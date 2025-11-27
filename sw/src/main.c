#define GL_SILENCE_DEPRECATION
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include "Render/inc/Renderer.h"
#include "Render/inc/Helpers.h"
#include "Sim/inc/Sim.h"
#include "Transform/inc/Transform.h"
#include "config.h"
#include <unistd.h>
#include <OpenGL/gl.h>
#include <time.h>

sim_params_t simParams = {.nx = 64,
                          .ny = 48,
                          .dt = 0.01,                  // simulation timestep in seconds
                          .p_density = 1.0,            // fluid density
                          .w = 0.1,                    // some simulation weight parameter
                          .overrelaxation_const = 1.7, // typical SOR relaxation factor
                          .PSolver_Interations = 50,   // number of iterations for pressure solver
                          .runTime = {0, 0},           // initialize to 0
                          .advectionScheme = SEMI_LAGRANGIAN,
                          .PsolverScene = JACOBI};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    LOG("Fluid Sim Starting Up");

#ifdef RUN_TRANSFORM
    ASSERT_COMMON_POSIX(Transform_Init(), "Failed to init Transform");
#endif

#ifdef RUN_RENDERER
    ASSERT_COMMON_POSIX(Render_Init(), "Failed to launch render service");
#endif

#ifdef RUN_SIM_ENGINE
    ASSERT_COMMON_POSIX(Sim_Init(&simParams), "Failed to Init Sim");
    ASSERT_COMMON_POSIX(Sim_Start(), "Failed to Start Sim");
#endif

#ifdef RUN_RENDERER
    while (!Render_ShouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT);
        
#ifdef DISPLAY_COLORS
        draw_frame_colors();  // Draw color frame (velocity as colors)
#else
        draw_pressure();      // Draw pressure colors
        draw_velocities();    // Draw velocity arrows
#endif
        draw_grid();          // Draw grid
        Render_SwapBuffers();
        Render_PollEvents();
    }
#endif

#ifdef RUN_SIM_ENGINE
    ASSERT_COMMON_POSIX(Sim_Stop(), "Failed to Stop Sim");
    Transform_SimEngine_WaitFor_Teardown();
#endif

#ifdef RUN_TRANSFORM
    ASSERT_COMMON_POSIX(Transform_Dtr(), "Failed to kill transform service");
#endif

#ifdef RUN_RENDERER
    ASSERT_COMMON_POSIX(Render_Dtr(), "Failed to kill renderer");
#endif

    LOG("Program Exited");
    return EXIT_SUCCESS;
}
