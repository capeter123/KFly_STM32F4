#ifndef __MAIN_H
#define __MAIN_H

/* Standard includes */
#include "stm32f4xx.h"

/* System includes */
#include "usbd_cdc_core.h"
#include "usbd_cdc.h"
#include "usbd_usr.h"
#include "usbd_desc.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* KFly includes */

/* Driver Incudes */
#include "comlink.h"
#include "timer.h"
#include "i2c.h"
#include "mpu6050.h"
#include "hmc5883l.h"
#include "crc.h"
#include "serialmanager.h"

/* Includes */
#include "led.h"
#include "linear_algebra.h"

#ifndef DATE
	#define DATE "no timestamp"
#endif

#define MAINVERSION	 	"1"
#define SUBVERSION 		"23"
#define KFLY_VERSION "KFly v" MAINVERSION "." SUBVERSION " (20" DATE ")\0"


void main(void);
void vTaskPrintTimer(void *);

#endif
