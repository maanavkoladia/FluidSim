#include "../../Render/inc/Renderer.h"
#include "../inc/Transform.h"
#include "Assert_Common.h"
#include "ForLoop.h"
#include "LOG.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define SLEEP_TIME (3)
#define SIM_DIM_X (10)
#define SIM_DIM_Y (10)

pthread_t test_th;
velocity_t ux = -10;
velocity_t uy = -10;

#define V_GROWTH (.01)

static Cell_t** CreateCellsBuffer(uint64_t nx, uint64_t ny) {
    Cell_t** return_val = NULL;
    return_val = (Cell_t**)malloc(sizeof(Cell_t*) * nx);
    for (uint64_t i = 0; i < nx; i++) {
        return_val[i] = malloc(sizeof(Cell_t) * ny);
    }
    return return_val;
}

static SimSnap_t* CreateSimSnap(uint64_t nx, uint64_t ny) {
    SimSnap_t* res = malloc(sizeof(SimSnap_t));
    res->nx = nx;
    res->ny = ny;
    res->cells = CreateCellsBuffer(res->nx, res->ny);
    return res;
}

static void FreeCells(Cell_t** cells, uint64_t nx) {
    for (uint64_t i = 0; i < nx; i++)
        free(cells[i]);
    free(cells);
}

sim_err_t Sim_SimSnap_Yeild(SimSnap_t* pSnap) {
    ASSERT_COMMON(pSnap, "NULL snap yeild");
    FreeCells(pSnap->cells, pSnap->nx);
    free(pSnap);

    // LOG("Freed Yeild Snap");
    return SIM_SUCCESS;
}

void CreateTestSnap(SimSnap_t* pSnap) {
    ASSERT_COMMON_NOT_NULL(pSnap);
    FOR_LOOP_COMMON(i, pSnap->nx) {
        FOR_LOOP_COMMON(j, pSnap->ny) {
            pSnap->cells[i][j].ux = ux;
            pSnap->cells[i][j].uy = uy;
            pSnap->cells[i][j].p = j + (i * pSnap->nx);
        }
    }

    ux += V_GROWTH;
    uy += V_GROWTH;
    LOG("v Val: %f", ux);
}

void* Task_TestTask(void* pvArgs) {
    LOG("Started Test");
    sleep(2);
    while (1) {
        SimSnap_t* pSnap = NULL;
        pSnap = CreateSimSnap(SIM_DIM_X, SIM_DIM_Y);
        ASSERT_COMMON_NOT_NULL(pSnap);
        CreateTestSnap(pSnap);
        Transform_SendNewSimSnap(pSnap);
        usleep(10000);
    }

    return NULL;
}

int main(void) {
    LOG("Starting XForm Test");
    ASSERT_COMMON_POSIX(Transform_Init(), "Faield to init Xform");
    ASSERT_COMMON_POSIX(pthread_create(&test_th, NULL, Task_TestTask, NULL),
                        "Failed to statup to utpt eh therad");
    ASSERT_COMMON_POSIX(Render_Init(), "Fialed to init renderer");
#if defined(linux)
    pthread_join(test_th, NULL);
#endif
    ASSERT_COMMON_POSIX(Transform_Dtr(), "Failed to kill tranform service");
    LOG("Ending XForm Test");
    return EXIT_SUCCESS;
}
