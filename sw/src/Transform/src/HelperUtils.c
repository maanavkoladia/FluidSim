#include "HelperUtils.h"
#include "../inc/Transform.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include "ForLoop.h"
#include "LFfifo.h"
#include <pthread.h>
#include <stdint.h>

static pthread_mutex_t xform_sim_teardown_mutex = PTHREAD_MUTEX_INITIALIZER;

void Transform_SimEngine_WaitFor_Teardown(void) {
    pthread_mutex_lock(&xform_sim_teardown_mutex);
}

void Transform_SimEngine_Signal_TeardownComplete(void) {
    pthread_mutex_unlock(&xform_sim_teardown_mutex);
}

transform_err_t Init_RenderBuf_Colors(SimSnap_t* pSnap) {
    return TRANSFORM_SUCCESS;
}
