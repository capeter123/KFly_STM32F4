/* *
 *
 * All the generators for the different messages.
 * 
 * */

#include "stdint.h"
#include "statemachine_parsers.h"

/**
 * The message generation lookup table
 */
static const Generator_Type generator_lookup[128] = {
	NULL,						/* 0: 	Cmd_None 						*/
	GenerateACK,				/* 1: 	Cmd_ACK 						*/
	GeneratePing,				/* 2: 	Cmd_Ping 						*/
	NULL,						/* 3:	Cmd_DebugMessage				*/
	NULL,						/* 4:	Cmd_GetRunningMode 				*/
	NULL,						/* 5 */
	NULL,						/* 6 */
	NULL,						/* 7 */
	NULL,						/* 8 */
	NULL,						/* 9 */
	NULL,						/* 10:	Cmd_PrepareWriteFirmware 		*/
	NULL,						/* 11: 	Cmd_WriteFirmwarePackage 		*/
	NULL,						/* 12:	Cmd_WriteLastFirmwarePackage	*/
	NULL,						/* 13:	Cmd_ReadFirmwarePackage			*/
	NULL,						/* 14:	Cmd_ReadLastFirmwarePackage		*/
	NULL,						/* 15:	Cmd_NextPackage					*/
	NULL,						/* 16:	Cmd_ExitBootloader				*/
	NULL,						/* 17:	Cmd_GetBootloaderVersion		*/
	NULL,						/* 18:	Cmd_GetFirmwareVersion			*/
	NULL,						/* 19:	Cmd_SaveToFlash					*/
	NULL,						/* 20 */
	NULL,						/* 21 */
	NULL,						/* 22 */
	NULL,						/* 23 */
	NULL,						/* 24 */
	NULL,						/* 25 */
	NULL,						/* 26 */
	NULL,						/* 27 */
	NULL,						/* 28 */
	NULL,						/* 29 */
	NULL,						/* 30:	Cmd_GetRateControllerData		*/
	NULL,						/* 31:	Cmd_SetRateControllerData		*/
	NULL,						/* 32:	Cmd_GetAttitudeControllerData	*/
	NULL,						/* 33:	Cmd_SetAttitudeControllerData	*/
	NULL,						/* 34:	Cmd_GetVelocityControllerData	*/
	NULL,						/* 35: 	Cmd_SetVelocityControllerData 	*/
	NULL,						/* 36:	Cmd_GetPositionControllerData	*/
	NULL,						/* 37:	Cmd_SetPositionControllerData	*/
	NULL,						/* 38:	RESERVED						*/
	NULL,						/* 39:	Cmd_GetChannelMix				*/
	NULL,						/* 40:	Cmd_SetChannelMix				*/
	NULL,						/* 41:	Cmd_GetRCCalibration			*/
	NULL,						/* 42:	Cmd_SetRCCalibration			*/
	NULL,						/* 43:	Cmd_GetRCValues					*/
	NULL,						/* 44:	Cmd_GetSensorData				*/
	NULL,						/* 45 */
	NULL,						/* 46 */
	NULL,						/* 47 */
	NULL,						/* 48 */
	NULL,						/* 49 */
	NULL,						/* 50 */
	NULL,						/* 51 */
	NULL,						/* 52 */
	NULL,						/* 53 */
	NULL,						/* 54 */
	NULL,						/* 55 */
	NULL,						/* 56 */
	NULL,						/* 57 */
	NULL,						/* 58 */
	NULL,						/* 59 */
	NULL,						/* 60 */
	NULL,						/* 61 */
	NULL,						/* 62 */
	NULL,						/* 63 */
	NULL,						/* 64 */
	NULL,						/* 65 */
	NULL,						/* 66 */
	NULL,						/* 67 */
	NULL,						/* 68 */
	NULL,						/* 69 */
	NULL,						/* 70 */
	NULL,						/* 71 */
	NULL,						/* 72 */
	NULL,						/* 73 */
	NULL,						/* 74 */
	NULL,						/* 75 */
	NULL,						/* 76 */
	NULL,						/* 77 */
	NULL,						/* 78 */
	NULL,						/* 79 */
	NULL,						/* 80 */
	NULL,						/* 81 */
	NULL,						/* 82 */
	NULL,						/* 83 */
	NULL,						/* 84 */
	NULL,						/* 85 */
	NULL,						/* 86 */
	NULL,						/* 87 */
	NULL,						/* 88 */
	NULL,						/* 89 */
	NULL,						/* 90 */
	NULL,						/* 91 */
	NULL,						/* 92 */
	NULL,						/* 93 */
	NULL,						/* 94 */
	NULL,						/* 95 */
	NULL,						/* 96 */
	NULL,						/* 97 */
	NULL,						/* 98 */
	NULL,						/* 99 */
	NULL,						/* 100 */
	NULL,						/* 101 */
	NULL,						/* 102 */
	NULL,						/* 103 */
	NULL,						/* 104 */
	NULL,						/* 105 */
	NULL,						/* 106 */
	NULL,						/* 107 */
	NULL,						/* 108 */
	NULL,						/* 109 */
	NULL,						/* 110 */
	NULL,						/* 111 */
	NULL,						/* 112 */
	NULL,						/* 113 */
	NULL,						/* 114 */
	NULL,						/* 115 */
	NULL,						/* 116 */
	NULL,						/* 117 */
	NULL,						/* 118 */
	NULL,						/* 119 */
	NULL,						/* 120 */
	NULL,						/* 121 */
	NULL,						/* 122 */
	NULL,						/* 123 */
	NULL,						/* 124 */
	NULL,						/* 125 */
	NULL,						/* 126 */
	NULL						/* 127 */
};

 /**
  * @brief 			Generate a message for the AUX ports based on the Generators in the lookup table.
  * @details 
  * 
  * @param command 	The command to generate a message for.
  * @param Cbuff 	Pointer to the circular buffer to put the data in.
  * 
  * @return 		Return ERROR if the message didn't fit or SUCCESS if it did fit.
  */
