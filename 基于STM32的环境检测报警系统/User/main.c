#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "AD.h"
#include "LED.h"
#include "BUZZER.h"
#include "SERIAL.h"
#include "MOTOR.h"
#include <string.h>
#include <stdlib.h>
#include "AppConfig.h"

uint16_t Value;
uint16_t Sensor_VoltageMv;
uint16_t Alarm_ThresholdMv = ALARM_THRESHOLD_MV;
uint8_t Alarm_State;
uint8_t Alarm_ManualMode;
uint8_t Alarm_ManualState;
int8_t Motor_AlarmSpeed = MOTOR_ALARM_SPEED;

uint16_t ADC_ToVoltageMv(uint16_t adc)
{
	return (uint32_t)adc * ADC_REF_MV / ADC_MAX_VALUE;
}
//取8次的转化值平均处理
uint16_t ADC_GetAverageValue(uint8_t sample_count)
{
	uint8_t i;
	uint32_t sum = 0;
	
	if(sample_count == 0)
	{
		sample_count = 1;
	}
	
	for(i = 0; i < sample_count; i++)
	{
		sum += AD_Getvalue();
		Delay_ms(1);
	}
	
	return (uint16_t)(sum / sample_count);
}
//设置电机速度
void Serial_SendSignedNumber(int32_t Number, uint8_t Length)
{
	if(Number < 0)
	{
		Serial_SendByte('-');
		Serial_SendNumber((uint32_t)(-Number), Length);
	}
	else
	{
		Serial_SendNumber((uint32_t)Number, Length);
	}
}

void Serial_SendStatus(void)
{
	Serial_SendString("$STATUS,ADC=");
	Serial_SendNumber(Value, 4);
	Serial_SendString(",MV=");
	Serial_SendNumber(Sensor_VoltageMv, 4);
	Serial_SendString(",TH=");
	Serial_SendNumber(Alarm_ThresholdMv, 4);
	Serial_SendString(",ALARM=");
	Serial_SendNumber(Alarm_State, 1);
	Serial_SendString(",MODE=");
	if(Alarm_ManualMode)
	{
		Serial_SendString(Alarm_ManualState ? "MANUAL_ON" : "MANUAL_OFF");
	}
	else
	{
		Serial_SendString("AUTO");
	}
	Serial_SendString("\r\n");
}
//检查串口输入是否为纯数字
uint8_t String_IsNumber(char *str)
{
	uint8_t i = 0;
	
	if(str[0] == '\0')
	{
		return 0;
	}
	
	while(str[i] != '\0')
	{
		if(str[i] < '0' || str[i] > '9')
		{
			return 0;
		}
		i++;
	}
	
	return 1;
}

