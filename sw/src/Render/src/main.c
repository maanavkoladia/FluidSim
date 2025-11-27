#define GL_SILENCE_DEPRECATION
#include "../../../mpsLibC/common/Assert_Common.h"
#include "../../../mpsLibC/common/LOG.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <OpenGL/gl.h>
#include "../inc/Renderer.h"
#include "../inc/Helpers.h"

int main(void) {
    LOG("Program Starting Up");
    ASSERT_COMMON_POSIX(Render_Init(), "Failed to initialize renderer");
    
#ifdef USE_COLOR_FRAME
    // Use Render_Frame_Colors_t
    int width = 1000, height = 1000;
    
    Color_t* colors = (Color_t*)malloc(sizeof(Color_t) * width * height);
    if (!colors) {
        LOG("Failed to allocate colors");
        Render_Dtr();
        return EXIT_FAILURE;
    }
    
    Render_Frame_Colors_t colorFrame = {
        .width = width,
        .height = height,
        .colors = colors
    };
    
    double time = 0.0;
    
    while (!Render_ShouldClose()) {
        time += 0.016;
        
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                int idx = y * width + x;
                double fx = (double)x / width;
                double fy = (double)y / height;
                
                double r_val = 0.5 + 0.5 * sin(2.0 * M_PI * (fx + fy) + time);
                double g_val = 0.5 + 0.5 * cos(2.0 * M_PI * (fx - fy) + time * 0.7);
                
                double cx = 0.5, cy = 0.5;
                double dx = fx - cx;
                double dy = fy - cy;
                double dist = sqrt(dx*dx + dy*dy);
                double angle = atan2(dy, dx) + time;
                double b_val = 0.5 + 0.5 * sin(angle + dist * 4.0);
                
                if (r_val < 0.0) r_val = 0.0;
                if (r_val > 1.0) r_val = 1.0;
                if (g_val < 0.0) g_val = 0.0;
                if (g_val > 1.0) g_val = 1.0;
                if (b_val < 0.0) b_val = 0.0;
                if (b_val > 1.0) b_val = 1.0;
                
                colors[idx].r = (float)r_val;
                colors[idx].g = (float)g_val;
                colors[idx].b = (float)b_val;
                colors[idx].a = 1.0f;
            }
        }
        
        Render_Send_Frame_Colors(&colorFrame);
        glClear(GL_COLOR_BUFFER_BIT);
        draw_frame_colors();
        Render_SwapBuffers();
        Render_PollEvents();
    }
    
    free(colors);
#else
    // Use Render_Frame_t with pressure and velocities
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
                pressure[p_idx] = 1.0 + 0.5 * sin(2.0 * M_PI * (x + y) + time) * 
                                        cos(2.0 * M_PI * (x - y) + time * 0.7);
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
                ux[ux_idx] = 0.3 * cos(angle) * (1.0 - dist) * exp(-dist * 2.0);
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
                uy[uy_idx] = 0.3 * sin(angle) * (1.0 - dist) * exp(-dist * 2.0);
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
#endif
    
    Render_Dtr();
    LOG("Program Exiting");
    return EXIT_SUCCESS;
}