ErrorStatus GenerateAUXMessage(KFly_Command_Type command, Circular_Buffer_Type *Cbuff)
{
	ErrorStatus status;

	/* Check so the circular buffer address is valid */
	if (Cbuff == NULL)
		return ERROR;

	/* Check so there is an available Generator function for this command */
	if (generator_lookup[command] != NULL)
	{
		/* Claim the circular buffer for writing */
		CircularBuffer_Claim(Cbuff, portMAX_DELAY);
		{
			status = generator_lookup[command](Cbuff);
		}
		CircularBuffer_Release(Cbuff);
		/* Release the circular buffer and return the status */
	}
	else
		status = ERROR;
	
	return status;
}


 /**
  * @brief 			Generate a message for the USB port based on the Generators in the lookup table.
  * @details 		This function tricks the circular buffer code into using the USB as a circular buffer.
  * 
  * @param command 	The command to generate a message for.
  * 
  * @return 		Return ERROR if the message didn't fit or SUCCESS if it did fit.
  */
ErrorStatus GenerateUSBMessage(KFly_Command_Type command)
{
	/* The two APP_* are from the USB CDC core and points to the transmitting buffer's location and index */
	extern uint8_t  APP_Rx_Buffer[]; /* Write CDC data to this buffer to send it over the USB */
	extern uint32_t APP_Rx_ptr_in;   /* Increment this pointer to initiate the transfer */
	ErrorStatus status;
	Circular_Buffer_Type temp;

	/* TODO: Check if the USB is available */
	if (xUSBQueue.bUSBAvalible == FALSE)
		return ERROR;

	/* Check so there is an available Generator function for this command */
	if (generator_lookup[command] != NULL)
	{
		/* Claim the USB for writing */
		xSemaphoreTake(xUSBQueue.USB_Write_Lock, portMAX_DELAY);
		{
			/* Fake the USB Endpoint to look like an Circular Buffer */
			temp.buffer = APP_Rx_Buffer;	/* 	Set the pointer to point at the USB buffer */
			temp.head 	= APP_Rx_ptr_in;	/* 	Set the head of the temporary circular buffer to the
												same location as where the USB pointer is now */
			temp.tail 	= APP_Rx_ptr_in;	/* 	Since we don't know where the tail of the USB buffer
												is, assume it is empty */
			temp.size 	= APP_RX_DATA_SIZE;	/* 	The size of the buffer is given from the CDC core */

			/* Call the generator from the lookup table */
			status = generator_lookup[command](&temp);

			/* Increment the USB buffer pointer. The circular buffer code 
			 * calculates the correct pointer location for us. */
			APP_Rx_ptr_in = temp.head;
		}
		xSemaphoreGive(xUSBQueue.USB_Write_Lock);

		/* Release the USB and return the status */
	}
	else
		status = ERROR;
	
	return status;
}

