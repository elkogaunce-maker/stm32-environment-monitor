#include "stm32f10x.h"                 
#include "Delay.h"

void Buzzer_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   //推挽输出模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
}

void Buzzer_ON(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_7);
}

void Buzzer_OFF(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_7);
}

void Buzzer_Turn(void)
{
	if(GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_7) == 0)
	{
		GPIO_SetBits(GPIOA,GPIO_Pin_7);
	}
	else
	{
		GPIO_ResetBits(GPIOA,GPIO_Pin_7);
	}
}
