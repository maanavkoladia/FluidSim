#define GL_SILENCE_DEPRECATION
#include "../../../mpsLibC/common/Assert_Common.h"
#include "../../../mpsLibC/common/LOG.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>

#define HEIGHT (480)
#define WIDTH  (640)

#define GRID_NX 10
#define GRID_NY 10

static GLFWwindow* gWindow = NULL;
static int gWinW = WIDTH, gWinH = HEIGHT;

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

// ----------------- main -----------------

static void KillGLFWProg(void* pvArgs) {
    char* msg = (char*)pvArgs;
    LOG("MSG: %s", msg);
    glfwTerminate();
}

int main(void) {
    LOG("Program Starting Up");
    ASSERT_COMMON(glfwInit() != 0, "Failed to init glfw");

    gWindow = glfwCreateWindow(WIDTH, HEIGHT, "Fluid Sim", NULL, NULL);
    ASSERT_COMMON_CB(gWindow, KillGLFWProg, "Called From Main", "Window is NULL");
    
    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetKeyCallback(gWindow, key_callback);

    framebuffer_size_callback(gWindow, WIDTH, HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark grey background

    while (!glfwWindowShouldClose(gWindow)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }

    glfwDestroyWindow(gWindow);
    glfwTerminate();
    LOG("Program Exiting");
    return EXIT_SUCCESS;
}
