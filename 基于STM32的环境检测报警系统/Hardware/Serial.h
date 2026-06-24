#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdbool.h>
#include <stdio.h>

#define SERIAL_COMMAND_SIZE 100

void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SentByte(uint8_t Byte);
void Serial_SendARRay(const uint8_t *Array,uint16_t Length);
void Serial_SendString(const char *Sring);
uint32_t Serial_Pow(uint32_t X,uint32_t Y);
void Serial_SendNumber(uint32_t Number,uint8_t Length);
void Serial_RxTask(void);
bool Serial_ReadCommand(char *command, uint16_t buffer_size);
uint8_t Serial_GetOverflowFlag(void);
void Serial_SendPacket(void);


#endif
