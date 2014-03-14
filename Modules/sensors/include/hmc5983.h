#ifndef __HMC5983_H
#define __HMC5983_H

/* Standard includes */
#include "stm32f4xx.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* KFly includes */

/* Includes */
#include "i2c.h"

/* Defines */
#define HMC5983_ADDRESS            0x1E

#define HMC5983_RA_CONFIG_A        0x00
#define HMC5983_RA_CONFIG_B        0x01
#define HMC5983_RA_MODE            0x02
#define HMC5983_RA_DATAX_H         0x03
#define HMC5983_RA_DATAX_L         0x04
#define HMC5983_RA_DATAZ_H         0x05
#define HMC5983_RA_DATAZ_L         0x06
#define HMC5983_RA_DATAY_H         0x07
#define HMC5983_RA_DATAY_L         0x08
#define HMC5983_RA_STATUS          0x09
#define HMC5983_RA_ID_A            0x0A
#define HMC5983_RA_ID_B            0x0B
#define HMC5983_RA_ID_C            0x0C

/* Global variable defines */

/* Typedefs */

/* Global function defines */
void HMC5983Init(void);
ErrorStatus GetHMC5983ID(uint8_t *);


#endif
