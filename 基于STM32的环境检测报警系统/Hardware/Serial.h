#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdio.h>

extern char Serial_RxPacket[];
extern volatile uint8_t Serial_RxFlag;


void Serial_Init(void);
void Serial_SendByte(uint8_t Byte);
void Serial_SentByte(uint8_t Byte);
void Serial_SendARRay(uint8_t *Array,uint16_t Length);
void Serial_SendString(char *Sring);
uint32_t Serial_Pow(uint32_t X,uint32_t Y);
void Serial_SendNumber(uint32_t Number,uint8_t Length);
uint8_t Serial_GetRxFlag(void);
void Serial_RxTask(void);
void Serial_SendPacket(void);


#endif
