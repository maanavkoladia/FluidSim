#include "Renderer_Display.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "LOG.h"
#include "unistd.h"
#include <GLFW/glfw3.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

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

static AtomicFlag_t killFlag;
static GLFWwindow* gWindow = NULL;
static int gWinW = RENDER_WINDOW_WIDTH, gWinH = RENDER_WINDOW_HEIGHT;

static pthread_t renderer_main_th;

static void Render_Draw(void) {

#ifdef DISPLAY_COLORS
    Render_ColorFrame_Process();
#else
    Render_RawFrame_Process();
#endif
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

GLFWwindow* Render_GetWindow(void) {
    return gWindow;
}

static void* Task_Renderer_Display(void* pvArgs) {
    if (!glfwInit()) return NULL;

    gWindow = glfwCreateWindow(RENDER_WINDOW_WIDTH, RENDER_WINDOW_HEIGHT, "Fluid Sim", NULL, NULL);
    ASSERT_COMMON_NOT_NULL(gWindow);

    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetKeyCallback(gWindow, key_callback);

    framebuffer_size_callback(gWindow, RENDER_WINDOW_WIDTH, RENDER_WINDOW_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    LOG("Render Loop started");
    while (!Render_ShouldClose()) {
        if (AtomicFlag_GetStatus(&killFlag) == FLAG_SET) {
            return NULL;
        }
        // usleep(50000);
        Render_Draw();

        Render_SwapBuffers();
        Render_PollEvents();
    }

    return NULL;
}

#if defined(__APPLE__)
render_err_t DisplayService_Init(void) {
    LOG("Render Starting Up");
    AtomicFlag_Clear(&killFlag);
    // create the sim snap fifo
    // LF_Fifo_Init(&frameInFifo, FRAME_IN_FIFO_SIZE);

#    ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Init();
#    else
    Render_RawFrameProcessing_Init();
#    endif
    Task_Renderer_Display(NULL);
    LOG("Renderer Init Success");
    return RENDER_SUCCESS;
}

#else

render_err_t DisplayService_Init(void) {
    LOG("Render Starting Up");
    AtomicFlag_Clear(&killFlag);
    // create the sim snap fifo
    // LF_Fifo_Init(&frameInFifo, FRAME_IN_FIFO_SIZE);

#    ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Init();
#    else
    Render_RawFrameProcessing_Init();
#    endif

    ASSERT_COMMON_POSIX(pthread_create(&renderer_main_th, NULL, Task_Renderer_Display, NULL),
                        "Faield to inti the redernder thread");
    LOG("Renderer Init Success");
    return RENDER_SUCCESS;
}

#endif

render_err_t DisplayService_Dtr(void) {
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
