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
        time += 0.016;
        
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                int p_idx = j * nx + i;
                double x = (double)i / nx;
                double y = (double)j / ny;
                pressure[p_idx] = 1.0 + 0.5 * sin(2.0 * M_PI * (x + y) + time);
            }
        }
        
        for (int i = 0; i <= nx; i++) {
            for (int j = 0; j < ny; j++) {
                int ux_idx = i * ny + j;
                double x = (double)i / nx;
                double y = (double)(j + 0.5) / ny;
                double cx = 0.5, cy = 0.5;
                double dx = x - cx;
                double dy = y - cy;
                double dist = sqrt(dx*dx + dy*dy);
                double angle = atan2(dy, dx) + time;
                ux[ux_idx] = 0.3 * cos(angle) * (1.0 - dist);
            }
        }
        
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j <= ny; j++) {
                int uy_idx = i * (ny + 1) + j;
                double x = (double)(i + 0.5) / nx;
                double y = (double)j / ny;
                double cx = 0.5, cy = 0.5;
                double dx = x - cx;
                double dy = y - cy;
                double dist = sqrt(dx*dx + dy*dy);
                double angle = atan2(dy, dx) + time;
                uy[uy_idx] = 0.3 * sin(angle) * (1.0 - dist);
            }
        }
        
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
