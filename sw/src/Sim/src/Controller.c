#include "Controller.h"
#include "../inc/Sim.h"
#include "Assert_Common.h"
#include <pthread.h>
#include <unistd.h>

#define TASK_CONTROLLER_RET (NULL)

pthread_t controller_th;

void* Task_Controller(void* pvArgs) {
    LOG("Task_Controller Started Up");
    while (1) {
        sleep(2);
        LOG("Task Controller Still Sleeping bc not implemented");
    }
    // timestep,
    // run sim interation, maybe pass in buffer that will be passed to opengl
    //
    return TASK_CONTROLLER_RET;
}

sim_err_t ControllerStart(void) {
    ASSERT_COMMON_POSIX(
        pthread_create(&controller_th, NULL, Task_Controller, NULL),
        "Failed to Launch controller thread");
    return SIM_SUCCESS;
}

sim_err_t ControllerStop(void) {
    return SIM_SUCCESS;
}

sim_err_t ControllerJoin(void) {
    ASSERT_COMMON_POSIX(pthread_join(controller_th, NULL),
                        "Fialed to join controller thread");
    return SIM_SUCCESS;
}
