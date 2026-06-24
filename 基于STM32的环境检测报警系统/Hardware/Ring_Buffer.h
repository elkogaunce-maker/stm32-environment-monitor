#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RB_SIZE 256  //环形缓冲区64字节太少了，容易串口溢出导致数据丢失

typedef struct
{
    uint8_t buffer[RB_SIZE];
    uint16_t volatile head;  //写入位置
    uint16_t volatile tail;  //读取位置
} RingBuffer;

void RB_Init(RingBuffer *rb);
bool RB_IsEmpty(const RingBuffer *rb);
bool RB_IsFull(const RingBuffer *rb);
bool RB_Put(RingBuffer *rb, uint8_t data);
bool RB_Get(RingBuffer *rb, uint8_t *data);

#endif
