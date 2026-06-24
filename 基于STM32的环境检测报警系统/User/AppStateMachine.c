#include "AppStateMachine.h"
#include "AppConfig.h"
#include "Buzzer.h"
#include "LED.h"
#include "Motor.h"
#include <stddef.h>

static void AppStateMachine_EvaluateAutoState(AppStateMachine *machine)
{
	if(machine->state == APP_STATE_AUTO_MONITOR)
	{
		if(machine->sensor_mv > machine->threshold_mv)
		{
			machine->state = APP_STATE_AUTO_ALARM;
		}
	}
	else if(machine->state == APP_STATE_AUTO_ALARM)
	{
		if((uint32_t)machine->sensor_mv + ALARM_HYSTERESIS_MV <
		   machine->threshold_mv)
		{
			machine->state = APP_STATE_AUTO_MONITOR;
		}
	}
}

void AppStateMachine_Init(AppStateMachine *machine)
{
	if(machine == NULL)
	{
		return;
	}

	machine->state = APP_STATE_AUTO_MONITOR;
	machine->sensor_mv = 0;
	machine->threshold_mv = ALARM_THRESHOLD_MV;
	machine->motor_alarm_speed = MOTOR_ALARM_SPEED;
	machine->blink_phase = 0;
	machine->warning_count = 0;
}

void AppStateMachine_ProcessEvent(AppStateMachine *machine,
	                              AppEvent event,
	                              int32_t value)
{
	if(machine == NULL)
	{
		return;
	}

	switch(event)
	{
		case APP_EVENT_SENSOR_UPDATED:
			machine->sensor_mv = (uint16_t)value;
			AppStateMachine_EvaluateAutoState(machine);
			break;

		case APP_EVENT_SET_AUTO:
			machine->state = (machine->sensor_mv > machine->threshold_mv) ?
			                 APP_STATE_AUTO_ALARM : APP_STATE_AUTO_MONITOR;
			break;

		case APP_EVENT_MANUAL_ON:
			machine->state = APP_STATE_MANUAL_ON;
			break;

		case APP_EVENT_MANUAL_OFF:
			machine->state = APP_STATE_MANUAL_OFF;
			break;

		case APP_EVENT_SET_THRESHOLD:
			machine->threshold_mv = (uint16_t)value;
			AppStateMachine_EvaluateAutoState(machine);
			break;

		case APP_EVENT_SET_MOTOR_SPEED:
			machine->motor_alarm_speed = (int8_t)value;
			break;

		default:
			break;
	}
}

bool AppStateMachine_IsAlarmActive(const AppStateMachine *machine)
{
	if(machine == NULL)
	{
		return false;
	}

	return machine->state == APP_STATE_AUTO_ALARM ||
	       machine->state == APP_STATE_MANUAL_ON;
}

bool AppStateMachine_IsManual(const AppStateMachine *machine)
{
	if(machine == NULL)
	{
		return false;
	}

	return machine->state == APP_STATE_MANUAL_ON ||
	       machine->state == APP_STATE_MANUAL_OFF;
}

bool AppStateMachine_UpdateOutputs(AppStateMachine *machine)
{
	if(machine == NULL)
	{
		return false;
	}

	if(AppStateMachine_IsAlarmActive(machine))
	{
		machine->blink_phase = !machine->blink_phase;
		if(machine->blink_phase)
		{
			LED_ON();
			Buzzer_ON();
		}
		else
		{
			LED_OFF();
			Buzzer_OFF();
		}

		Motor_SetSpeed(machine->motor_alarm_speed);
		machine->warning_count++;
		if(machine->warning_count >= ALARM_WARNING_INTERVAL_TICKS)
		{
			machine->warning_count = 0;
			return true;
		}
	}
	else
	{
		machine->blink_phase = 0;
		machine->warning_count = 0;
		LED_OFF();
		Buzzer_OFF();
		Motor_SetSpeed(MOTOR_STOP_SPEED);
	}

	return false;
}
