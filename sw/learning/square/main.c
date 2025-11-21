#include "Assert_Common.h"
#include "LOG.h"
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define HEIGHT (480)
#define WIDTH  (640)

#define MAX_POINTS   200000
#define MAX_STROKES  5000

typedef struct { float x, y; } Pt;
typedef struct { float r, g, b; } Color;

static GLFWwindow* gWindow = NULL;
static int gWinW = WIDTH, gWinH = HEIGHT;

static Pt     gPoints[MAX_POINTS];
static int    gPointCount = 0;

static int    gStrokeStarts[MAX_STROKES];   // index into gPoints where each stroke starts
static float  gStrokeSizes[MAX_STROKES];
static Color  gStrokeColors[MAX_STROKES];
static int    gStrokeCount = 0;

static int    gMouseDown = 0;
static Color  gCurrentColor = {1.0f, 0.2f, 0.1f}; // default red-ish
static float  gCurrentSize  = 3.0f;

// ----------------- helpers -----------------

static void window_to_gl_coords(double x, double y, float* glx, float* gly) {
    // convert window pixels to OpenGL NDC [-1,1]
    *glx = (float)((x / (double)gWinW) * 2.0 - 1.0);
    *gly = (float)(-((y / (double)gWinH) * 2.0 - 1.0));
}

static void start_new_stroke() {
    if (gStrokeCount >= MAX_STROKES) return;
    gStrokeStarts[gStrokeCount] = gPointCount;
    gStrokeColors[gStrokeCount] = gCurrentColor;
    gStrokeSizes[gStrokeCount]  = gCurrentSize;
    gStrokeCount++;
}

static void push_point(float x, float y) {
    if (gPointCount >= MAX_POINTS) return;
    gPoints[gPointCount].x = x;
    gPoints[gPointCount].y = y;
    gPointCount++;
}

static void clear_canvas() {
    gPointCount  = 0;
    gStrokeCount = 0;
}

// Very simple PPM writer (binary P6). glReadPixels gives bottom-left origin; PPM expects top row first.
// We flip the rows while writing.
static int save_framebuffer_ppm(const char* path, int w, int h) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;

    // header
    fprintf(f, "P6\n%d %d\n255\n", w, h);

    unsigned char* buf = (unsigned char*)malloc((size_t)w * (size_t)h * 3);
    if (!buf) { fclose(f); return 0; }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT); // read the front buffer (already drawn)
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);

    // flip vertically: write rows from top to bottom
    for (int row = h - 1; row >= 0; --row) {
        size_t offset = (size_t)row * (size_t)w * 3;
        fwrite(buf + offset, 1, (size_t)w * 3, f);
    }

    free(buf);
    fclose(f);
    return 1;
}

// ----------------- callbacks -----------------

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    gWinW = (width  > 0) ? width  : 1;
    gWinH = (height > 0) ? height : 1;
    glViewport(0, 0, gWinW, gWinH);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            gMouseDown = 1;
            start_new_stroke();

            // on press, also add the first point immediately to start the line from here
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            float gx, gy;
            window_to_gl_coords(xpos, ypos, &gx, &gy);
            push_point(gx, gy);
        } else if (action == GLFW_RELEASE) {
            gMouseDown = 0;
        }
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!gMouseDown) return;

    float gx, gy;
    window_to_gl_coords(xpos, ypos, &gx, &gy);
    push_point(gx, gy);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, 1);
            break;
        case GLFW_KEY_C: // clear
            clear_canvas();
            break;
        case GLFW_KEY_S: { // save
            char path[128];
            // simple incrementing filename: paint_XXXX.ppm
            static int sshot = 0;
            snprintf(path, sizeof(path), "paint_%04d.ppm", sshot++);
            if (save_framebuffer_ppm(path, gWinW, gWinH)) {
                LOG("Saved %s", path);
            } else {
                LOG("Failed to save screenshot");
            }
        } break;

        // size controls (optional nicety)
        case GLFW_KEY_LEFT_BRACKET:  // '[' smaller
            gCurrentSize = (gCurrentSize > 1.0f) ? (gCurrentSize - 1.0f) : 1.0f;
            break;
        case GLFW_KEY_RIGHT_BRACKET: // ']' bigger
            gCurrentSize += 1.0f;
            break;

        // color palette 1–9
        case GLFW_KEY_1: gCurrentColor = (Color){1.0f, 0.2f, 0.1f}; break; // red-ish
        case GLFW_KEY_2: gCurrentColor = (Color){0.2f, 0.8f, 0.2f}; break; // green
        case GLFW_KEY_3: gCurrentColor = (Color){0.2f, 0.4f, 1.0f}; break; // blue
        case GLFW_KEY_4: gCurrentColor = (Color){1.0f, 0.8f, 0.2f}; break; // yellow
        case GLFW_KEY_5: gCurrentColor = (Color){1.0f, 0.4f, 0.8f}; break; // pink
        case GLFW_KEY_6: gCurrentColor = (Color){0.2f, 1.0f, 1.0f}; break; // cyan
        case GLFW_KEY_7: gCurrentColor = (Color){1.0f, 1.0f, 1.0f}; break; // white
        case GLFW_KEY_8: gCurrentColor = (Color){0.6f, 0.4f, 0.2f}; break; // brown
        case GLFW_KEY_9: gCurrentColor = (Color){0.9f, 0.9f, 0.9f}; break; // light gray
        default: break;
    }
}

// ----------------- rendering -----------------

static void draw_strokes() {
    // draw each stroke as its own line strip with its own color/width
    for (int s = 0; s < gStrokeCount; ++s) {
        int start = gStrokeStarts[s];
        int end   = (s == gStrokeCount - 1) ? gPointCount : gStrokeStarts[s + 1];
        int count = end - start;

        if (count <= 0) continue;

        glColor3f(gStrokeColors[s].r, gStrokeColors[s].g, gStrokeColors[s].b);

        if (count == 1) {
            glPointSize(gStrokeSizes[s]);
            glBegin(GL_POINTS);
            glVertex2f(gPoints[start].x, gPoints[start].y);
            glEnd();
        } else {
            glLineWidth(gStrokeSizes[s]);
            glBegin(GL_LINE_STRIP);
            for (int i = start; i < end; ++i) {
                glVertex2f(gPoints[i].x, gPoints[i].y);
            }
            glEnd();
        }
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

    gWindow = glfwCreateWindow(WIDTH, HEIGHT, "Tiny Paint (GLFW + immediate mode)", NULL, NULL);
    ASSERT_COMMON_CB(gWindow, KillGLFWProg, "Called From Main", "Window is NULL");
    glfwMakeContextCurrent(gWindow);
    glfwSwapInterval(1); // vsync

    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    glfwSetMouseButtonCallback(gWindow, mouse_button_callback);
    glfwSetCursorPosCallback(gWindow, cursor_position_callback);
    glfwSetKeyCallback(gWindow, key_callback);

    // initial viewport
    framebuffer_size_callback(gWindow, WIDTH, HEIGHT);

    // background color
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);

    while (!glfwWindowShouldClose(gWindow)) {
        glClear(GL_COLOR_BUFFER_BIT);

        draw_strokes();

        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }

    glfwDestroyWindow(gWindow);
    glfwTerminate();
    LOG("Program Exiting");
    return EXIT_SUCCESS;
}
