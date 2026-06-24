#include "Queue.h"
#include <string.h>

bool Queue_Init(Queue *queue, void *storage, size_t capacity, size_t item_size)
{
	if(queue == NULL || storage == NULL || capacity == 0 || item_size == 0)
	{
		return false;
	}
	
	queue->Buffer = (uint8_t *)storage;
	queue->capacity = capacity;
	queue->item_size = item_size;
	queue->head = 0;
	queue->tail = 0;
	queue->count = 0;
	return true;
}

bool Queue_Empty(const Queue *queue)
{
	return queue == NULL || queue->count == 0;
}

bool Queue_Full(const Queue *queue)
{
	return queue != NULL && queue->count == queue->capacity;
}

bool Queue_write(Queue *queue, const void *item)
{
	if(queue == NULL || item == NULL || Queue_Full(queue))
	{
		return false;
	}
	
	memcpy(queue->Buffer + queue->head * queue->item_size,
	       item,
	       queue->item_size);
	queue->head = (queue->head + 1) % queue->capacity;
	queue->count++;
	return true;
}

bool Queue_Read(Queue *queue, void *item)
{
	if(queue == NULL || item == NULL || Queue_Empty(queue))
	{
		return false;
	}
	
	memcpy(item,
	       queue->Buffer + queue->tail * queue->item_size,
	       queue->item_size);
	queue->tail = (queue->tail + 1) % queue->capacity;
	queue->count--;
	return true;
}



