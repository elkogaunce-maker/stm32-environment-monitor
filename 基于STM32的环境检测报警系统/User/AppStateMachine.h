#ifndef __APP_STATE_MACHINE_H
#define __APP_STATE_MACHINE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	APP_STATE_AUTO_MONITOR,
	APP_STATE_AUTO_ALARM,
	APP_STATE_MANUAL_ON,
	APP_STATE_MANUAL_OFF
} AppState;

typedef enum
{
	APP_EVENT_SENSOR_UPDATED,
	APP_EVENT_SET_AUTO,
	APP_EVENT_MANUAL_ON,
	APP_EVENT_MANUAL_OFF,
	APP_EVENT_SET_THRESHOLD,
	APP_EVENT_SET_MOTOR_SPEED
} AppEvent;

typedef struct
{
	AppState state;
	uint16_t sensor_mv;
	uint16_t threshold_mv;
	int8_t motor_alarm_speed;
	uint8_t blink_phase;
	uint8_t warning_count;
} AppStateMachine;

void AppStateMachine_Init(AppStateMachine *machine);
void AppStateMachine_ProcessEvent(AppStateMachine *machine,
	                              AppEvent event,
	                              int32_t value);
bool AppStateMachine_UpdateOutputs(AppStateMachine *machine);
bool AppStateMachine_IsAlarmActive(const AppStateMachine *machine);
bool AppStateMachine_IsManual(const AppStateMachine *machine);

#endif
