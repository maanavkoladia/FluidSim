#include "Renderer_OffScreen.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "LOG.h"
#include "unistd.h"
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

#ifdef DISPLAY_COLORS
#    include "Renderer_ColorFrame.h"
#else
#    include "Renderer_RawFrames.h"
#endif

#define GLEW_STATIC  // optional if using static lib
#include <GL/glew.h> // MUST be before gl.h
//
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" // make sure this is in your include path

static AtomicFlag_t killFlag;

// EGL context variables
static EGLDisplay eglDisplay = EGL_NO_DISPLAY;
static EGLContext eglContext = EGL_NO_CONTEXT;
static EGLSurface eglSurface = EGL_NO_SURFACE;

static int gWinW = RENDER_WINDOW_WIDTH;
static int gWinH = RENDER_WINDOW_HEIGHT;

// FBO variables
static GLuint fbo = 0;
static GLuint fboTexture = 0;
static GLuint rboDepth = 0;
static unsigned char* pixelBuffer = NULL;

static pthread_t renderer_main_th;

static void Render_Draw(void) {
#ifdef DISPLAY_COLORS
    Render_ColorFrame_Process();
#else
    Render_RawFrame_Process();
#endif
}

static void framebuffer_resize(int w, int h) {
    gWinW = (w > 0) ? w : 1;
    gWinH = (h > 0) ? h : 1;
    glViewport(0, 0, gWinW, gWinH);
}

static void* Task_OffScreen_Buffering(void* pvArgs) {
    // 1. Initialize EGL
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    ASSERT_COMMON(eglDisplay != EGL_NO_DISPLAY, "Failed to get EGL display");
    ASSERT_COMMON(eglInitialize(eglDisplay, NULL, NULL) != 0, "Failed to initialize EGL");

    // 2. Choose EGL config
    EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_BIT,
                              EGL_RED_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_ALPHA_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              24,
                              EGL_NONE};
    EGLConfig config;
    EGLint numConfigs;
    ASSERT_COMMON(eglChooseConfig(eglDisplay, configAttribs, &config, 1, &numConfigs) != 0,
                  "Failed to choose EGL config");
    ASSERT_COMMON(numConfigs != 0, "No matching EGL configs found");

    // 3. Create PBuffer surface
    EGLint pbufferAttribs[] = {EGL_WIDTH, gWinW, EGL_HEIGHT, gWinH, EGL_NONE};
    eglSurface = eglCreatePbufferSurface(eglDisplay, config, pbufferAttribs);
    ASSERT_COMMON(eglSurface != EGL_NO_SURFACE, "Failed to create PBuffer surface");

    // 4. Bind OpenGL API and create context
    ASSERT_COMMON(eglBindAPI(EGL_OPENGL_API), "Failed to bind OpenGL API");
    eglContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, NULL);
    ASSERT_COMMON(eglContext != EGL_NO_CONTEXT, "Failed to create EGL context");
    ASSERT_COMMON(eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext),
                  "Failed to make EGL context current");
    ASSERT_COMMON(glewInit() == GLEW_OK, "Failed to initialize GLEW");

    LOG("EGL offscreen context created successfully");

    // 5. Setup FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &fboTexture);
    glBindTexture(GL_TEXTURE_2D, fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gWinW, gWinH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, gWinW, gWinH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    ASSERT_COMMON(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                  "FBO setup failed");

    framebuffer_resize(gWinW, gWinH);

    pixelBuffer = (unsigned char*)malloc(gWinW * gWinH * 4);
    ASSERT_COMMON(pixelBuffer != NULL, "Failed to allocate pixel buffer");

    LOG("Render Loop started");

    // Make sure folder exists
    system("mkdir -p frames");

    int frameIndex = 0;
    while (AtomicFlag_GetStatus(&killFlag) != FLAG_SET) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Render_Draw();
        usleep(16666);
        // Read pixels from FBO
        glReadPixels(0, 0, gWinW, gWinH, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer);

        // Save PNG using stb_image_write
        char filename[256];
        snprintf(filename, sizeof(filename), "frames/frame_%05d.png", frameIndex++);
        // Note: OpenGL origin is bottom-left, PNG expects top-left origin, so flip vertically
        unsigned char* flipped = (unsigned char*)malloc(gWinW * gWinH * 4);
        for (int y = 0; y < gWinH; y++) {
            memcpy(flipped + (gWinH - 1 - y) * gWinW * 4, pixelBuffer + y * gWinW * 4, gWinW * 4);
        }
        stbi_write_png(filename, gWinW, gWinH, 4, flipped, gWinW * 4);
        free(flipped);
    }

    return NULL;
}

render_err_t OffScreenRender_Init(void) {
    LOG("Render Starting Up");
    AtomicFlag_Clear(&killFlag);

#ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Init();
#else
    Render_RawFrameProcessing_Init();
#endif

    ASSERT_COMMON_POSIX(pthread_create(&renderer_main_th, NULL, Task_OffScreen_Buffering, NULL),
                        "Failed to start renderer thread");
    LOG("Renderer Init Success");
    return RENDER_SUCCESS;
}

render_err_t OffScreenRender_Dtr(void) {
    AtomicFlag_Set(&killFlag);
    pthread_join(renderer_main_th, NULL);

#ifdef DISPLAY_COLORS
    Render_ColorFramesProcessing_Dtr();
#else
    Render_RawFramesProcessing_Dtr();
#endif

    // Cleanup FBO
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (fboTexture) {
        glDeleteTextures(1, &fboTexture);
        fboTexture = 0;
    }
    if (rboDepth) {
        glDeleteRenderbuffers(1, &rboDepth);
        rboDepth = 0;
    }
    if (pixelBuffer) {
        free(pixelBuffer);
        pixelBuffer = NULL;
    }

    // Cleanup EGL
    if (eglDisplay != EGL_NO_DISPLAY) {
        ASSERT_COMMON(eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT),
                      "Failed to release EGL context");
    }
    if (eglContext != EGL_NO_CONTEXT) {
        ASSERT_COMMON(eglDestroyContext(eglDisplay, eglContext) != EGL_FALSE,
                      "Failed to destroy EGL context");
        eglContext = EGL_NO_CONTEXT;
    }
    if (eglSurface != EGL_NO_SURFACE) {
        ASSERT_COMMON(eglDestroySurface(eglDisplay, eglSurface) != EGL_FALSE,
                      "Failed to destroy EGL surface");
        eglSurface = EGL_NO_SURFACE;
    }
    if (eglDisplay != EGL_NO_DISPLAY) {
        ASSERT_COMMON(eglTerminate(eglDisplay) != EGL_FALSE, "Failed to terminate EGL display");
        eglDisplay = EGL_NO_DISPLAY;
    }

    LOG("Renderer Dtr success");
    return RENDER_SUCCESS;
}