/**
 * @brief 			Generates a message with no data part.
 * @details
 * 
 * @param command 	Command to generate message for.
 * @param Cbuff 	Pointer to the circular buffer to put the data in.
 * 
 * @return			Return ERROR if the message didn't fit or SUCCESS if it did fit.
 */
ErrorStatus GenerateHeaderOnlyCommand(KFly_Command_Type command, Circular_Buffer_Type *Cbuff)
{
	int32_t count = 0;
	uint8_t crc8;

	/* Write the stating SYNC (without doubling it) */
	CircularBuffer_WriteSYNCNoIncrement(		Cbuff, &count, &crc8, NULL); 

	/* Add all the data to the message */
	CircularBuffer_WriteNoIncrement(command,	Cbuff, &count, &crc8, NULL); 
	CircularBuffer_WriteNoIncrement(0, 			Cbuff, &count, &crc8, NULL); 
	CircularBuffer_WriteNoIncrement(crc8, 		Cbuff, &count, NULL,  NULL);

	/* Check if the message fit inside the buffer */
	return CircularBuffer_Increment(count, 		Cbuff);	
}

/**
 * @brief 				Generates a message with data and CRC16 part. 
 * @details	
 * 
 * @param command 		Command to generate message for.
 * @param data 			Pointer to where the data is located.
 * @param data_count 	Number of data bytes.
 * @param Cbuff 		Pointer to the circular buffer to put the data in.
 * 
 * @return				Return ERROR if the message didn't fit or SUCCESS if it did fit.
 */
ErrorStatus GenerateGenericCommand(KFly_Command_Type command, uint8_t *data, const uint32_t data_count, Circular_Buffer_Type *Cbuff)
{
	int32_t count = 0;
	uint8_t crc8;
	uint16_t crc16;

	/* Check if the "best case" won't fit in the buffer which is
	 * data_count + header + CRC16 = data_count + 6 bytes */
	if (CircularBuffer_SpaceLeft(Cbuff) < (data_count + 6))
		return ERROR;

	/* Add the header */
	/* Write the starting SYNC (without doubling it) */
	CircularBuffer_WriteSYNCNoIncrement(		Cbuff, &count, &crc8, &crc16); 

	/* Add all of the header to the message */
	CircularBuffer_WriteNoIncrement(command,	Cbuff, &count, &crc8, &crc16); 
	CircularBuffer_WriteNoIncrement(data_count,	Cbuff, &count, &crc8, &crc16); 
	CircularBuffer_WriteNoIncrement(crc8, 		Cbuff, &count, NULL,  &crc16);

	/* Add the data to the message */
	for (int i = 0; i < data_count; i++)
		CircularBuffer_WriteNoIncrement(data[i], Cbuff, &count, NULL, &crc16); 

	/* Add the CRC16 */
	CircularBuffer_WriteNoIncrement((uint8_t)(crc16 >> 8), 	Cbuff, &count, NULL, NULL);
	CircularBuffer_WriteNoIncrement((uint8_t)(crc16), 		Cbuff, &count, NULL, NULL);

	return CircularBuffer_Increment(count, 		Cbuff);	/* Check if the message fit inside the buffer */
}

/**
 * @brief 			Generates an ACK.
 * @details
 * 
 * @param Cbuff 	Pointer to the circular buffer to put the data in.
 * 
 * @return 			Return ERROR if the message didn't fit or SUCCESS if it did fit.
 */
ErrorStatus GenerateACK(Circular_Buffer_Type *Cbuff)
{
	return GenerateHeaderOnlyCommand(Cmd_ACK, Cbuff);	/* Return status */
}

/**
 * @brief 			Generates a Ping.
 * @details
 * 
 * @param Cbuff 	Pointer to the circular buffer to put the data in.
 * 
 * @return 			Return ERROR if the message didn't fit or SUCCESS if it did fit.
 */
ErrorStatus GeneratePing(Circular_Buffer_Type *Cbuff)
{
	return GenerateHeaderOnlyCommand(Cmd_Ping, Cbuff);	/* Return status */
}
