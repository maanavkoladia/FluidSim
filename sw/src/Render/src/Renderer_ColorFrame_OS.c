#include "../../Transform/inc/Transform.h"
#include "../inc/Renderer.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "LFfifo.h"
#include "Renderer_ColorFrame.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static LF_Fifo_t* pColorFrameInFifo = NULL;
static int frame_counter = 0;

/* If you don't want the program to stream directly to ffmpeg set this to 0.
 * When enabled the program will both write frames to frames/frame_00000.rgba ...
 * AND stream them to ffmpeg via a pipe to produce output.mp4 in realtime. */
#ifndef ENABLE_FFMPEG_PIPE
#    define ENABLE_FFMPEG_PIPE 0
#endif

static FILE* video_pipe = NULL;

#ifdef OFF_SCREEN_RENDERING

// ----------------------------------------
// Convert float colors (0.0-1.0) to bytes (0-255)
// ----------------------------------------
static inline unsigned char float_to_byte(float f) {
    if (f <= 0.0f) return 0;
    if (f >= 1.0f) return 255;
    /* multiply before cast to reduce rounding bias */
    return (unsigned char)(f * 255.0f);
}

// ----------------------------------------
// Helper: ensure directory exists
// ----------------------------------------
static void ensure_frames_dir(void) {
    /* mkdir -p equivalent */
    struct stat st = {0};
    if (stat("frames", &st) == -1) {
        if (mkdir("frames", 0755) != 0) {
            fprintf(stderr, "ERROR: failed to create frames/ directory\n");
        }
    }
}

// ----------------------------------------
// Raw RGBA - write to disk and optionally pipe to ffmpeg
// ----------------------------------------
static void write_frame_raw_to_ffmpeg(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->colors);

    ensure_frames_dir();

#    if ENABLE_FFMPEG_PIPE
    /* Open pipe to ffmpeg on first frame */
    if (video_pipe == NULL) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -y -f rawvideo -pixel_format rgba -video_size %lux%lu -framerate 100 "
                 "-i - -c:v libx264 -preset ultrafast -crf 18 output.mp4",
                 (unsigned long)pFrame->width, (unsigned long)pFrame->height);

        printf("FFmpeg command: %s\n", cmd);
        video_pipe = popen(cmd, "w");
        if (!video_pipe) {
            fprintf(stderr, "ERROR: popen() failed opening ffmpeg pipe\n");
            /* We continue: frames will still be written to disk. */
        } else {
            LOG("Opened ffmpeg pipe for raw video encoding (converting float->byte)");
        }
    }
#    endif

    // Convert float colors to byte RGBA
    size_t num_pixels = (size_t)pFrame->width * (size_t)pFrame->height;
    size_t frame_bytes = num_pixels * 4;
    unsigned char* byte_buffer = (unsigned char*)malloc(frame_bytes);
    if (!byte_buffer) {
        fprintf(stderr, "ERROR: malloc failed for frame buffer (%zu bytes)\n", frame_bytes);
        return;
    }

    for (size_t i = 0; i < num_pixels; i++) {
        Color_t* c = &pFrame->colors[i];
        byte_buffer[i * 4 + 0] = float_to_byte(c->r);
        byte_buffer[i * 4 + 1] = float_to_byte(c->g);
        byte_buffer[i * 4 + 2] = float_to_byte(c->b);
        byte_buffer[i * 4 + 3] = float_to_byte(c->a);
    }

    // --- WRITE TO DISK ---
    char filename[256];
    snprintf(filename, sizeof(filename), "frames/frame_%05d.rgba", frame_counter);

    FILE* f = fopen(filename, "wb");
    if (f) {
        size_t wrote = fwrite(byte_buffer, 1, frame_bytes, f);
        if (wrote != frame_bytes) {
            fprintf(stderr, "ERROR: wrote %zu/%zu bytes to %s\n", wrote, frame_bytes, filename);
        }
        fflush(f);
        /* optionally ensure data is on disk */
#    if defined(__linux__)
        /* get fd and fsync */
        int fd = fileno(f);
        if (fd >= 0) {
            fsync(fd);
        }
#    endif
        fclose(f);
    } else {
        fprintf(stderr, "ERROR: Cannot write frame to %s: %s\n", filename, strerror(errno));
    }

    // --- WRITE TO FFmpeg PIPE (if available) ---