uint8_t String_IsSignedNumber(char *str)
{
	if(str[0] == '-')
	{
		return String_IsNumber(&str[1]);
	}
	
	return String_IsNumber(str);
}
//串口发送什么指令，单片机执行什么指令
void Command_Parse(char *cmd)
{
	if(strcmp(cmd, "STATUS?") == 0)
	{
		Serial_SendStatus();
	}
	else if(strcmp(cmd, "VERSION?") == 0)
	{
		Serial_SendString("$VERSION=1.0.0\r\n");
	}
	else if(strncmp(cmd, "TH=", 3) == 0)
	{
		uint16_t value;
		
		if(!String_IsNumber(&cmd[3]))
		{
			Serial_SendString("$ERR=TH_FORMAT\r\n");
			return;
		}
		
		value = (uint16_t)atoi(&cmd[3]);
		
		if(value <= ADC_REF_MV)
		{
			Alarm_ThresholdMv = value;
			Alarm_ManualMode = 0;
			Alarm_ManualState = 0;
			Alarm_State = (Sensor_VoltageMv > Alarm_ThresholdMv);
			Serial_SendString("$OK,TH=");
			Serial_SendNumber(Alarm_ThresholdMv, 4);
			Serial_SendString("\r\n");
		}
		else
		{
			Serial_SendString("$ERR=TH_RANGE\r\n");
		}
	}
	else if(strcmp(cmd, "ALARM=ON") == 0)
	{
		Alarm_ManualMode = 1;
		Alarm_ManualState = 1;
		Serial_SendString("$OK,ALARM=ON\r\n");
	}
	else if(strcmp(cmd, "ALARM=OFF") == 0)
	{
		Alarm_ManualMode = 1;
		Alarm_ManualState = 0;
		Serial_SendString("$OK,ALARM=OFF\r\n");
	}
	else if(strcmp(cmd, "ALARM=AUTO") == 0)
	{
		Alarm_ManualMode = 0;
		Alarm_State = (Sensor_VoltageMv > Alarm_ThresholdMv);
		Serial_SendString("$OK,ALARM=AUTO\r\n");
	}
	else if(strncmp(cmd, "MOTOR=", 6) == 0)
	{
		int speed;
		
		if(!String_IsSignedNumber(&cmd[6]))
		{
			Serial_SendString("$ERR=MOTOR_FORMAT\r\n");
			return;
		}
		
		speed = atoi(&cmd[6]);
		
		if(speed >= -100 && speed <= 100)
		{
			Motor_AlarmSpeed = (int8_t)speed;
			Serial_SendString("$OK,MOTOR=");
			Serial_SendSignedNumber(Motor_AlarmSpeed, 3);
			Serial_SendString("\r\n");
		}
		else
		{
			Serial_SendString("$ERR=MOTOR_RANGE\r\n");
		}
	}
	else
	{
		Serial_SendString("$ERR=UNKNOWN_CMD\r\n");
	}
}

void Sensor_Update(void)
{
	Value = ADC_GetAverageValue(ADC_FILTER_SAMPLE_COUNT);
	Sensor_VoltageMv = ADC_ToVoltageMv(Value);
}
void Display_Update(void)
{
	OLED_ShowNum(1,5,Value,4);
	OLED_ShowNum(2,4,Sensor_VoltageMv,4);
	OLED_ShowSignedNum(3,7,Motor_AlarmSpeed,3);
}
void Alarm_Handle(void)
{
	static uint8_t blink = 0;
	static uint8_t warn_count = 0;
	
	if(Alarm_ManualMode == 1)
	{
		Alarm_State = Alarm_ManualState;
	}
	else
	{
		if(Alarm_State == 0)
		{
			if(Sensor_VoltageMv > Alarm_ThresholdMv)
			{
				Alarm_State = 1;
			}
		}
		else
		{
			if((Sensor_VoltageMv + ALARM_HYSTERESIS_MV) < Alarm_ThresholdMv)
			{
				Alarm_State = 0;
			}
		}
	}
	
	if(Alarm_State)
	{
		blink = !blink;
		if(blink)
		{
			LED_ON();
			Buzzer_ON();
		}
		else
		{
			LED_OFF();
			Buzzer_OFF();
		}
		
		Motor_SetSpeed(Motor_AlarmSpeed);
		
		if(warn_count++ >= 20)
		{
			warn_count = 0;
			Serial_SendString("$WARN=ALARM\r\n");
		}
	}
	else
	{
		LED_OFF();
		Buzzer_OFF();
		Motor_SetSpeed(MOTOR_STOP_SPEED);
		warn_count = 0;
	}
}
void Communication_Handle(void)
{
	static uint8_t status_count = 0;
	
	if(Serial_GetRxFlag())
	{
		Command_Parse(Serial_RxPacket);
	}
	
	if(status_count++ >= 20)
	{
		status_count = 0;
		Serial_SendStatus();
	}
}
int main()
{

	OLED_Init();
	AD_Init();
	LED_Init();
	Buzzer_Init();
	Serial_Init();
	Motor_Init();
	
	OLED_ShowString(1,1,"AD0:");
	OLED_ShowString(2,1,"MV:");
	OLED_ShowString(3,1,"Speed:");
	
	Serial_SendString("$VERSION=1.0.0\r\n");
	
	while(1)
	{
		Sensor_Update();
		Display_Update();
		Alarm_Handle();
		Communication_Handle();
		Delay_ms(50);
	}
}
