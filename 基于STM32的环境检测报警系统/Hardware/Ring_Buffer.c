#include "Ring_Buffer.h"

void RB_Init( RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

bool RB_IsEmpty(const RingBuffer *rb)
{
    return rb->head == rb->tail;
}

bool RB_IsFull(const RingBuffer *rb)
{
    return ((rb->head + 1) % RB_SIZE) == rb->tail;
}

bool RB_Put(RingBuffer *rb, uint8_t data)
{
    if (RB_IsFull(rb))
    {
        return false;
    }

    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % RB_SIZE;

    return true;
}

bool RB_Get(RingBuffer *rb, uint8_t *data)
{
    if (RB_IsEmpty(rb))
    {
        return false;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RB_SIZE;

    return true;
}
