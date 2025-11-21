// main.cpp
#include <cmath>
#include <vector>
#include <iostream>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

// --------- simple helpers ---------
inline int idx(int i, int j, int Nx) { return j * Nx + i; }

// --------- shader sources ----------
const char* kVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

uniform vec2 uGridSize;

void main()
{
    // aPos is in [0, Nx] x [0, Ny] grid space
    vec2 uv  = aPos / uGridSize;    // [0,N] -> [0,1]
    vec2 ndc = uv * 2.0 - 1.0;      // [0,1] -> [-1,1]
    // ndc.y = -ndc.y;              // flip Y if you prefer

    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = aColor;
}
)";

const char* kFragmentShaderSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
)";

// --------- shader compile helpers ----------
GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        std::cerr << "Shader compile error: " << log.data() << std::endl;
    }
    return shader;
}

GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER,   vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Program link error: " << log.data() << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// --------- your data struct ----------
struct FluidField {
    int Nx;
    int Ny;
    const float* p;  // pressure: size Nx*Ny
    const float* u;  // x-vel: size Nx*Ny
    const float* v;  // y-vel: size Nx*Ny
};

// --------- renderer ----------
struct GridRenderer {
    int Nx = 0, Ny = 0;

    GLuint shaderProgram = 0;
    GLint  gridSizeLoc   = -1;

    GLuint gridVAO = 0, gridVBO = 0;
    GLuint pVAO    = 0, pVBO    = 0;
    GLuint velVAO  = 0, velVBO  = 0;

    int gridVertexCount = 0;

    std::vector<float> pressureVerts; // x,y,r,g,b
    std::vector<float> velVerts;      // x,y,r,g,b

