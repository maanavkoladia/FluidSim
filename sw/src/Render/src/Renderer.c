#define GL_SILENCE_DEPRECATION
#include "../inc/Renderer.h"
#include "../../../mpsLibC/common/Assert_Common.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <OpenGL/gl.h> 
#include <stdlib.h>

static Render_Frame_t* gCurrentFrame = NULL;

#define HEIGHT 480
#define WIDTH  640

static GLFWwindow* gWindow = NULL;
static int gWinW = WIDTH, gWinH = HEIGHT;

#define GRID_NX 10
#define GRID_NY 10

void draw_grid(void) {
    int nx = 5, ny = 5;
    
    if (gCurrentFrame) {
        nx = gCurrentFrame->nx;
        ny = gCurrentFrame->ny;
    }
    
    glColor3f(0.5f, 0.5f, 0.5f);
    
    glBegin(GL_LINES);
    
    for (int i = 0; i <= nx; i++) {
        float x = -1.0f + (2.0f * i / nx);
        glVertex2f(x, -1.0f);
        glVertex2f(x,  1.0f);
    }
    
    for (int j = 0; j <= ny; j++) {
        float y = -1.0f + (2.0f * j / ny);
        glVertex2f(-1.0f, y);
        glVertex2f( 1.0f, y);
    }
    
    glEnd();
}

void draw_velocities(void) {
    if (!gCurrentFrame || !gCurrentFrame->ux || !gCurrentFrame->uy) {
        return;
    }

    int nx = gCurrentFrame->nx;
    int ny = gCurrentFrame->ny;
    float arrow_scale = 0.15f;
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    
    glBegin(GL_LINES);
    
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int ux_idx = (i + 1) * ny + j;
            if (ux_idx < (nx + 1) * ny) {
                float ux_val = (float)gCurrentFrame->ux[ux_idx];
                
                float x = -1.0f + (2.0f * i / nx);
                float y = -1.0f + (2.0f * (j + 0.5f) / ny);
                
                glVertex2f(x, y);
                glVertex2f(x + ux_val * arrow_scale, y);
            }
        }
    }
    
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int uy_idx = i * (ny + 1) + j;
            if (uy_idx < nx * (ny + 1)) {
                float uy_val = (float)gCurrentFrame->uy[uy_idx];
                
                float x = -1.0f + (2.0f * (i + 0.5f) / nx);
                float y = -1.0f + (2.0f * j / ny);
                
                glVertex2f(x, y);
                glVertex2f(x, y + uy_val * arrow_scale);
            }
        }
    }
    
    glEnd();
    glLineWidth(1.0f);
}

void draw_pressure(void) {
    if (!gCurrentFrame || !gCurrentFrame->pressure) {
        return;
    }

    int nx = gCurrentFrame->nx;
    int ny = gCurrentFrame->ny;
    
    double pMin = gCurrentFrame->pressure[0];
    double pMax = gCurrentFrame->pressure[0];
    for (int i = 0; i < nx * ny; i++) {
        if (gCurrentFrame->pressure[i] < pMin) pMin = gCurrentFrame->pressure[i];
        if (gCurrentFrame->pressure[i] > pMax) pMax = gCurrentFrame->pressure[i];
    }
    
    double pRange = pMax - pMin;
    if (pRange < 1e-10) pRange = 1.0;
    
    glBegin(GL_QUADS);
    
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int p_idx = j * nx + i;
            double p = gCurrentFrame->pressure[p_idx];
            
            double t = (p - pMin) / pRange;
            if (t < 0.0) t = 0.0;
            if (t > 1.0) t = 1.0;
            
            float r = (float)t;
            float g = 0.0f;
            float b = 1.0f - (float)t;
            
            glColor3f(r, g, b);
            
            float x0 = -1.0f + (2.0f * i / nx);
            float x1 = -1.0f + (2.0f * (i + 1) / nx);
            float y0 = -1.0f + (2.0f * j / ny);
            float y1 = -1.0f + (2.0f * (j + 1) / ny);
            
            glVertex2f(x0, y0);
            glVertex2f(x1, y0);
            glVertex2f(x1, y1);
            glVertex2f(x0, y1);
        }
    }
    
    glEnd();
}

render_err_t Render_Send_Frame(Render_Frame_t* pFrameIn) {
    gCurrentFrame = pFrameIn;
    return RENDER_SUCCESS;
}

// ----------------- callbacks -----------------

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    gWinW = (width  > 0) ? width  : 1;
    gWinH = (height > 0) ? height : 1;
    glViewport(0, 0, gWinW, gWinH);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, 1);
    }
}

static void KillGLFWProg(void* pvArgs) {
    char* msg = (char*)pvArgs;
    LOG("MSG: %s", msg);
    glfwTerminate();
}


render_err_t Render_Init(void) {
    LOG("Render Starting Up");
    if (!glfwInit()) {
        LOG("Failed to init glfw");
        return RENDER_FAIL;
    }

    gWindow = glfwCreateWindow(WIDTH, HEIGHT, "Fluid Sim", NULL, NULL);
    if (!gWindow) {
        KillGLFWProg("Window creation failed");
        return RENDER_FAIL;
    }
    
    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetKeyCallback(gWindow, key_callback);

    framebuffer_size_callback(gWindow, WIDTH, HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    
    return RENDER_SUCCESS;
}

int Render_ShouldClose(void) {
    return gWindow ? glfwWindowShouldClose(gWindow) : 1;
}

void Render_SwapBuffers(void) {
    if (gWindow) glfwSwapBuffers(gWindow);
}

void Render_PollEvents(void) {
    glfwPollEvents();
}

GLFWwindow* Render_GetWindow(void) {
    return gWindow;
}

render_err_t Render_Dtr(void) {
    if (gWindow) {
        glfwDestroyWindow(gWindow);
        gWindow = NULL;
    }
    glfwTerminate();
    return RENDER_SUCCESS;
}