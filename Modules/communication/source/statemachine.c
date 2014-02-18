/* *
 *
 * OS layer for Serial Communication.
 * Handles package coding/decoding.
 *
 * ----------!!!----------
 * A warning to those who wants to read the code:
 * This is an orgie of function pointers and buffers.
 * ----------!!!----------
 *
 * Serial Communication Protocol
 * -----------------------------
 * This was designed so that binary data could be sent while not
 * needing to "code" it as ASCII HEX. A simple SYNC byte is used
 * to denote the start of a transfer and after that a header
 * containing the command and size of the message. If the size is
 * greater than 0 a data package comes after the first CRC. If
 * the data contains a byte that has the same value as the sync
 * byte it will be replaced by two sync bytes "x" -> "xx" to denote
 * a byte of the value sync and not a data package sync.
 *
 *
 * Protocol:
 * 		SYNC | HEADER | CRC8 | DATA | CRC16
 * 		DATA and CRC16 is optional.
 *
 * HEADER:
 * 		CMD 	| DATA SIZE
 * 		1 byte 	| 1 byte
 *
 * DATA:
 * 		BINARY DATA
 * 		1 - 255 bytes
 *
 * SYNC: 1 byte
 * 		Sent once = SYNC
 * 		Sent twice = data byte with the value of SYNC ("escaping")
 *
 * CRC8: 1 byte
 * 		CRC-8 of SYNC and HEADER
 *
 * CRC16: 2 bytes
 * 		CCITT (16-bit) of whole message including SYNC and CRC8
 * 		For more information about the CRCs look in crc.c/crc.h
 *
 *
 * -------------------- OBSERVE! --------------------
 * A command is build up by 8 bits (1 byte) and the 8-th bit is the ACK Request bit.
 * So all command must not use the ACK bit unless they need an ACK.
 * Command 0bAxxx xxxx <- A is ACK-bit.
 * Also the value 0xa6/166d/0b10100110 is reserved as the SYNC-byte.
 *
 * This gives 126 commands for the user.
 *
 * */

#include "statemachine.h"


/* *
 *
 * ---------------- State machine functions ----------------
 * State machine's different states (functions) starts below.
 * NO (!) other functions are allowed there!
 *
 * */

/* *
 * The entry point of serial data.
 * */
void vStatemachineDataEntry(uint8_t data, Parser_Holder_Type *pHolder)
{
	if (data == SYNC_BYTE)
	{
		if ((pHolder->next_state != vWaitingForSYNC) && \
		    (pHolder->next_state != vWaitingForSYNCorCMD) && \
		    (pHolder->next_state != vRxCmd))
		{
			pHolder->current_state = pHolder->next_state;
			pHolder->next_state = vWaitingForSYNCorCMD;
		}
		else
			pHolder->next_state(data, pHolder);
	}
	else
		pHolder->next_state(data, pHolder);
}


/* *
 * Waiting for SYNC function. Will run this until a valid SYNC has occurred.
 * */
void vWaitingForSYNC(uint8_t data, Parser_Holder_Type *pHolder)
{
	if (data == SYNC_BYTE)
	{
		pHolder->buffer_count = 0;
		pHolder->buffer[pHolder->buffer_count++] = SYNC_BYTE;
		pHolder->next_state = vRxCmd;
	}
}

/* *
 * A SYNC appeared in data stream.
 * When a SYNC is detected in the data stream the program will
 * look and see if it is an SYNC byte or if it's a new SYNC.
 *
 * */
void vWaitingForSYNCorCMD(uint8_t data, Parser_Holder_Type *pHolder)
{
	if (data == SYNC_BYTE) /* Byte with value of SYNC received,	send it to the function waiting for a byte */
		pHolder->current_state(data, pHolder);
	else /* If not SYNC, reset transfer and check if byte is command */
	{
		pHolder->buffer_count = 0;
		pHolder->buffer[pHolder->buffer_count++] = SYNC_BYTE;
		vRxCmd(data, pHolder);
	}
}

/* *
 * Command parser. Checks if a valid command was received.
 * */
