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
#include "semphr.h"

/* KFly includes */

/* Driver Includes */
#include "serialmanager.h"
#include "timer.h"
#include "i2c.h"
#include "uart.h"
#include "mpu6050.h"
#include "hmc5983.h"
#include "crc.h"
#include "pwm.h"
#include "spi.h"
#include "ext_flash.h"
#include "sensor_read.h"
#include "estimation.h"
#include "circularbuffer.h"

/* Includes */
#include "led.h"
#include "linear_algebra.h"


void main(void);

#endif
