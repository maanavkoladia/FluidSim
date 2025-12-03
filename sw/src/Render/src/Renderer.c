#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#define GL_SILENCE_DEPRECATION
#include "../../../mpsLibC/common/Assert_Common.h"
// #include "../../Transform/inc/Transform.h"
//  #include "../inc/Helpers.h"
#include "../../config.h"
#include "../inc/Renderer.h"
#include "AtomicFlag.h"
#include "LFfifo.h"
#include <GLFW/glfw3.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>

#if defined(__APPLE__)
#    include <OpenGL/gl.h>
#elif defined(__linux__)
#    include <GL/gl.h>
#endif

#define WIDTH (RENDER_WINDOW_WIDTH)
#define HEIGHT (RENDER_WINDOW_HEIGHT)

static AtomicFlag_t killFlag;

extern void TransForm_RawFrameYeild(Render_Frame_t* pFrame);
extern void TransForm_ColorFrameYeild(Render_Frame_Colors_t* pFrame);

// static Render_Frame_t* gCurrentFrame = NULL;
// static Render_Frame_Colors_t* gCurrentFrameColors = NULL;

static GLFWwindow* gWindow = NULL;
static int gWinW = WIDTH, gWinH = HEIGHT;

static pthread_t renderer_main_th;

static LF_Fifo_t* frameInFifo = NULL;

// -----------------------------------------------------------------------------
// Grid
// -----------------------------------------------------------------------------
static void draw_grid(Render_Frame_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame);
    int nx = pFrame->nx;
    int ny = pFrame->ny;

    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);

    for (int i = 0; i <= nx; i++) {
        float x = -1.0f + (2.0f * i / nx);
        glVertex2f(x, -1.0f);
        glVertex2f(x, 1.0f);
    }

    for (int j = 0; j <= ny; j++) {
        float y = -1.0f + (2.0f * j / ny);
        glVertex2f(-1.0f, y);
        glVertex2f(1.0f, y);
    }

    glEnd();
}

// -----------------------------------------------------------------------------
// Velocity Arrows
// -----------------------------------------------------------------------------
static void draw_velocities(Render_Frame_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->ux && pFrame->uy);

    int nx = pFrame->nx;
    int ny = pFrame->ny;

    float arrow_scale = 1.0f; // increased for visibility

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);

    glBegin(GL_LINES);

    // ---- Ux ----
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int idx = j * nx + i; // row-major again

            float ux = (float)pFrame->ux[idx];

            float x = -1.0f + (2.0f * i / nx);
            float y = -1.0f + (2.0f * (j + 0.5f) / ny);

            glVertex2f(x, y);
            glVertex2f(x + ux * arrow_scale, y);
        }
    }

    // ---- Uy ----
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            int idx = j * nx + i;

            float uy = (float)pFrame->uy[idx];

            float x = -1.0f + (2.0f * (i + 0.5f) / nx);
            float y = -1.0f + (2.0f * j / ny);

            
            LOG("Sample velocity: ux=%.2f uy=%.2f", pFrame->ux[idx], pFrame->uy[idx]);

            glVertex2f(x, y);
            glVertex2f(x, y + uy * arrow_scale);
        }
    }

    glEnd();
    glLineWidth(1.0f);
}

// -----------------------------------------------------------------------------
// Pressure Field
// -----------------------------------------------------------------------------
static void draw_pressure(Render_Frame_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->pressure);

    int nx = pFrame->nx;
    int ny = pFrame->ny;
    

    double pMin = pFrame->pressure[0];
    double pMax = pMin;

    /* --- Compute min/max, ignoring exact zero if you want solid walls to be black --- */
    for (int i = 1; i < nx * ny; i++) {
        double p = pFrame->pressure[i];
        if (p < pMin) pMin = p;
        if (p > pMax) pMax = p;
    }

    double pRange = (pMax - pMin) < 1e-10 ? 1.0 : (pMax - pMin);

    glBegin(GL_QUADS);

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {

            int idx = j * nx + i;
            double p = pFrame->pressure[idx];

            float r, g, b;

            if (p == 0.0) {
                /* --- ZERO PRESSURE → BLACK --- */
                r = g = b = 0.0f;

            } else {
                /* --- NORMALIZED COLOR MAP (red to blue) --- */
                double t = (p - pMin) / pRange;
                if (t < 0) t = 0;
                if (t > 1) t = 1;

                r = (float)t;
                g = 0.0f;
                b = 1.0f - (float)t;
            }

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

// -----------------------------------------------------------------------------
// Color Frame
// -----------------------------------------------------------------------------

void draw_frame_colors(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->colors);

    int width = pFrame->width;
    int height = pFrame->height;

    glBegin(GL_QUADS);

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {

            int idx = y * width + x;
            Color_t* c = &pFrame->colors[idx];
            glColor3f(c->r, c->g, c->b);

            float x0 = -1.0f + (2.0f * x / width);
            float x1 = -1.0f + (2.0f * (x + 1) / width);
            float y0 = -1.0f + (2.0f * y / height);
            float y1 = -1.0f + (2.0f * (y + 1) / height);

            glVertex2f(x0, y0);
            glVertex2f(x1, y0);
            glVertex2f(x1, y1);
            glVertex2f(x0, y1);
        }
    }

    glEnd();
}

// -----------------------------------------------------------------------------
// PUBLIC DRAW ENTRYPOINT (WITH IFDEF SWITCH)
// -----------------------------------------------------------------------------