void vRxCmd(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->next_state = vRxSize;
	pHolder->buffer[pHolder->buffer_count++] = data;

	switch (data & ~ACK_BIT)
	{
		case SYNC_BYTE: /* SYNC is not allowed as command! */
			pHolder->next_state = vRxCmd; /* If sync comes, continue running vRxCmd */
			pHolder->rx_error++;
			pHolder->buffer_count--; /* So buffer_count doesn't get the wrong value on error */
			break;

		case Cmd_Ping:
			pHolder->parser = vPing;
			pHolder->data_length = Length_Ping;
			break;

		case Cmd_DebugMessage:
			pHolder->parser = NULL;
			pHolder->data_length = Length_DebugMessage;
			break;

		case Cmd_GetRunningMode:
			pHolder->parser = vGetRunningMode;
			pHolder->data_length = Length_GetRunningMode;
			break;


		/* ----- Start Bootloader specific commands ----- */

		case Cmd_PrepareWriteFirmware:
			pHolder->parser = NULL;
			pHolder->data_length = Length_PrepareWriteFirmware;
			break;

		case Cmd_WriteFirmwarePackage:
			pHolder->parser = NULL;
			pHolder->data_length = Length_WriteFirmwarePackage;
			break;

		case Cmd_WriteLastFirmwarePackage:
			pHolder->parser = NULL;
			pHolder->data_length = Length_WriteLastFirmwarePackage;
			break;

		case Cmd_ReadFirmwarePackage:
			pHolder->parser = NULL;
			pHolder->data_length = Length_ReadFirmwarePackage;
			break;

		case Cmd_ReadLastFirmwarePackage:
			pHolder->parser = NULL;
			pHolder->data_length = Length_ReadLastFirmwarePackage;
			break;

		case Cmd_NextPackage:
			pHolder->parser = NULL;
			pHolder->data_length = Length_NextPackage;
			break;

		case Cmd_ExitBootloader:
			pHolder->parser = NULL;
			pHolder->data_length = Length_ExitBootloader;
			break;

		/* ----- End Bootloader specific commands ----- */


		case Cmd_GetBootloaderVersion:
			pHolder->parser = vGetBootloaderVersion;
			pHolder->data_length = Length_GetBootloaderVersion;
			break;

		case Cmd_GetFirmwareVersion:
			pHolder->parser = vGetFirmwareVersion;
			pHolder->data_length = Length_GetFirmwareVersion;
			break;


		/* ----- Start Firmware specific commands ----- */

		case Cmd_SaveToFlash:
			pHolder->parser = NULL;
			pHolder->data_length = Length_SaveToFlash;
			break;

		case Cmd_GetRateControllerData:
			pHolder->parser = vGetRateControllerData;
			pHolder->data_length = Length_GetControllerData;
			break;

		case Cmd_SetRateControllerData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_SetControllerData;
			break;

		case Cmd_GetAttitudeControllerData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_GetControllerData;
			break;

		case Cmd_SetAttitudeControllerData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_SetControllerData;
			break;

		case Cmd_GetVelocityControllerData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_GetControllerData;
			break;

		case Cmd_SetVelocityControllerData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_SetControllerData;
			break;

		case Cmd_GetPositionControllerData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_GetControllerData;
			break;

		case Cmd_SetPositionControllerData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_SetControllerData;
			break;

		case Cmd_GetChannelMix:
			pHolder->parser = NULL;
			pHolder->data_length = Length_GetChannelMix;
			break;

		case Cmd_SetChannelMix:
			pHolder->parser = NULL;
			pHolder->data_length = Length_SetChannelMix;
			break;

		case Cmd_GetRCCalibration:
			pHolder->parser = NULL;
			pHolder->data_length = Length_GetRCCalibration;
			break;

		case Cmd_SetRCCalibration:
			pHolder->parser = NULL;
			pHolder->data_length = Length_SetRCCalibration;
			break;

		case Cmd_GetRCValues:
			pHolder->parser = NULL;
			pHolder->data_length = Length_GetRCValues;
			break;

		case Cmd_GetSensorData:
			pHolder->parser = NULL;
			pHolder->data_length = Length_GetSensorData;
			break;

		/* ----- End Firmware specific commands ----- */

		/* *
		 * Add new commands here!
		 * */

		default:
			pHolder->next_state = vWaitingForSYNC;
			pHolder->rx_error++;
			break;
	}
}

/* *
 * Checks the length of a message. If it equals the correct length
 * or if the message has unspecified length (0xFF) it will wait for
 * Header CRC8.
 * */
void vRxSize(uint8_t data, Parser_Holder_Type *pHolder)
{
	if ((pHolder->data_length == data) || (pHolder->data_length == 0xFF))
	{	/* If correct length or unspecified length, go to CRC8 */
		pHolder->next_state = vRxCRC8;
		pHolder->buffer[pHolder->buffer_count++] = data;
		pHolder->data_length = data; /* Set the length of the message to that of the header. */
	}
	else /* Data length error! */
	{
		pHolder->next_state = vWaitingForSYNC;
		pHolder->rx_error++;
	}
}

/* *
 * Checks the Header CRC8.
 * */
void vRxCRC8(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->buffer[pHolder->buffer_count++] = data;

	if (CRC8(pHolder->buffer, 3) == data)
	{
		/* CRC OK! */

		if (pHolder->data_length == 0)
		{	/* If no data, parse now! */
			pHolder->next_state = vWaitingForSYNC;

			/* If ACK is requested */
			if (pHolder->buffer[1] & ACK_BIT)
				pHolder->AckRequested = TRUE;
			else
				pHolder->AckRequested = FALSE;

			if (pHolder->parser != NULL)
				pHolder->parser(pHolder);
		}
		else
			pHolder->next_state = vRxData;
	}
	else /* CRC error! */
	{
		pHolder->next_state = vWaitingForSYNC;
		pHolder->rx_error++;
	}
}

/* *
 * Data receiver function. Keeps track of the number of received bytes
 * and decides when to check for CRC.
 * */
void vRxData(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->buffer[pHolder->buffer_count++] = data;

	if (pHolder->buffer_count < (pHolder->data_length + 4))
		pHolder->next_state = vRxData;
	else
		pHolder->next_state = vRxCRC16;
}
/* *
 * Checks the whole message for errors and calls the parser if the
 * message was without errors.
 * */
void vRxCRC16(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->buffer[pHolder->buffer_count++] = data;

	if (pHolder->buffer_count < (pHolder->data_length + 6))
		pHolder->next_state = vRxCRC16;

	else
	{
		pHolder->next_state = vWaitingForSYNC;
		/* Cast the 2 bytes containing the CRC-CCITT to a 16-bit variable */
		uint16_t crc = ((uint16_t)(pHolder->buffer[pHolder->buffer_count - 2]) << 8) | (uint16_t)pHolder->buffer[pHolder->buffer_count - 1];

		if (CRC16(pHolder->buffer, (pHolder->buffer_count - 2)) == crc)
		{
			/* If ACK is requested */
			if (pHolder->buffer[1] & ACK_BIT)
				pHolder->AckRequested = TRUE;
			else
				pHolder->AckRequested = FALSE;

			/* If there is a parser for the message, execute it */
			if (pHolder->parser != NULL)
				pHolder->parser(pHolder);
		}
		else /* CRC error! Discard data. */
		{
			pHolder->next_state = vWaitingForSYNC;
			pHolder->rx_error++;
		}
	}
}
