#define GL_SILENCE_DEPRECATION
#include "../../../mpsLibC/common/Assert_Common.h"
#include "../../../mpsLibC/common/LOG.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <OpenGL/gl.h>
#include "../inc/Renderer.h"

int main(void) {
    LOG("Program Starting Up");
    ASSERT_COMMON_POSIX(Render_Init(), "Failed to initialize renderer");
    
    int nx = 5, ny = 5;
    
    double* ux = (double*)malloc(sizeof(double) * (nx + 1) * ny);
    double* uy = (double*)malloc(sizeof(double) * nx * (ny + 1));
    double* pressure = (double*)malloc(sizeof(double) * nx * ny);
    
    Render_Frame_t frame = {
        .pressure = pressure,
        .nx = nx,
        .ny = ny,
        .ux = ux,
        .uy = uy
    };
    
    double time = 0.0;
    
    while (!Render_ShouldClose()) {
        
        Render_SwapBuffers();
        Render_PollEvents();
    }

    free(ux);
    free(uy);
    free(pressure);
    Render_Dtr();
    
    LOG("Program Exiting");
    return EXIT_SUCCESS;
}
