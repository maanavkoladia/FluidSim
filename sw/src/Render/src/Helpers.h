#pragma once

typedef struct GLFWwindow GLFWwindow;

void draw_grid(void);
void draw_velocities(void);
void draw_pressure(void);

int Render_ShouldClose(void);
void Render_SwapBuffers(void);
void Render_PollEvents(void);
GLFWwindow* Render_GetWindow(void);
