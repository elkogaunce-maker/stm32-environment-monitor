#include "Serial.h"
#include <stdio.h>
#include "Ring_Buffer.h"
#include "Queue.h"

#define SERIAL_COMMAND_QUEUE_DEPTH 4

static RingBuffer Serial_RxBuffer;
static Queue Serial_CommandQueue;
static char Serial_CommandStorage[SERIAL_COMMAND_QUEUE_DEPTH][SERIAL_COMMAND_SIZE];
static volatile uint8_t Serial_OverflowFlag;

void Serial_Init(void)
{
	RB_Init(&Serial_RxBuffer);
	Queue_Init(&Serial_CommandQueue,
	           Serial_CommandStorage,
	           SERIAL_COMMAND_QUEUE_DEPTH,
	           SERIAL_COMMAND_SIZE);
	Serial_OverflowFlag = 0;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b ;
	USART_Init(USART1,&USART_InitStructure);
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_Cmd(USART1,ENABLE);
	
}
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1,Byte);
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
}

void Serial_SendARRay(const uint8_t *Array,uint16_t Length)
{
	uint16_t i;
	for(i = 0;i < Length;i ++)
	{
		Serial_SendByte(Array[i]);
	}
}
void Serial_SendString(const char *String)
{
	uint8_t i;
	for(i = 0;String[i] != 0;i++)
	{
		Serial_SendByte(String[i]);
	}
}
//为后面的Serial_SendNumber做铺垫
uint32_t Serial_Pow(uint32_t X,uint32_t Y)
{
	uint32_t Result = 1;
	while(Y--)
	{
		Result *= X;
	}
	return Result;
}

void Serial_SendNumber(uint32_t Number,uint8_t Length)
{
	uint8_t i;
	for(i = 0;i < Length;i++)
	{
		Serial_SendByte(Number/Serial_Pow(10,Length-i-1)%10 + 0x30);//ASCII字符转化
	}
}

//void Serial_SentByte(uint8_t ch)
//{

//    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
//    USART_SendData(USART1, ch);
//}

void Serial_SendPacket(void)
{
	Serial_SendByte(0xFF);
	Serial_SendByte(0xFE);
}

bool Serial_ReadCommand(char *command, uint16_t buffer_size)
{
	if(command == NULL || buffer_size < SERIAL_COMMAND_SIZE)
	{
		return false;
	}

	return Queue_Read(&Serial_CommandQueue, command);
}

uint8_t Serial_GetOverflowFlag(void)
{
	uint8_t overflow = Serial_OverflowFlag;
	Serial_OverflowFlag = 0;
	return overflow;
}

void Serial_RxTask(void)
{
    static uint8_t RxState = 0;
    static uint8_t pRxPacket = 0;
	static char RxPacket[SERIAL_COMMAND_SIZE];
    uint8_t RxData;

    while (RB_Get(&Serial_RxBuffer, &RxData))
    {
        if (RxState == 0)
        {
            if (RxData == '@')
            {
                RxState = 1;
                pRxPacket = 0;
            }
        }
        else if (RxState == 1)
        {
            if (RxData == '\r')
            {
                RxState = 2;
            }
            else
            {
				if (pRxPacket < SERIAL_COMMAND_SIZE - 1)
                {
					RxPacket[pRxPacket++] = (char)RxData;
                }
                else
                {
                    RxState = 0;
                    pRxPacket = 0;
                }
            }
        }
        else if (RxState == 2)
        {
            if (RxData == '\n')
            {
				RxPacket[pRxPacket] = '\0';
				if(!Queue_write(&Serial_CommandQueue, RxPacket))
				{
					Serial_OverflowFlag = 1;
				}
                RxState = 0;
                pRxPacket = 0;
				continue;
            }

            RxState = 0;
            pRxPacket = 0;
        }
    }
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t RxData = (uint8_t)USART_ReceiveData(USART1);
		if(!RB_Put(&Serial_RxBuffer, RxData))
		{
			Serial_OverflowFlag = 1;
		}
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
