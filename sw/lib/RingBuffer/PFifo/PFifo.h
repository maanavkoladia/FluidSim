#ifndef PFIFO_C
#define PFIFO_C

/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stddef.h>
#include <stdint.h>
#include <semaphore.h>

/* ================================================== */
/*                      DEFINES                       */
/* ================================================== */
#define PFIFO_API

typedef enum {
    PFIFO_SUCCESS,
    PFIFO_FREE_FAIL,
    PFIFO_PUSH_FAIL,
    PFIFO_POP_FAIL, 
    PFIFO_POP_FAIL_FIFO_EMPTY
} err_pFifo_t;

typedef struct {
    size_t head_pFifo, tail_pFifo, size_pFifo;
    uint8_t* dataBuf_pFifo;
    size_t elem_size;
    sem_t* mutex, *SlotsAvailable, *DataAvailable;
}pFifo_t;

/* ================================================== */
/*                 FUNCTION PROTOTYPES                */
/* ================================================== */

PFIFO_API pFifo_t* pFifo_Create(size_t datasize, size_t fifoSize);

PFIFO_API err_pFifo_t pFifoFree(pFifo_t* fifo);

PFIFO_API err_pFifo_t pFifoPush(pFifo_t* pfifo, void* data);

PFIFO_API err_pFifo_t pFifoPop(pFifo_t* pfifo, void* data);

PFIFO_API err_pFifo_t pFifoTryPop(pFifo_t* pfifo, void* data);

PFIFO_API uint32_t pFifoSize(pFifo_t* fifo );

PFIFO_API size_t pFifoElemCount(pFifo_t* fifo);

PFIFO_API void pFifoFlush(pFifo_t* fifo);

#endif // !PFIFO_C
