#define GL_SILENCE_DEPRECATION
#include "../../../mpsLibC/common/Assert_Common.h"
#include "../../../mpsLibC/common/LOG.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "../inc/Renderer.h"
// ----------------- main -----------------
int main(void) {
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
        Render_Send_Frame(&frame);
        
        glClear(GL_COLOR_BUFFER_BIT);

        draw_pressure();
        draw_grid();
        draw_velocities();

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