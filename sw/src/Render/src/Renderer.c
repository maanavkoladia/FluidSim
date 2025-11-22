#include "../inc/Renderer.h"
#include "../inc/glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

static GLuint gShaderProgram = 0;
static GLuint gGridVAO = 0;
static GLuint gGridVBO = 0;
static int gVertexCount = 0;

#define GRID_NX 10
#define GRID_NY 10