#    if ENABLE_FFMPEG_PIPE
    if (video_pipe) {
        size_t written = fwrite(byte_buffer, 1, frame_bytes, video_pipe);
        if (written != frame_bytes) {
            fprintf(stderr, "WARNING: Frame %d - wrote %zu/%zu bytes to FFmpeg\n", frame_counter,
                    written, frame_bytes);
        }
        fflush(video_pipe);
    }
#    endif

    free(byte_buffer);

    /* increment frame counter only here */
    frame_counter++;
}

// ----------------------------------------
// Public API
// ----------------------------------------
render_err_t Render_ColorFrame_Process(void) {
    Render_Frame_Colors_t* pFrame = NULL;

    /* Non-blocking pop from FIFO (spin) */
    while (LF_Fifo_SpinPop(pColorFrameInFifo, &pFrame) == LF_FIFO_FAIL_TRY_POP) {
        sched_yield();
    }

    ASSERT_COMMON_NOT_NULL(pFrame);

    /* Write frame (writes to disk and optionally pipes to ffmpeg) */
    write_frame_raw_to_ffmpeg(pFrame);

    /* Return frame to transform service */
    TransForm_ColorFrameYeild(pFrame);

    return RENDER_SUCCESS;
}

render_err_t Render_Send_Frame_Colors(Render_Frame_Colors_t* pFrameIn) {
    ASSERT_COMMON_NOT_NULL(pFrameIn && pFrameIn->colors);
    ASSERT_COMMON(pFrameIn->height == RENDER_WINDOW_HEIGHT, "Height mismatch");
    ASSERT_COMMON(pFrameIn->width == RENDER_WINDOW_WIDTH, "Width mismatch");

    while (LF_Fifo_TryPush(pColorFrameInFifo, pFrameIn) == LF_FIFO_FAIL_TRY_PUSH) {
        sched_yield();
    }

    return RENDER_SUCCESS;
}

render_err_t Render_ColorFramesProcessing_Init(void) {
    ASSERT_COMMON_POSIX(LF_Fifo_Init(&pColorFrameInFifo, FRAME_IN_FIFO_SIZE),
                        "Failed to init frame FIFO");

    frame_counter = 0;
    video_pipe = NULL;

    /* Create frames directory up-front (avoid repeated system() calls) */
    ensure_frames_dir();

    LOG("Renderer: Color Frames Init Success (Writing raw RGBA files to frames/)");
    return RENDER_SUCCESS;
}

render_err_t Render_ColorFramesProcessing_Dtr(void) {
    /* Close ffmpeg pipe if opened (and wait for encoder to finish) */
#    if ENABLE_FFMPEG_PIPE
    if (video_pipe) {
        int rc = pclose(video_pipe);
        if (rc != 0) {
            LOG("Warning: ffmpeg returned non-zero status: %d", rc);
        }
        video_pipe = NULL;
        LOG("Closed ffmpeg pipe - wrote %d frames to output.mp4", frame_counter);
    }
#    endif

    ASSERT_COMMON_POSIX(LF_Fifo_Dtr(pColorFrameInFifo), "Failed to DTR FIFO");

    LOG("Wrote %d raw RGBA frames to frames/ directory", frame_counter);
    LOG("Convert to video with:");
    LOG("  ffmpeg -f rawvideo -pixel_format rgba -video_size %dx%d -framerate 60 -i "
        "frames/frame_%%05d.rgba -c:v libx264 -preset fast -crf 18 output.mp4",
        RENDER_WINDOW_WIDTH, RENDER_WINDOW_HEIGHT);

    return RENDER_SUCCESS;
}

#endif /* OFF_SCREEN_RENDERING */
