#ifndef SIMPLE_FIFO_H
#define SIMPLE_FIFO_H

/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "SafeMem.h"
#include <stdbool.h>
#include "Assert_Common.h"

/* ================================================== */
/*                    ENUMS & TYPES                   */
/* ================================================== */
typedef enum {
    SIMPLE_FIFO_SUCCESS = 0,
    SIMPLE_FIFO_FAIL,
    SIMPLE_FIFO_FAIL_INIT,
    SIMPLE_FIFO_FAIL_PUSH,
    SIMPLE_FIFO_FAIL_POP,
    SIMPLE_FIFO_FULL,
    SIMPLE_FIFO_EMPTY,
}err_SimpleFifo_t;

typedef struct {
    uint8_t* buf;
    size_t head, tail, capacity, elementSize, size;
    //size_t elements;
}SimpleFifo_t;

typedef size_t SimpleFifoItr_t ;

/* ================================================== */
/*                 MACRO FUNC  DEFINITIONS            */
/* ================================================== */
#define SIMPLE_FIFO_ITR_END (SIZE_MAX)

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

static inline size_t GetNextHead(SimpleFifo_t* fifo){
    return (fifo->head + 1) % fifo->size;
}

static inline size_t GetNextTail(SimpleFifo_t* fifo){
    return ((fifo->tail + 1) % fifo->size);
}

static inline SimpleFifoItr_t SimpleFifo_GetItr(SimpleFifo_t* fifo){
    return fifo->tail;
}

static inline SimpleFifoItr_t SimpleFifo_GetItrNext(SimpleFifo_t* fifo, SimpleFifoItr_t itr){
    SimpleFifoItr_t nextitr = ((itr + 1) % fifo->size);
    return nextitr == fifo->head ? SIMPLE_FIFO_ITR_END : nextitr;
}

//posix complaint returns vals
static inline err_SimpleFifo_t SimpleFifo_GetItrVal(SimpleFifo_t* fifo,  SimpleFifoItr_t itr, void* buf, size_t bufSize){
    ASSERT_COMMON(bufSize >= fifo->elementSize, "buf too small", SIMPLE_FIFO_FAIL);
    memcpy(buf, fifo->buf + (itr * fifo->elementSize), fifo->elementSize);
    return SIMPLE_FIFO_SUCCESS;
}

static inline bool SimpleFifo_IsEnd(SimpleFifoItr_t itr){
    return itr == SIMPLE_FIFO_ITR_END;
}

static inline bool SimpleFifo_IsEmpty(SimpleFifo_t* fifo) {
    return (fifo->head == fifo->tail);
}

static inline bool SimpleFifo_IsFull(SimpleFifo_t* fifo) {
    return (GetNextHead(fifo) == fifo->tail);
}

static inline err_SimpleFifo_t SimpleFifo_Push(SimpleFifo_t* fifo, void* dataIn) {
    if (SimpleFifo_IsFull(fifo)) {
        return SIMPLE_FIFO_FULL;  // FIFO full
    }

    memcpy(fifo->buf + (fifo->head * fifo->elementSize), dataIn, fifo->elementSize);
    fifo->head = GetNextHead(fifo);
    return SIMPLE_FIFO_SUCCESS;
}

static inline err_SimpleFifo_t SimpleFifo_Pop(SimpleFifo_t* fifo, void* dataOut) {
    if (SimpleFifo_IsEmpty(fifo)) {
        return SIMPLE_FIFO_EMPTY;  // FIFO empty
    }
    memcpy(dataOut, fifo->buf + (fifo->tail * fifo->elementSize), fifo->elementSize);
    fifo->tail = GetNextTail(fifo);
    return SIMPLE_FIFO_SUCCESS;
}

static inline err_SimpleFifo_t SimpleFifoInitNewBuf(SimpleFifo_t* fifo, void* newBuf){
    fifo->buf =  newBuf;
    return SIMPLE_FIFO_SUCCESS;
}

__attribute__((nonnull(1, 2))) static inline err_SimpleFifo_t SimpleFifo_Static_Init(SimpleFifo_t* fifo, 
        void* buf, size_t bufSize, size_t capacity, size_t elementSize) {

    //CHECKS
    if(fifo == NULL || buf == NULL){ 
        LOG("Got invalid simple fifo params"); return SIMPLE_FIFO_FAIL; }

    //not enough mem to support
    if(bufSize < ((capacity + 1) * elementSize)){ 
        LOG("Got invalid simple fifo params"); return SIMPLE_FIFO_FAIL; }

    //Init ritual
    memset(fifo, 0, sizeof(SimpleFifo_t));//clean out params
    memset(buf, 0, bufSize);
    fifo->buf = buf;
    fifo->size = capacity + 1;
    fifo->capacity = capacity;
    fifo->elementSize = elementSize;
    return SIMPLE_FIFO_SUCCESS;
}

__attribute__((nonnull(1))) static inline err_SimpleFifo_t SimpleFifo_Dynamic_Init(SimpleFifo_t** pfifoptr,
        size_t capacity, size_t elementSize) {

    if(pfifoptr == NULL || capacity == 0 || elementSize == 0){
        LOG("Got invalid simple fifo params"); return SIMPLE_FIFO_FAIL;}

    SimpleFifo_t* fifoBuf = (SimpleFifo_t*)xMalloc(sizeof(SimpleFifo_t));
    if(fifoBuf == NULL){
        LOG("Fialed to get mem for pfifo strucure"); return SIMPLE_FIFO_FAIL; }

    fifoBuf->size = capacity + 1; //bc ring buf logic
    fifoBuf->elementSize = elementSize;
    fifoBuf->capacity = capacity;
                                  //
    fifoBuf->buf = xMalloc(fifoBuf->size * fifoBuf->elementSize);
    if(fifoBuf->buf == NULL){ xFree(fifoBuf); return SIMPLE_FIFO_FAIL; }

    fifoBuf->head = 0;
    fifoBuf->tail = 0;
    //fifoBuf->elements = 0;
    *pfifoptr = fifoBuf;
    return SIMPLE_FIFO_SUCCESS;
}

static inline err_SimpleFifo_t SimpleFifo_CleanUp(SimpleFifo_t* fifo){
    xFree(fifo->buf);
    xFree(fifo);
    return SIMPLE_FIFO_SUCCESS;
}

#endif
