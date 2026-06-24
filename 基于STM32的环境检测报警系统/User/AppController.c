#include "AppController.h"
#include "AppStateMachine.h"
#include "AppConfig.h"
#include "AD.h"
#include "Buzzer.h"
#include "Delay.h"
#include "LED.h"
#include "Motor.h"
#include "OLED.h"
#include "Serial.h"
#include <stdlib.h>
#include <string.h>

static AppStateMachine AppMachine;
static uint16_t Sensor_AdcValue;

static uint16_t ADC_ToVoltageMv(uint16_t adc)
{
	return (uint16_t)((uint32_t)adc * ADC_REF_MV / ADC_MAX_VALUE);
}

static uint16_t ADC_GetAverageValue(uint8_t sample_count)
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

static void Serial_SendSignedNumber(int32_t number, uint8_t length)
{
	if(number < 0)
	{
		Serial_SendByte('-');
		Serial_SendNumber((uint32_t)(-number), length);
	}
	else
	{
		Serial_SendNumber((uint32_t)number, length);
	}
}

static const char *App_GetStateName(void)
{
	switch(AppMachine.state)
	{
		case APP_STATE_AUTO_MONITOR:
			return "AUTO_MONITOR";
		case APP_STATE_AUTO_ALARM:
			return "AUTO_ALARM";
		case APP_STATE_MANUAL_ON:
			return "MANUAL_ON";
		case APP_STATE_MANUAL_OFF:
			return "MANUAL_OFF";
		default:
			return "UNKNOWN";
	}
}

static void Serial_SendStatus(void)
{
	Serial_SendString("$STATUS,ADC=");
	Serial_SendNumber(Sensor_AdcValue, 4);
	Serial_SendString(",MV=");
	Serial_SendNumber(AppMachine.sensor_mv, 4);
	Serial_SendString(",TH=");
	Serial_SendNumber(AppMachine.threshold_mv, 4);
	Serial_SendString(",ALARM=");
	Serial_SendNumber(AppStateMachine_IsAlarmActive(&AppMachine), 1);
	Serial_SendString(",STATE=");
	Serial_SendString(App_GetStateName());
	Serial_SendString("\r\n");
}

static uint8_t String_IsNumber(const char *str)
{
	uint8_t i = 0;

	if(str == NULL || str[0] == '\0')
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

static uint8_t String_IsSignedNumber(const char *str)
{
	if(str == NULL)
	{
		return 0;
	}

	if(str[0] == '-')
	{
		return String_IsNumber(&str[1]);
	}

	return String_IsNumber(str);
}

static void Command_Parse(const char *command)
{
	long value;

	if(strcmp(command, "STATUS?") == 0)
	{
		Serial_SendStatus();
	}
	else if(strcmp(command, "VERSION?") == 0)
	{
		Serial_SendString("$VERSION=1.1.0\r\n");
	}
	else if(strncmp(command, "TH=", 3) == 0)
	{
		if(!String_IsNumber(&command[3]))
		{
			Serial_SendString("$ERR=TH_FORMAT\r\n");
			return;
		}

		value = strtol(&command[3], NULL, 10);
		if(value >= 0 && value <= ADC_REF_MV)
		{
			AppStateMachine_ProcessEvent(&AppMachine,
			                             APP_EVENT_SET_THRESHOLD,
			                             value);
			Serial_SendString("$OK,TH=");
			Serial_SendNumber(AppMachine.threshold_mv, 4);
			Serial_SendString("\r\n");
		}
		else
		{
			Serial_SendString("$ERR=TH_RANGE\r\n");
		}
	}
	else if(strcmp(command, "ALARM=ON") == 0)
	{
		AppStateMachine_ProcessEvent(&AppMachine, APP_EVENT_MANUAL_ON, 0);
		Serial_SendString("$OK,ALARM=ON\r\n");
	}
	else if(strcmp(command, "ALARM=OFF") == 0)
	{
		AppStateMachine_ProcessEvent(&AppMachine, APP_EVENT_MANUAL_OFF, 0);
		Serial_SendString("$OK,ALARM=OFF\r\n");
	}
	else if(strcmp(command, "ALARM=AUTO") == 0)
	{
		AppStateMachine_ProcessEvent(&AppMachine, APP_EVENT_SET_AUTO, 0);
		Serial_SendString("$OK,ALARM=AUTO\r\n");
	}
	else if(strncmp(command, "MOTOR=", 6) == 0)
	{
		if(!String_IsSignedNumber(&command[6]))
		{
			Serial_SendString("$ERR=MOTOR_FORMAT\r\n");
			return;
		}

		value = strtol(&command[6], NULL, 10);
		if(value >= -100 && value <= 100)
		{
			AppStateMachine_ProcessEvent(&AppMachine,
			                             APP_EVENT_SET_MOTOR_SPEED,
			                             value);
			Serial_SendString("$OK,MOTOR=");
			Serial_SendSignedNumber(AppMachine.motor_alarm_speed, 3);
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
	uint16_t voltage_mv;

	Sensor_AdcValue = ADC_GetAverageValue(ADC_FILTER_SAMPLE_COUNT);
	voltage_mv = ADC_ToVoltageMv(Sensor_AdcValue);
	AppStateMachine_ProcessEvent(&AppMachine,
	                             APP_EVENT_SENSOR_UPDATED,
	                             voltage_mv);
}

void Display_Update(void)
{
	OLED_ShowNum(1, 5, Sensor_AdcValue, 4);
	OLED_ShowNum(2, 4, AppMachine.sensor_mv, 4);
	OLED_ShowSignedNum(3, 7, AppMachine.motor_alarm_speed, 3);

	switch(AppMachine.state)
	{
		case APP_STATE_AUTO_MONITOR:
			OLED_ShowString(4, 1, "AUTO NORMAL     ");
			break;
		case APP_STATE_AUTO_ALARM:
			OLED_ShowString(4, 1, "AUTO ALARM      ");
			break;
		case APP_STATE_MANUAL_ON:
			OLED_ShowString(4, 1, "MANUAL ON       ");
			break;
		case APP_STATE_MANUAL_OFF:
			OLED_ShowString(4, 1, "MANUAL OFF      ");
			break;
		default:
			break;
	}
}

void Communication_Handle(void)
{
	static uint8_t status_count = 0;
	char command[SERIAL_COMMAND_SIZE];

	Serial_RxTask();

	while(Serial_ReadCommand(command, sizeof(command)))
	{
		Command_Parse(command);
	}

	if(Serial_GetOverflowFlag())
	{
		Serial_SendString("$ERR=RX_OVERFLOW\r\n");
	}

	status_count++;
	if(status_count >= STATUS_REPORT_INTERVAL_TICKS)
	{
		status_count = 0;
		Serial_SendStatus();
	}
}

void AppController_Init(void)
{
	OLED_Init();
	AD_Init();
	LED_Init();
	Buzzer_Init();
	Serial_Init();
	Motor_Init();
	AppStateMachine_Init(&AppMachine);

	OLED_ShowString(1, 1, "AD0:");
	OLED_ShowString(2, 1, "MV:");
	OLED_ShowString(3, 1, "Speed:");
	Serial_SendString("$VERSION=1.1.0\r\n");
}

void Alarm_Handle(void)
{
	if(AppStateMachine_UpdateOutputs(&AppMachine))
	{
		Serial_SendString("$WARN=ALARM\r\n");
	}
}
