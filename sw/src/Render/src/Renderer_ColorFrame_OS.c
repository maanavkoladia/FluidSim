#include "../../Transform/inc/Transform.h"
#include "../inc/Renderer.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "LFfifo.h"
#include "Renderer_ColorFrame.h"
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static LF_Fifo_t* pColorFrameInFifo = NULL;
static int frame_counter = 0;

#ifdef OFF_SCREEN_RENDERING

// ----------------------------------------
// Convert float colors (0.0-1.0) to bytes (0-255)
// ----------------------------------------
static inline unsigned char float_to_byte(float f) {
    if (f <= 0.0f) return 0;
    if (f >= 1.0f) return 255;
    return (unsigned char)(f * 255.0f);
}

// ----------------------------------------
// Write raw RGBA to file
// ----------------------------------------
static void write_frame_raw_to_ffmpeg(Render_Frame_Colors_t* pFrame) {
    ASSERT_COMMON_NOT_NULL(pFrame && pFrame->colors);

    // Ensure frames/ directory exists
    system("mkdir -p frames");

    // Open pipe to ffmpeg on first frame
    if (video_pipe == NULL) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd),
                 "ffmpeg -y -f rawvideo -pixel_format rgba -video_size %lux%lu -framerate 100 "
                 "-i - -c:v libx264 -preset ultrafast -crf 18 output.mp4",
                 (unsigned long)pFrame->width, (unsigned long)pFrame->height);

        printf("FFmpeg command: %s\n", cmd);
        video_pipe = popen(cmd, "w");
        ASSERT_COMMON_NOT_NULL(video_pipe);
        LOG("Opened ffmpeg pipe for raw video encoding (converting float->byte)");
    }

    // Convert float colors to byte RGBA
    size_t num_pixels = pFrame->width * pFrame->height;
    unsigned char* byte_buffer = (unsigned char*)malloc(num_pixels * 4);
    ASSERT_COMMON_NOT_NULL(byte_buffer);

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
        fwrite(byte_buffer, 1, num_pixels * 4, f);
        fclose(f);
    } else {
        fprintf(stderr, "ERROR: Cannot write frame to %s\n", filename);
    }

    // --- WRITE TO FFmpeg PIPE ---
    size_t written = fwrite(byte_buffer, 1, num_pixels * 4, video_pipe);
    if (written != num_pixels * 4) {
        fprintf(stderr, "WARNING: Frame %d - wrote %zu/%zu bytes to FFmpeg\n", frame_counter,
                written, num_pixels * 4);
    }

    fflush(video_pipe);
    free(byte_buffer);
    frame_counter++;
}

// ----------------------------------------
// Public API
// ----------------------------------------
render_err_t Render_ColorFrame_Process(void) {
    Render_Frame_Colors_t* pFrame = NULL;

    // Non-blocking pop from FIFO
    while (LF_Fifo_SpinPop(pColorFrameInFifo, &pFrame) == LF_FIFO_FAIL_TRY_POP) {
        sched_yield();
    }

    ASSERT_COMMON_NOT_NULL(pFrame);

    // Write frame
    write_frame_raw_to_file(pFrame);
    frame_counter++;

    // Return frame to transform service
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

    // Create frames directory
    system("mkdir -p frames");

    LOG("Renderer: Color Frames Init Success (Writing raw RGBA files to frames/)");
    return RENDER_SUCCESS;
}

render_err_t Render_ColorFramesProcessing_Dtr(void) {
    ASSERT_COMMON_POSIX(LF_Fifo_Dtr(pColorFrameInFifo), "Failed to DTR FIFO");

    LOG("Wrote %d raw RGBA frames to frames/ directory", frame_counter);
    LOG("Convert to video with:");
    LOG("  ffmpeg -f rawvideo -pixel_format rgba -video_size %dx%d -framerate 60 -i "
        "frames/frame_%%05d.rgba -c:v libx264 -preset fast -crf 18 output.mp4",
        RENDER_WINDOW_WIDTH, RENDER_WINDOW_HEIGHT);

    return RENDER_SUCCESS;
}

#endif
