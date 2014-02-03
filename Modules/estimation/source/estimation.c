/* *
 *
 *
 * */

/* Includes */
#include "estimation.h"

/* Private Defines */

/* Private Typedefs */

/* Global variable defines */
volatile Estimation_State_Struct Estimation_State;

/* Private function defines */
static void sendSensordata(void);
static void CreateMessage(KFly_Command_Type cmd, uint8_t *message, uint8_t size);

void EstimationInit(void)
{
	Estimation_State.q.q0 = 1.0f;
	Estimation_State.q.q1 = 0.0f;
	Estimation_State.q.q2 = 0.0f;
	Estimation_State.q.q3 = 0.0f;
	Estimation_State.w.x = 0.0f;
	Estimation_State.w.y = 0.0f;
	Estimation_State.w.z = 0.0f;
	Estimation_State.wb.x = 0.0f;
	Estimation_State.wb.y = 0.0f;
	Estimation_State.wb.z = 0.0f;

	xTaskCreate(vTaskRunEstimation,
			"Estimation",
			(1024 * 16) / 4, // 16kB
			0,
			configMAX_PRIORITIES - 1,
			0);
}

void vTaskRunEstimation(void *pvParameters)
{
	int i = 0;

	while(1)
	{
		/* Wait until the sensors have delivered new data */
		xSemaphoreTake(NewMeasurementAvaiable, portMAX_DELAY);

		sendSensordata();

	}
}

static void sendSensordata(void)
{
	f2bArray s_q[4];
	sint2bArray s_m[9];

	s_q[0].value = q0;
	s_q[1].value = q1;
	s_q[2].value = q2;
	s_q[3].value = q3;

	s_m[0].value = Sensor_Data.acc_x;
	s_m[1].value = Sensor_Data.acc_y;
	s_m[2].value = Sensor_Data.acc_z;
	s_m[3].value = Sensor_Data.gyro_x;
	s_m[4].value = Sensor_Data.gyro_y;
	s_m[5].value = Sensor_Data.gyro_z;
	s_m[6].value = Sensor_Data.mag_x;
	s_m[7].value = Sensor_Data.mag_y;
	s_m[8].value = Sensor_Data.mag_z;

	uint8_t msg[34];
	int i;
	for (i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			msg[i*4 + j] = s_q[i].data[j];
		}
	}

	for (int k = 0; k < 9; k++)
	{
		for (int j = 0; j < 2; j++)
		{
			msg[i*4 + k*2 + j] = s_m[k].data[j];
		}
	}

	CreateMessage(Cmd_GetSensorData, msg, 34);
}

static void CreateMessage(KFly_Command_Type cmd, uint8_t *message, uint8_t size)
{
	uint8_t msg[64];

	int i = 0;

	msg[i++] = SYNC_BYTE;
	msg[i++] = cmd;
	msg[i++] = size;
	msg[i++] = CRC8(msg, 3);

	for (int j = 0; j < size; j++)
	{
		msg[i++] = message[j];
	}

	msg[i] = (uint8_t)(CRC16(msg, i) >> 8);
	i++;
	msg[i] = (uint8_t)(CRC16(msg, i - 1));


	xUSBSendData(&msg[0], 1);
	for (int j = 1; j <= i; j++)
	{
		xUSBSendData(&msg[j], 1);

		if (msg[j] == SYNC_BYTE)
			xUSBSendData(&msg[j], 1);
	}
}