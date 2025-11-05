#include "Assert_Common.h"
#include "LOG.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>

#define HEIGHT (480)
#define WIDTH (640)

void KillGLFWProg(void* pvArgs) {
    char* msg = (char*)pvArgs;
    LOG("MSG: %s", msg);
    glfwTerminate();
}

void DrawSquare() {
    glBegin(GL_QUADS);
    glColor3f(0.2f, 0.7f, 0.3f); // greenish color
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(0.5f, 0.5f);
    glVertex2f(-0.5f, 0.5f);
    glEnd();
}

int main(void) {
    LOG("Program Starting Up");
    ASSERT_COMMON(glfwInit() != 0, "Fialed to init glfw");

    GLFWwindow* window = NULL;
    window = glfwCreateWindow(WIDTH, HEIGHT, "MySquare", NULL, NULL);
    ASSERT_COMMON_CB(window, KillGLFWProg, "Called From Main", "Window is NULL");
    LOG("Created window");
    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        DrawSquare();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    LOG("Program Exiting");
    return EXIT_SUCCESS;
}
