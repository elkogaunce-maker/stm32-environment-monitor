#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RB_SIZE 64

typedef struct
{
    uint8_t buffer[RB_SIZE];
    uint16_t head;  //写入位置
    uint16_t tail;  //读取位置
} RingBuffer;

void RB_Init(RingBuffer *rb);
bool RB_IsEmpty(RingBuffer *rb);
bool RB_IsFull(RingBuffer *rb);
bool RB_Put(RingBuffer *rb, uint8_t data);
bool RB_Get(RingBuffer *rb, uint8_t *data);

#endif
