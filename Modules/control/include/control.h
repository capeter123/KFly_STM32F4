#ifndef __CONTROL_H
#define __CONTROL_H

/* Standard includes */
#include <math.h>
#include "stm32f4xx.h"

/* System includes */
#include "pwm.h"
#include "pid.h"

/* Scheduler includes */

/* KFly includes */

/* Includes */

/* Defines */
#define RATE_PI_OFFSET			0
#define ATTITUDE_PI_OFFSET		3
#define VELOCITY_PI_OFFSET		6
#define POSITION_PI_OFFSET		9

#define RATE_LIMIT_OFFSET		0
#define ATTITUDE_LIMIT_OFFSET	12
#define VELOCITY_LIMIT_OFFSET	32
#define POSITION_LIMIT_OFFSET	40

#define RATE_LIMIT_COUNT		12
#define ATTITUDE_LIMIT_COUNT	20
#define VELOCITY_LIMIT_COUNT	8
#define POSITION_LIMIT_COUNT	0

/* Sizes */
#define OUTPUT_MIXER_SIZE		(4*8*4)
#define CONTROL_DATA_SIZE		(12*PI_DATA_SIZE)
#define CONTROL_LIMITS_SIZE		(10*4)


/* Typedefs */
typedef enum
{
	/* The different flight modes available */
	FLIGHTMODE_DISARMED = 0,
	FLIGHTMODE_RATE,
	FLIGHTMODE_ATTITUDE,
	FLIGHTMODE_VELOCITY,
	FLIGHTMODE_POSITION,
	FLIGHTMODE_POSITION_HOLD
} Flight_Mode_Type;

typedef enum
{
	/* This determines where the system will be looking
	 * while following velocity and position commands */
	TARGET_FIXED_ANGLE = 0,
	TARGET_VELOCITY_VECTOR,
	TARGET_GOAL,
	TARGET_COORDINATE
} Target_Direcion_Type;

typedef struct
{
	/* The general structure containing control reference */
	Flight_Mode_Type mode;
	Target_Direcion_Type target;
	vector3f_t reference;
	float throttle;
} Control_Reference_Type;

typedef struct
{
	/* The controller gains, integral limits and states
	 * of each controller in the cascade scheme */
	PI_Data_Type position_controller[3];
	PI_Data_Type velocity_controller[3];
	PI_Data_Type attitude_controller[3];
	PI_Data_Type rate_controller[3];
} Control_Data_Type;

typedef struct
{
	/* The controller limits for rate, angles and velocity */
	struct /* Belongs to the rate controller */
	{	/* Rate control limits */
		float pitch;
		float roll;
		float yaw;
	} max_rate;

	struct /* Belongs to the attitude controller */
	{	/* Rate control limits whilst in attitude mode */
		float pitch;
		float roll;
		float yaw;
	} max_rate_attitude;

	struct /* Belongs to the attitude controller */
	{	/* Attitude control limits */
		float pitch;
		float roll;
	} max_angle;

	struct /* Belongs to the velocity controller */
	{	/* Velocity control limits */
		float horizontal;
		float vertical;
	} max_velocity;
} Control_Limits_Type;

typedef struct
{
	float weights[8][4];
} Output_Mixer_Type;

/* Global variable defines */

/* Global function defines */
void vInitControl(void);
void vUpdateControlAction(Control_Reference_Type *, Control_Limits_Type *, float);
Control_Data_Type *ptrGetControlData(void);
Control_Limits_Type *ptrGetControlLimits(void);
Output_Mixer_Type *ptrGetOutputMixer(void);

#endif
