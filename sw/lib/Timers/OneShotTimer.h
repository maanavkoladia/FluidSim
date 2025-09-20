#ifndef ONE_SHOT_TIMER_H
#define ONE_SHOT_TIMER_H

/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>
#include "../common/Assert_Common.h"
#include <string.h>

/* ================================================== */
/*                      DEFINES                       */
/* ================================================== */

typedef uint64_t timeMS_t;

#define MAX_TAG_LEN (16)

typedef struct {
    timeMS_t time_ms;
    void* cbData;
    void (*cb)(void*);
    char TAG[MAX_TAG_LEN];
} OneShotTimerParams_t;

typedef struct {
    pthread_t thread;
    OneShotTimerParams_t* params;  // store allocated pointer for cleanup if needed
} OneShotTimer_t;

typedef enum {
    TIMER_SUCCESS = 0,
    TIMER_FAILED_LAUNCH,
    TIMER_FAILED_SYSCALL,
    TIMER_FAILED_JOIN
} err_OneShotTimer_t;

/* ================================================== */
/*                 INTERNAL UTILITIES                 */
/* ================================================== */

static inline void sleep_ms(timeMS_t ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

static void* Task_OneShotTimer(void* args) {
    OneShotTimerParams_t* params = (OneShotTimerParams_t*)args;
    sleep_ms(params->time_ms);
    LOG("%s: Callback Fired", params->TAG);
    params->cb(params->cbData);
    free(params);  // cleanup
    return NULL;
}

/* ================================================== */
/*                 PUBLIC API                         */
/* ================================================== */

// Launch a one-shot timer and store the thread in the timer handle
// Launch a one-shot timer and store the thread in the timer handle
static inline err_OneShotTimer_t LaunchOneShotTimer(
    OneShotTimer_t* timer,
    timeMS_t time_ms,
    void (*cb)(void*),
    void* cbData,
    const char* TAG
) 
{
    ASSERT_COMMON(timer != NULL, "Timer struct is null", TIMER_FAILED_SYSCALL);
    ASSERT_COMMON(cb    != NULL, "Timer callback is null", TIMER_FAILED_LAUNCH);

    OneShotTimerParams_t* params = (OneShotTimerParams_t*)malloc(sizeof(*params));
    ASSERT_COMMON(params != NULL,
        "Failed to allocate memory for one-shot timer params", TIMER_FAILED_SYSCALL);

    params->time_ms = time_ms;
    params->cb      = cb;
    params->cbData  = cbData;

    // Safe tag copy: bounded and always NUL-terminated
#if MAX_TAG_LEN > 0
    if (TAG) {
        size_t n = strnlen(TAG, MAX_TAG_LEN - 1);
        memcpy(params->TAG, TAG, n);
        params->TAG[n] = '\0';
    } else {
        params->TAG[0] = '\0';
    }
#endif

    int ret = pthread_create(&timer->thread, NULL, Task_OneShotTimer, (void*)params);
    ASSERT_COMMON(ret == 0,
        "Failed to launch one-shot timer thread", TIMER_FAILED_LAUNCH);

    // Decide ownership: see notes below
    timer->params = params;

    LOG("%s: Timer Launched", params->TAG);
    return TIMER_SUCCESS;
}
// Optional: wait for the one-shot timer to complete (joinable version)
static inline err_OneShotTimer_t WaitForOneShotTimer(OneShotTimer_t* timer) {
    ASSERT_COMMON(timer != NULL, "Timer handle is null", TIMER_FAILED_SYSCALL);

    int ret = pthread_join(timer->thread, NULL);
    ASSERT_COMMON(ret == 0,
        "Failed to join one-shot timer thread", TIMER_FAILED_JOIN);

    timer->params = NULL;  // already freed in task
    return TIMER_SUCCESS;
}

#endif // ONE_SHOT_TIMER_H
