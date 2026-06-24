#ifndef __QUEUE_H
#define __QUEUE_H


#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint8_t *Buffer;
	size_t capacity;
	size_t item_size;
	size_t head;
	size_t tail;
	size_t count;
}Queue;

bool Queue_Init(Queue *queue, void *storage, size_t capacity, size_t item_size);
bool Queue_Empty(const Queue *queue);
bool Queue_Full(const Queue *queue);
bool Queue_write(Queue *queue, const void *item);
bool Queue_Read(Queue *queue, void *item);


#endif
