#ifndef BARRIER_H
#define BARRIER_H

/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Assert_Common.h"
#include <stdatomic.h>

/* ================================================== */
/*                    ENUMS & TYPES                   */
/* ================================================== */

#define MAX_TAG_LEN (16)

#define BARRIER_OPEN (0)
#define BARRIER_CLOSED (1)

#define WAITED_AT_BARRIER (0)
#define SKIPPED_BARRIER (1)

typedef enum {
    BARRIER_SUCCESS = 0,
    BARRIER_INIT_FAILED,
    BARRIER_WAIT_ERR,
    BARRIER_SIGNAL_ERR,
} err_barrier_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    atomic_uint_fast64_t flag;
    size_t waiting_threads;
    char TAG[MAX_TAG_LEN];
} Barrier_t;

typedef struct {
    const char* TAG;
    const char* errMsg;
} Barrier_CBInput_t;

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

// Callback used for ASSERT_COMMON_CB
static void Barrier_CB(void* args) {
    Barrier_CBInput_t* inputs = (Barrier_CBInput_t*)args;
    LOG("%s: %s", inputs->TAG, inputs->errMsg);
}

// Initialize the barrier
static inline err_barrier_t Barrier_Init(Barrier_t* barrier, const char* tag_in) {
    // Basic guard (optional if ASSERT_COMMON_CB also checks)
    if (!barrier) return BARRIER_INIT_FAILED;

    // Copy tag safely first, so failure callbacks can print it
    if (tag_in && MAX_TAG_LEN > 0) {
        size_t n = strnlen(tag_in, MAX_TAG_LEN - 1);
        memcpy(barrier->TAG, tag_in, n);
        barrier->TAG[n] = '\0';
    } else if (MAX_TAG_LEN > 0) {
        barrier->TAG[0] = '\0';
    }

    Barrier_CBInput_t cbInputs = {
        .TAG = barrier->TAG,                // now initialized
        .errMsg = "Failed to Init Barrier"
    };

    ASSERT_COMMON_CB(
        pthread_mutex_init(&barrier->lock, NULL) == 0,
        Barrier_CB, (void*)&cbInputs, BARRIER_INIT_FAILED
    );

    ASSERT_COMMON_CB(
        pthread_cond_init(&barrier->cond, NULL) == 0,
        Barrier_CB, (void*)&cbInputs, BARRIER_INIT_FAILED
    );

    atomic_store(&barrier->flag, BARRIER_CLOSED);
    barrier->waiting_threads = 0;
    return BARRIER_SUCCESS;
}
// Wait on the barrier
static inline err_barrier_t Barrier_Wait(Barrier_t* barrier, int* waited) {
    Barrier_CBInput_t cbInputs = {
        .TAG = barrier->TAG,
        .errMsg = "Failed to Wait on Barrier"
    };

    int waitedChk = SKIPPED_BARRIER;
    pthread_mutex_lock(&barrier->lock);

    while(atomic_load(&barrier->flag) == BARRIER_CLOSED){

        barrier->waiting_threads++;
        waitedChk = WAITED_AT_BARRIER;

        ASSERT_COMMON_CB(
            pthread_cond_wait(&barrier->cond, &barrier->lock) == 0,
            Barrier_CB,
            (void*)&cbInputs,
            BARRIER_WAIT_ERR
        );

        barrier->waiting_threads--;

    }

    pthread_mutex_unlock(&barrier->lock);

    if(waited){
        *waited = waitedChk;
    }

    return BARRIER_SUCCESS;
}

static inline err_barrier_t Barrier_Close(Barrier_t* barrier){
    pthread_mutex_lock(&barrier->lock);
    atomic_store(&barrier->flag, BARRIER_CLOSED);
    pthread_mutex_unlock(&barrier->lock);
    return BARRIER_SUCCESS;
}

// Signal the barrier (wake all waiting threads)
static inline err_barrier_t Barrier_Open(Barrier_t* barrier) {
    Barrier_CBInput_t cbInputs = {
        .TAG = barrier->TAG,
        .errMsg = "Failed to Signal Barrier"
    };

    pthread_mutex_lock(&barrier->lock);
    atomic_store(&barrier->flag, BARRIER_OPEN);
    ASSERT_COMMON_CB(
        pthread_cond_broadcast(&barrier->cond) == 0,
        Barrier_CB,
        (void*)&cbInputs,
        BARRIER_SIGNAL_ERR
    );

    pthread_mutex_unlock(&barrier->lock);
    return BARRIER_SUCCESS;
}

// Destroy the barrier
static inline err_barrier_t Barrier_Dtr(Barrier_t* barrier) {
    pthread_mutex_destroy(&barrier->lock);
    pthread_cond_destroy(&barrier->cond);
    return BARRIER_SUCCESS;
}

static inline size_t Barrier_WaitingCount(Barrier_t* barrier) {
    unsigned int count;
    pthread_mutex_lock(&barrier->lock);
    count = barrier->waiting_threads;
    pthread_mutex_unlock(&barrier->lock);
    return count;
}

#endif // BARRIER_H
