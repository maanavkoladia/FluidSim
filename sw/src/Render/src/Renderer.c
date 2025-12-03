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
#include <math.h>
#include <pthread.h>
#include <stdio.h>

#ifdef DISPLAY_COLORS
#    include "Renderer_ColorFrame.h"
#else
#    include "Renderer_RawFrames.h"
#endif

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

static GLFWwindow* gWindow = NULL;
static int gWinW = WIDTH, gWinH = HEIGHT;

static pthread_t renderer_main_th;

static void Render_Draw(void) {

#ifdef DISPLAY_COLORS
    Render_ColorFrame_Process();
#else
    Render_RawFrame_Process();
#endif
}

// -----------------------------------------------------------------------------
// Frame / Init / Window Management
// -----------------------------------------------------------------------------
//

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
    // LF_Fifo_Init(&frameInFifo, FRAME_IN_FIFO_SIZE);

#    ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Init();
#    else
    Render_RawFrameProcessing_Init();
#    endif

    ASSERT_COMMON_POSIX(pthread_create(&renderer_main_th, NULL, Task_Renderer, NULL),
                        "Faield to inti the redernder thread");
    LOG("Renderer Init Success");
    return RENDER_SUCCESS;
}

#endif

render_err_t Render_Dtr(void) {
    AtomicFlag_Set(&killFlag);
    pthread_join(renderer_main_th, NULL);

#ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Dtr();
#else
    Render_RawFramesProcessing_Dtr();
#endif
    LOG("Render thread exited");
    ASSERT_COMMON(gWindow, "Trying to destroy NULL window");
    glfwDestroyWindow(gWindow);
    gWindow = NULL;
    glfwTerminate();
    LOG("Renderer Dtr success");
    return RENDER_SUCCESS;
}
