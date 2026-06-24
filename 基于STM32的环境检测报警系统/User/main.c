#include "AppController.h"
#include "AppConfig.h"
#include "Delay.h"

int main(void)
{
	AppController_Init();

	while(1)
	{
		Sensor_Update();

		Communication_Handle();

		Alarm_Handle();

		Display_Update();

		Delay_ms(APP_LOOP_PERIOD_MS);
	}
}
