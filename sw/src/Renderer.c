#define GL_SILENCE_DEPRECATION  // Silence macOS OpenGL deprecation warnings
#include <GLFW/glfw3.h>
#include "Sim/inc/Sim.h"
#include "LOG.h"

GLFWwindow* window;
sim_state_t* g_sim_state;

void framebuffer_size_callback(GLFWwindow* win, int width, int height) {
    (void)win;  // Unused parameter
    glViewport(0, 0, width, height);
}

void render_fluid() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (!g_sim_state || !g_sim_state->cells) return;
    
    // Render each cell as a colored quad
    glBegin(GL_QUADS);
    for (int i = 0; i < g_sim_state->nx; i++) {
        for (int j = 0; j < g_sim_state->ny; j++) {
            Cell_t* cell = &g_sim_state->cells[i + j * g_sim_state->nx];
            
            // Calculate normalized coordinates
            float x = (float)i / g_sim_state->nx;
            float y = (float)j / g_sim_state->ny;
            float size = 1.0f / g_sim_state->nx;
            
            // Color based on level set (phi)
            if (cell->phi < 0) {
                glColor3f(0.0f, 0.5f, 1.0f);  // Blue water
            } else {
                glColor3f(0.8f, 0.8f, 1.0f);  // Light blue air
            }
            
            // Draw quad
            glVertex2f(x, y);
            glVertex2f(x + size, y);
            glVertex2f(x + size, y + size);
            glVertex2f(x, y + size);
        }
    }
    glEnd();
}

int init_opengl() {
    // Initialize GLFW
    if (!glfwInit()) {
        LOG("Failed to initialize GLFW");
        return -1;
    }
    
    // Create window
    window = glfwCreateWindow(800, 600, "Fluid Simulation", NULL, NULL);
    if (!window) {
        LOG("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // Set up OpenGL (no GLAD needed)
    glViewport(0, 0, 800, 600);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    return 0;
}

void cleanup_opengl() {
    glfwTerminate();
}

int should_close() {
    return glfwWindowShouldClose(window);
}

void swap_buffers() {
    glfwSwapBuffers(window);
}

void poll_events() {
    glfwPollEvents();
}

void set_sim_state(sim_state_t* state) {
    g_sim_state = state;
}

void render_grid(int nx, int ny){
    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(0.1f);
}