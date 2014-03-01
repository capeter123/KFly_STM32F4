/**
 *
 * OS layer for Serial Communication.
 * Handles package coding/decoding.
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
 * A command is build up by 8 bits (1 byte) and the 8-th bit (MSB) is the ACK Request bit.
 * So all command must not use the ACK bit unless they need an ACK.
 * Command 0bAxxx xxxx <- A is ACK-bit.
 * Also the value 0xa6/166d/0b10100110 is reserved as the SYNC-byte.
 *
 * This gives 126 commands for the user.
 *
 */

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
		pHolder->next_state = vRxCmd;

		pHolder->buffer_count = 0;
		pHolder->crc8 = CRC8_step(SYNC_BYTE, 0x00);
		pHolder->crc16 = CRC16_step(SYNC_BYTE, 0xffff);
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
		pHolder->crc8 = CRC8_step(SYNC_BYTE, 0x00);
		pHolder->crc16 = CRC16_step(SYNC_BYTE, 0xffff);

		vRxCmd(data, pHolder);
	}
}

/* *
 * Command parser. Checks if a valid command was received.
 * */
void vRxCmd(uint8_t data, Parser_Holder_Type *pHolder)
{
	/* 0 is not an allowed command (Cmd_None) */
	if ((data & ~ACK_BIT) > Cmd_None)
	{
		pHolder->next_state = vRxSize;

		/* Update the CRCs */
		pHolder->crc8 = CRC8_step(data, pHolder->crc8);
		pHolder->crc16 = CRC16_step(data, pHolder->crc16);

		/* Get the correct parser from the parser lookup table */
		pHolder->parser = GetParser(data & ~ACK_BIT);

		/* If ACK is requested */
		if (data & ACK_BIT)
			pHolder->AckRequested = TRUE;
		else
			pHolder->AckRequested = FALSE;
	}
	else
	{
		pHolder->next_state = vWaitingForSYNC;
		pHolder->rx_error++;
	}
}

/* *
 * Checks the length of a message.
 * */
void vRxSize(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->next_state = vRxCRC8;

	pHolder->crc8 = CRC8_step(data, pHolder->crc8);
	pHolder->crc16 = CRC16_step(data, pHolder->crc16);
	pHolder->data_length = data; /* Set the length of the message to that of the header. */
}

/* *
 * Checks the Header CRC8.
 * */
void vRxCRC8(uint8_t data, Parser_Holder_Type *pHolder)
{
	if (pHolder->crc8 == data)
	{
		/* CRC OK! */
		if (pHolder->data_length == 0)
		{	/* If no data, parse now! */
			pHolder->next_state = vWaitingForSYNC;

			if (pHolder->parser != NULL)
				pHolder->parser(pHolder);
		}
		else
		{
			pHolder->next_state = vRxData;

			pHolder->crc16 = CRC16_step(data, pHolder->crc16);
		}
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
	if (pHolder->buffer_count < (pHolder->data_length - 1))
		pHolder->next_state = vRxData;
	else
		pHolder->next_state = vRxCRC16_1;

	pHolder->crc16 = CRC16_step(data, pHolder->crc16);
	pHolder->buffer[pHolder->buffer_count++] = data;
}

/* *
 * Checks the first CRC16 byte.
 * */
void vRxCRC16_1(uint8_t data, Parser_Holder_Type *pHolder)
{
	if (data == (uint8_t)(pHolder->crc16 >> 8))
	{
		/* CRC OK! */
		pHolder->next_state = vRxCRC16_2;
	}
	else
	{
		pHolder->next_state = vWaitingForSYNC;
		pHolder->rx_error++;
	}
}

/* *
 * Checks the second CRC16 byte. If OK: run parser.
 * */
void vRxCRC16_2(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->next_state = vWaitingForSYNC;


	if (data == (uint8_t)(pHolder->crc16))
	{
		/* If there is a parser for the message, execute it */
		if (pHolder->parser != NULL)
			pHolder->parser(pHolder);

		/* If an ACK was requested, send it after the parser finished */
		if (pHolder->AckRequested == TRUE)
		{
			if (pHolder->Port == PORT_USB)
				GenerateUSBMessage(Cmd_ACK);

			else if (pHolder->Port == PORT_AUX1)
				GenerateAUXMessage(Cmd_ACK, NULL);

			else if (pHolder->Port == PORT_AUX2)
				GenerateAUXMessage(Cmd_ACK, NULL);

			else if (pHolder->Port == PORT_AUX3)
				GenerateAUXMessage(Cmd_ACK, NULL);
			
			else if (pHolder->Port == PORT_AUX4)
				GenerateAUXMessage(Cmd_ACK, NULL);
		}
	}
	else /* CRC error! Discard data. */
	{
		pHolder->next_state = vWaitingForSYNC;
		pHolder->rx_error++;
	}
}
