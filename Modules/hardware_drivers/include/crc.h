#ifndef __CRC_H
#define __CRC_H

/* Standard includes */
#include "stm32f4xx.h"

/* System includes */

/* Scheduler includes. */

/* KFly includes */

/* Includes */

/* Defines */

/* Global variable defines */

/* Typedefs */

/* Global function defines */
uint8_t CRC8(uint8_t *, uint32_t);
uint8_t CRC8_step(uint8_t, uint8_t);
uint16_t CRC16(uint8_t *, uint32_t);
uint16_t CRC16_step(uint8_t, uint16_t);

#endif
