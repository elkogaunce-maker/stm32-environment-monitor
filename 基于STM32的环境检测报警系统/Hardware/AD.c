#include "stm32f10x.h"                  // Device header

uint16_t AD_Value[2];

uint32_t timeout;

void AD_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);//ADC,GPIO,DMA时钟开启
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//时钟总线6分频
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;//模拟输出模式，只适用于AD模数转化
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_8,1,ADC_SampleTime_55Cycles5);
	//ADC外设，ADC采样通道，ADC采样顺序，ADC采样时间
	
	ADC_InitTypeDef ADC_InitSturucture;
	ADC_InitSturucture.ADC_ContinuousConvMode = DISABLE;
	ADC_InitSturucture.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitSturucture.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitSturucture.ADC_Mode = ADC_Mode_Independent;
	ADC_InitSturucture.ADC_NbrOfChannel = 1;
	ADC_InitSturucture.ADC_ScanConvMode = DISABLE;
	ADC_Init(ADC1,&ADC_InitSturucture);
	
	
	DMA_InitTypeDef DMA_InitSturcture;
	DMA_InitSturcture.DMA_BufferSize = 1;
	DMA_InitSturcture.DMA_DIR = DMA_DIR_PeripheralSRC;//由外设到内存
	DMA_InitSturcture.DMA_M2M = DMA_M2M_Disable;
	DMA_InitSturcture.DMA_MemoryBaseAddr = (uint32_t) AD_Value;
	DMA_InitSturcture.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//内存配置单位
	DMA_InitSturcture.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitSturcture.DMA_Mode = DMA_Mode_Normal;//正常模式
	DMA_InitSturcture.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitSturcture.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//外设配置单位
	DMA_InitSturcture.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitSturcture.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel1,&DMA_InitSturcture);
	
	DMA_Cmd(DMA1_Channel1,ENABLE);
	
	ADC_DMACmd(ADC1,ENABLE);
	ADC_Cmd(ADC1,ENABLE);
	//自动校准
	ADC_ResetCalibration(ADC1);
	
	for(timeout=0;timeout<1000000;timeout++)
	{
		if(ADC_GetResetCalibrationStatus(ADC1) == RESET)
		{
			break;
		}
	}
	if(timeout >= 1000000)
	{
		return;
	}
	
	ADC_StartCalibration(ADC1);
	
	for(timeout=0;timeout<1000000;timeout++)
	{
		 if(ADC_GetCalibrationStatus(ADC1) == RESET)
		 {
			 break;
		 }
	}
	if(timeout >= 1000000)
	{
		return ;
	}

}

uint16_t  AD_Getvalue(void)
{
	DMA_Cmd(DMA1_Channel1,DISABLE);//关闭DMA转运通道
	DMA_SetCurrDataCounter(DMA1_Channel1,1);//重启DMA转运通道
	DMA_ClearFlag(DMA1_FLAG_TC1);
	DMA_Cmd(DMA1_Channel1,ENABLE);//DMA初始化
	
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);//软件触发
	
	for(timeout=0;timeout<100000;timeout++)
	{
		if(DMA_GetFlagStatus(DMA1_FLAG_TC1) == SET)
		{
			break;
		}
	}
	if(timeout >= 100000)
	{
			 return 0;
	}
	
	DMA_ClearFlag(DMA1_FLAG_TC1);
	return AD_Value[0];
}
