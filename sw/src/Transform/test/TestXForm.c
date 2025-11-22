#include "../inc/Transform.h"
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include <stdlib.h>
#include <unistd.h>

#define SLEEP_TIME (3)

int main(void) {
    LOG("Starting XForm Test");

    ASSERT_COMMON_POSIX(Transform_Init(), "Faield to init Xform");

    FOR_LOOP_COMMON(i, SLEEP_TIME) {
        sleep(1);
        LOG("Slept for %d sec", i + 1);
    }

    ASSERT_COMMON_POSIX(Transform_Dtr(), "Failed to kill tranform service");
    LOG("Ending XForm Test");
    return EXIT_SUCCESS;
}