static void Render_ServeRawFrame(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    Render_Frame_t* pFrame = NULL;

    while (LF_Fifo_SpinPop(frameInFifo, &pFrame) == LF_FIFO_FAIL_TRY_POP) {
        sched_yield();
    }

    ASSERT_COMMON_NOT_NULL(pFrame);
    draw_pressure(pFrame);
    draw_velocities(pFrame);
    draw_grid(pFrame);
    TransForm_RawFrameYeild(pFrame);
    // LOG("One server run");
}

static void Render_ServeColorFrame(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    Render_Frame_Colors_t* pFrame = NULL;

    while (LF_Fifo_SpinPop(frameInFifo, &pFrame) == LF_FIFO_FAIL_TRY_POP) {
        sched_yield();
    }

    ASSERT_COMMON_NOT_NULL(pFrame);
    draw_frame_colors(pFrame);
    TransForm_ColorFrameYeild(pFrame);
    // LOG("One server run");
}

static void Render_Draw(void) {

    // #ifdef DISPLAY_COLORS
    Render_ServeColorFrame();
    // #else
    // Render_ServeRawFrame();
    // #endif
}

// -----------------------------------------------------------------------------
// Frame / Init / Window Management
// -----------------------------------------------------------------------------
//

render_err_t Render_Send_Frame(Render_Frame_t* pFrameIn) {
    ASSERT_COMMON_NOT_NULL(pFrameIn);
    // LOG("Rxed a raw frame");
    // if (!pFrameIn) return RENDER_FAIL;
    // ASSERT_COMMON_POSIX(LF_Fifo_SpinPush(frameInFifo, pFrameIn), "Fialed to push from into
    // fifo");
    while (LF_Fifo_TryPush(frameInFifo, pFrameIn) == LF_FIFO_FAIL_TRY_PUSH) {
        sched_yield();
    } // gCurrentFrame = pFrameIn;
    return RENDER_SUCCESS;
}

render_err_t Render_Send_Frame_Colors(Render_Frame_Colors_t* pFrameIn) {
    ASSERT_COMMON_NOT_NULL(pFrameIn && pFrameIn->colors);
    ASSERT_COMMON(pFrameIn->height == RENDER_WINDOW_HEIGHT, "Hieght deosnt match");
    ASSERT_COMMON(pFrameIn->width == RENDER_WINDOW_WIDTH, "width deosnt match");
    // if (!pFrameIn || !pFrameIn->colors) return RENDER_FAIL;
    // if (pFrameIn->width <= 0 || pFrameIn->height <= 0) return RENDER_FAIL;

    // if (gCurrentFrameColors) TransForm_ColorFrameYeild(gCurrentFrameColors);
    while (LF_Fifo_TryPush(frameInFifo, pFrameIn) == LF_FIFO_FAIL_TRY_PUSH) {
        sched_yield();
    } // gCurrentFrame = pFrameIn;
    return RENDER_SUCCESS;
}

static int Render_ShouldClose(void) {

    return gWindow ? glfwWindowShouldClose(gWindow) : EXIT_FAILURE;
}

static void Render_SwapBuffers(void) {
    // LOG("Swap Called");
    ASSERT_COMMON_NOT_NULL(gWindow);
    glfwSwapBuffers(gWindow);
}

static void Render_PollEvents(void) {
    glfwPollEvents();
}

static void framebuffer_size_callback(GLFWwindow* window, int w, int h) {
    gWinW = w > 0 ? w : 1;
    gWinH = h > 0 ? h : 1;
    glViewport(0, 0, gWinW, gWinH);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, 1);
}

static void* Task_Renderer(void* pvArgs) {
    if (!glfwInit()) return NULL;

    gWindow = glfwCreateWindow(WIDTH, HEIGHT, "Fluid Sim", NULL, NULL);
    ASSERT_COMMON_NOT_NULL(gWindow);

    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetKeyCallback(gWindow, key_callback);

    framebuffer_size_callback(gWindow, WIDTH, HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    LOG("Render Loop started");
    while (!Render_ShouldClose()) {
        if (AtomicFlag_GetStatus(&killFlag) == FLAG_SET) {
            return NULL;
        }
        usleep(50000);
        Render_Draw();


        Render_SwapBuffers();
        Render_PollEvents();
    }

    return NULL;
}

GLFWwindow* Render_GetWindow(void) {
    return gWindow;
}

#if defined(__APPLE__)
render_err_t Render_Init(void) {
    LOG("Render Starting Up");
    AtomicFlag_Clear(&killFlag);
    // create the sim snap fifo
    LF_Fifo_Init(&frameInFifo, FRAME_IN_FIFO_SIZE);
    Task_Renderer(NULL);
    // LOG("Renderer Init Success");
    return RENDER_SUCCESS;
}
 
#else
render_err_t Render_Init(void) {
    LOG("Render Starting Up");
    AtomicFlag_Clear(&killFlag);
    // create the sim snap fifo
    LF_Fifo_Init(&frameInFifo, FRAME_IN_FIFO_SIZE);

    ASSERT_COMMON_POSIX(pthread_create(&renderer_main_th, NULL, Task_Renderer, NULL),
                        "Faield to inti the redernder thread");
    LOG("Renderer Init Success");
    return RENDER_SUCCESS;
}

#endif

render_err_t Render_Dtr(void) {
    AtomicFlag_Set(&killFlag);
    pthread_join(renderer_main_th, NULL);
    LOG("Render thread exited");
    ASSERT_COMMON(gWindow, "Trying to destroy NULL window");
    glfwDestroyWindow(gWindow);
    gWindow = NULL;
    glfwTerminate();
    LOG("Renderer Dtr success");
    return RENDER_SUCCESS;
}
