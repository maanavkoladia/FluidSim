/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "PFifo.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <stdint.h>

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
#define PFIFO_CREATE_ERR (NULL)
#define PFIFO_NUM_SEMA4S_NEEDED (3)

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */


/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

static bool Enqueue(uint8_t *inData, pFifo_t* fifo){
    size_t nextptr = fifo->head_pFifo + 1 >= fifo->size_pFifo ? 0 : fifo->head_pFifo + 1;
    if (nextptr == fifo->tail_pFifo) { // FIFO is full
        return false;
    }

    memcpy(fifo->dataBuf_pFifo + (fifo->head_pFifo * fifo->elem_size), inData, fifo->elem_size); // Copy data into FIFO

    fifo->head_pFifo = nextptr;
    return true;
}

static bool Dequeue(uint8_t* OutData, pFifo_t* fifo){
    if (fifo->head_pFifo == fifo->tail_pFifo) { // FIFO is empty
        return false;
    }

    memcpy(OutData, fifo->dataBuf_pFifo + (fifo->tail_pFifo * fifo->elem_size), fifo->elem_size); // Copy data out of FIFO

    fifo->tail_pFifo = fifo->tail_pFifo + 1 >= fifo->size_pFifo ? 0 : fifo->tail_pFifo + 1;
    return true;
}

PFIFO_API err_pFifo_t pFifoPush(pFifo_t* pfifo, void* data){
    sem_wait(pfifo->SlotsAvailable);
    sem_wait(pfifo->mutex);

    bool sucess = Enqueue(data, pfifo);

    sem_post(pfifo->mutex);
    sem_post(pfifo->DataAvailable);
    return sucess? PFIFO_SUCCESS : PFIFO_PUSH_FAIL;
}

PFIFO_API err_pFifo_t pFifoPop(pFifo_t* pfifo, void* data){
    sem_wait(pfifo->DataAvailable);
    sem_wait(pfifo->mutex);

    bool sucess = Dequeue(data, pfifo);

    sem_post(pfifo->mutex);
    sem_post(pfifo->SlotsAvailable);
    return sucess? PFIFO_SUCCESS : PFIFO_POP_FAIL;
}

PFIFO_API err_pFifo_t pFifoTryPop(pFifo_t* pfifo, void* data){
    if(sem_trywait(pfifo->DataAvailable) != EXIT_SUCCESS){
        return PFIFO_POP_FAIL_FIFO_EMPTY;
    }

    sem_wait(pfifo->mutex);

    bool sucess = Dequeue(data, pfifo);

    sem_post(pfifo->mutex);
    sem_post(pfifo->SlotsAvailable);
    return sucess? PFIFO_SUCCESS : PFIFO_POP_FAIL;
}

PFIFO_API pFifo_t* pFifo_Create(size_t datasize, size_t numOfElements) {
    if (datasize < 1 || numOfElements < 1) {
        return PFIFO_CREATE_ERR;
    }
    int fifoSize = numOfElements + 1;
    pFifo_t* pnewFifo = NULL;
    uint8_t* dataBuf = NULL;
    sem_t* semBuf = NULL;

    pnewFifo = (pFifo_t*)malloc(sizeof(pFifo_t));
    if (!pnewFifo) goto cleanup;

    dataBuf = (uint8_t*)malloc(datasize * (fifoSize));
    if (!dataBuf) goto cleanup;

    semBuf = (sem_t*)malloc(sizeof(sem_t) * PFIFO_NUM_SEMA4S_NEEDED);
    if (!semBuf) goto cleanup;

    pnewFifo->dataBuf_pFifo = dataBuf;
    pnewFifo->head_pFifo = 0;
    pnewFifo->tail_pFifo = 0;
    pnewFifo->size_pFifo = fifoSize;
    pnewFifo->elem_size = datasize;

    pnewFifo->mutex = &semBuf[0];
    pnewFifo->SlotsAvailable = &semBuf[1];
    pnewFifo->DataAvailable = &semBuf[2];

    if (sem_init(pnewFifo->mutex, 0, 1) != 0) goto cleanup;
    if (sem_init(pnewFifo->DataAvailable, 0, 0) != 0) goto cleanup;
    if (sem_init(pnewFifo->SlotsAvailable, 0, fifoSize-1) != 0) goto cleanup;

    return pnewFifo;

cleanup:
    if (semBuf) {
        // If any sem_init fails, destroy any initialized semaphores
        sem_destroy(&semBuf[0]);
        sem_destroy(&semBuf[1]);
        sem_destroy(&semBuf[2]);
        free(semBuf);
    }
    free(dataBuf);
    free(pnewFifo);
    return PFIFO_CREATE_ERR;
}

PFIFO_API err_pFifo_t pFifoFree(pFifo_t* fifo){
    free(fifo->dataBuf_pFifo);
    free(fifo->mutex);
    free(fifo);
    return PFIFO_SUCCESS;
}

PFIFO_API uint32_t pFifoSize(pFifo_t* fifo ){
    return fifo->size_pFifo - 1;
}

PFIFO_API size_t pFifoElemCount(pFifo_t* fifo) {
    if (!fifo) return 0;

    if (fifo->head_pFifo >= fifo->tail_pFifo) {
        return fifo->head_pFifo - fifo->tail_pFifo;
    } else {
        return fifo->size_pFifo - (fifo->tail_pFifo - fifo->head_pFifo);
    }
}

PFIFO_API void pFifoFlush(pFifo_t* fifo){
    uint8_t dumpBuf[fifo->elem_size];
    while(pFifoTryPop(fifo, dumpBuf) != PFIFO_POP_FAIL_FIFO_EMPTY){}
}


