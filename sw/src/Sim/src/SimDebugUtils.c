#include "SimDebugUtils.h"
#include "Assert_Common.h"
#include <inttypes.h> // for PRIu64
#include <stdio.h>

// Helper: Convert Advection_Scheme_t enum to string
static const char* advectionSchemeToStr(Advection_Scheme_t scheme) {
    switch (scheme) {
    case SEMI_LAGRANGIAN:
        return "SEMI_LAGRANGIAN";
    default:
        return "UNKNOWN_ADVECTION_SCHEME";
    }
}

// Helper: Convert PressureSolver_Scheme_t enum to string
static const char* pressureSolverToStr(PressureSolver_Scheme_t solver) {
    switch (solver) {
    case GAUSS_SEIDEL:
        return "GAUSS_SEIDEL";
    case JACOBI:
        return "JACOBI";
    case RED_BLACK_GAUSS_SEIDEL:
        return "RED_BLACK_GAUSS_SEIDEL";
    default:
        return "UNKNOWN_PRESSURE_SOLVER";
    }
}

// Main print function
void PrintSimParams(const sim_params_t* params) {
    ASSERT_COMMON(params, "Got a NULL Params");
    printf("\n========== Simulation Parameters ==========\n");
    printf("Grid Size (nx, ny):        %" PRIu64 " x %" PRIu64 "\n", params->nx, params->ny);
    printf("Time Step (dt):            %.6f s\n", params->dt);
    printf("Cell Width (w):            %.6f m\n", params->w);
    printf("Advection Scheme:          %s\n", advectionSchemeToStr(params->advectionScheme));
    printf("Pressure Solver Scheme:    %s\n", pressureSolverToStr(params->PsolverScene));
    printf("Run Time:                  %ld.%09ld s\n", (long)params->runTime.tv_sec,
           params->runTime.tv_nsec);
    printf("Particle Density:          %.6f kg/m³\n", params->p_density);
    printf("==========================================\n\n");
}
