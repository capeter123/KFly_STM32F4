#ifndef __STATEMACHINE_PARSERS_H
#define __STATEMACHINE_PARSERS_H

/* Standard includes */
#include "stm32f4xx.h"

/* Driver includes */
#include "serialmanager_types.h"
#include "statemachine_types.h"
#include "statemachine_generators.h"
#include "comlink.h"
#include "crc.h"
#include "pid.h"
#include "control.h"
#include "circularbuffer.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* KFly includes */

/* Includes */

/* Defines */

/* Typedefs */

/* Global variable defines */

/* Global function defines */
Parser_Type GetParser(KFly_Command_Type command);
void ParsePing(Parser_Holder_Type *pHolder);
void ParseGetRunningMode(Parser_Holder_Type *pHolder);
void ParseGetBootloaderVersion(Parser_Holder_Type *pHolder);
void ParseGetFirmwareVersion(Parser_Holder_Type *pHolder);
void ParseSaveToFlash(Parser_Holder_Type *pHolder);
void ParseGetRateControllerData(Parser_Holder_Type *pHolder);
void ParseSetRateControllerData(Parser_Holder_Type *pHolder);
void ParseGetAttitudeControllerData(Parser_Holder_Type *pHolder);
void ParseSetAttitudeControllerData(Parser_Holder_Type *pHolder);
void ParseGetVelocityControllerData(Parser_Holder_Type *pHolder);
void ParseSetVelocityControllerData(Parser_Holder_Type *pHolder);
void ParseGetPositionControllerData(Parser_Holder_Type *pHolder);
void ParseSetPositionControllerData(Parser_Holder_Type *pHolder);
void ParseGetChannelMix(Parser_Holder_Type *pHolder);
void ParseSetChannelMix(Parser_Holder_Type *pHolder);
void ParseGetRCCalibration(Parser_Holder_Type *pHolder);
void ParseSetRCCalibration(Parser_Holder_Type *pHolder);
void ParseGetRCValues(Parser_Holder_Type *pHolder);
void ParseGetSensorData(Parser_Holder_Type *pHolder);


#endif
