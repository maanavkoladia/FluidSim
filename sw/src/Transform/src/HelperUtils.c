#include <pthread.h>

static pthread_mutex_t xform_sim_teardown_mutex = PTHREAD_MUTEX_INITIALIZER;

void Transform_SimEngine_WaitFor_Teardown(void) {
    pthread_mutex_lock(&xform_sim_teardown_mutex);
}

void Transform_SimEngine_Signal_TeardownComplete(void) {
    pthread_mutex_unlock(&xform_sim_teardown_mutex);
}
