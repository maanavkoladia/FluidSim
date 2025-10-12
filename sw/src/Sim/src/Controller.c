#include "Controller.h"
#include "../inc/Sim.h"
#include "Assert_Common.h"
#include "AtomicFlag.h"
#include <pthread.h>
#include <unistd.h>

#define TASK_CONTROLLER_RET (NULL)

pthread_t controller_th;
AtomicFlag_t killFlag;

#define KILL_FLAG_SET (FLAG_SET)
#define KILL_FLAG_CLEAR (FLAG_CLEAR)

#define CHECK_FLAG_STATUS(killFlag)                                                                \
    do {                                                                                           \
        if (AtomicFlag_GetStatus(&killFlag) == KILL_FLAG_SET) {                                    \
            return TASK_CONTROLLER_RET;                                                            \
        }                                                                                          \
    } while (0)

void* Task_Controller(void* pvArgs) {
    LOG("Task_Controller Started Up");
    while (1) {
        CHECK_FLAG_STATUS(killFlag);
        sleep(2);
        LOG("Task Controller Still Sleeping bc not implemented");
    }
    // timestep,
    // run sim interation, maybe pass in buffer that will be passed to opengl
    //
    return TASK_CONTROLLER_RET;
}

sim_err_t ControllerInit(void) {
    AtomicFlag_Init(&killFlag, "Controller Kill Flag", KILL_FLAG_CLEAR);
    ASSERT_COMMON_POSIX(pthread_create(&controller_th, NULL, Task_Controller, NULL),
                        "Failed to Launch controller thread");
    return SIM_SUCCESS;
}

sim_err_t ControllerStop(void) {
    AtomicFlag_UpdateStatus(&killFlag, KILL_FLAG_SET);
    ControllerJoin();
    return SIM_SUCCESS;
}

sim_err_t ControllerJoin(void) {
    ASSERT_COMMON_POSIX(pthread_join(controller_th, NULL), "Fialed to join controller thread");
    return SIM_SUCCESS;
}