    void init(int Nx_, int Ny_) {
        Nx = Nx_;
        Ny = Ny_;

        shaderProgram = createProgram(kVertexShaderSrc, kFragmentShaderSrc);
        glUseProgram(shaderProgram);

        gridSizeLoc = glGetUniformLocation(shaderProgram, "uGridSize");
        glUniform2f(gridSizeLoc, (float)Nx, (float)Ny);

        // --------- static grid lines ----------
        std::vector<float> gridVerts;
        auto pushGridV = [&](float x, float y, float r, float g, float b) {
            gridVerts.push_back(x);
            gridVerts.push_back(y);
            gridVerts.push_back(r);
            gridVerts.push_back(g);
            gridVerts.push_back(b);
        };

        // vertical lines
        for (int i = 0; i <= Nx; ++i) {
            float x = (float)i;
            pushGridV(x, 0.0f,      0.3f, 0.3f, 0.3f);
            pushGridV(x, (float)Ny, 0.3f, 0.3f, 0.3f);
        }
        // horizontal lines
        for (int j = 0; j <= Ny; ++j) {
            float y = (float)j;
            pushGridV(0.0f,      y, 0.3f, 0.3f, 0.3f);
            pushGridV((float)Nx, y, 0.3f, 0.3f, 0.3f);
        }

        gridVertexCount = (int)(gridVerts.size() / 5); // 5 floats per vertex

        glGenVertexArrays(1, &gridVAO);
        glGenBuffers(1, &gridVBO);
        glBindVertexArray(gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     gridVerts.size() * sizeof(float),
                     gridVerts.data(),
                     GL_STATIC_DRAW);

        GLsizei stride = 5 * sizeof(float);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        // --------- pressure (dynamic) ----------
        glGenVertexArrays(1, &pVAO);
        glGenBuffers(1, &pVBO);
        glBindVertexArray(pVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     Nx * Ny * 6 * 5 * sizeof(float), // worst case
                     nullptr,
                     GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        // --------- velocity (dynamic) ----------
        glGenVertexArrays(1, &velVAO);
        glGenBuffers(1, &velVBO);
        glBindVertexArray(velVAO);
        glBindBuffer(GL_ARRAY_BUFFER, velVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     Nx * Ny * 2 * 5 * sizeof(float), // 1 line (2 vertices) per cell
                     nullptr,
                     GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    static std::array<float,3> pressureColor(float p, float pMin, float pMax) {
        float t = (p - pMin) / (pMax - pMin + 1e-8f);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        // blue -> red
        return { t, 0.0f, 1.0f - t };
    }

    void update(const FluidField& field) {
        // --- compute pressure range ---
        float pMin = field.p[0];
        float pMax = field.p[0];
        for (int j = 0; j < Ny; ++j) {
            for (int i = 0; i < Nx; ++i) {
                float v = field.p[idx(i,j,Nx)];
                pMin = std::min(pMin, v);
                pMax = std::max(pMax, v);
            }
        }

        // --- build pressure quads ---
        pressureVerts.clear();
        pressureVerts.reserve(Nx * Ny * 6 * 5);

        auto pushP = [&](float x, float y, float r, float g, float b) {
            pressureVerts.push_back(x);
            pressureVerts.push_back(y);
            pressureVerts.push_back(r);
            pressureVerts.push_back(g);
            pressureVerts.push_back(b);
        };

        for (int j = 0; j < Ny; ++j) {
            for (int i = 0; i < Nx; ++i) {
                float x0 = (float)i;
                float x1 = (float)(i + 1);
                float y0 = (float)j;
                float y1 = (float)(j + 1);

                float p = field.p[idx(i,j,Nx)];
                auto col = pressureColor(p, pMin, pMax);
                float r = col[0], g = col[1], b = col[2];

                // tri 1
                pushP(x0, y0, r,g,b);
                pushP(x1, y0, r,g,b);
                pushP(x1, y1, r,g,b);
                // tri 2
                pushP(x0, y0, r,g,b);
                pushP(x1, y1, r,g,b);
                pushP(x0, y1, r,g,b);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, pVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        pressureVerts.size() * sizeof(float),
                        pressureVerts.data());

        // --- build velocity arrows ---
        velVerts.clear();
        velVerts.reserve(Nx * Ny * 2 * 5);

        auto pushLine = [&](float x0, float y0, float x1, float y1, float r, float g, float b) {
            // start
            velVerts.push_back(x0); velVerts.push_back(y0);
            velVerts.push_back(r);  velVerts.push_back(g); velVerts.push_back(b);
            // end
            velVerts.push_back(x1); velVerts.push_back(y1);
            velVerts.push_back(r);  velVerts.push_back(g); velVerts.push_back(b);
        };

        float arrowScale = 0.3f; // tweak

        for (int j = 0; j < Ny; ++j) {
            for (int i = 0; i < Nx; ++i) {
                float u = field.u[idx(i,j,Nx)];
                float v = field.v[idx(i,j,Nx)];

                float cx = (float)i + 0.5f;
                float cy = (float)j + 0.5f;

                float x1 = cx + arrowScale * u;
                float y1 = cy + arrowScale * v;

                pushLine(cx, cy, x1, y1, 1.0f, 1.0f, 1.0f);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, velVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        velVerts.size() * sizeof(float),
                        velVerts.data());
    }

    void draw() {
        glUseProgram(shaderProgram);
        glUniform2f(gridSizeLoc, (float)Nx, (float)Ny);

        // draw pressure
        glBindVertexArray(pVAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(pressureVerts.size() / 5));

        // draw grid lines
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVertexCount);

        // draw velocities
        glBindVertexArray(velVAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)(velVerts.size() / 5));

        glBindVertexArray(0);
    }
};

// --------- sample field initialization ----------
void initSampleField(int Nx, int Ny,
                     std::vector<float>& p,
                     std::vector<float>& u,
                     std::vector<float>& v,
                     float t)
{
    float cx = 0.5f * Nx;
    float cy = 0.5f * Ny;

    for (int j = 0; j < Ny; ++j) {
        for (int i = 0; i < Nx; ++i) {
            int id = idx(i,j,Nx);
            float x = (float)i + 0.5f;
            float y = (float)j + 0.5f;

            // pressure: a Gaussian bump that wiggles over time
            float dx = x - cx;
            float dy = y - cy;
            float r2 = (dx*dx + dy*dy) / (0.1f * Nx * Ny);
            p[id] = std::exp(-r2) * (0.5f + 0.5f*std::sin(0.5f * t));

            // velocity: circular flow around center
            float vx = -(y - cy);
            float vy =  (x - cx);
            float len = std::sqrt(vx*vx + vy*vy) + 1e-5f;
            vx /= len;
            vy /= len;
            float speed = 0.8f; // arbitrary
            u[id] = speed * vx;
            v[id] = speed * vy;
        }
    }
}

// --------- main ----------
int main() {
    const int Nx = 32;
    const int Ny = 32;

    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Fluid Grid", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // init GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return 1;
    }

    glViewport(0, 0, 800, 800);

    // sample data
    std::vector<float> p(Nx * Ny);
    std::vector<float> u(Nx * Ny);
    std::vector<float> v(Nx * Ny);

    GridRenderer renderer;
    renderer.init(Nx, Ny);

    float time = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // --- update sample field ---
        time += 0.016f; // fake dt
        initSampleField(Nx, Ny, p, u, v, time);

        FluidField field { Nx, Ny, p.data(), u.data(), v.data() };
        renderer.update(field);

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderer.draw();

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
