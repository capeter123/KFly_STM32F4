/* *
 *
 * OS layer for Serial Communication.
 * Handles package coding/decoding.
 *
 * A warning to those who wants to read the code:
 * This is an orgie of function pointers and buffers.
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
 * 		Sent twice = databyte with the value of SYNC
 *
 * CRC8: 1 byte
 * 		CRC-8 of SYNC and HEADER
 *
 * CRC16: 2 bytes
 * 		CCITT (16-bit) of whole message including SYNC and CRC8
 * 		For more information about the CRCs look in crc.c/crc.h
 *
 * */

#include "serialmanager.h"

/* *
 *
 * Initializes all communication.
 *
 * */
void vInitSerialManager(void)
{
	vUSBQueueInit();


	xTaskCreate(vTaskUSBSerialManager,
				"SerialManager(USB)",
				512,
				0,
				tskSerialManagerPRIORITY,
			    0);
}


/* *
 *
 * The Serial Manager task will handle incomming
 * data and direct it for decode and processing.
 * The state machine can be seen in serial_state.png
 *
 * */
void vTaskUSBSerialManager(void *pvParameters)
{
	char in_data;
	Parser_Holder_Type data_holder;

	data_holder.Port = PORT_USB;
	data_holder.current_state = NULL;
	data_holder.next_state = vWaitingForSYNC;
	data_holder.parser = NULL;
	data_holder.rx_error = 0;

	while(1)
	{
		xQueueReceive(xUSBQueue.xUSBQueueHandle, &in_data, portMAX_DELAY);

		if (in_data == SYNC_BYTE)
		{
			if ((data_holder.next_state != vWaitingForSYNC) && (data_holder.next_state != vWaitingForSYNCorCMD))
				data_holder.next_state = vWaitingForSYNCorCMD;
			else
				data_holder.next_state(in_data, &data_holder);
		}
		else
			data_holder.next_state(in_data, &data_holder);
	}
}

/* *
 * Waiting for SYNC function. Will run this until a valid SYNC has occured.
 * */
void vWaitingForSYNC(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->current_state = vWaitingForSYNC;

	if (data == SYNC_BYTE)
	{
		pHolder->buffer_count = 0;
		pHolder->buffer[pHolder->buffer_count++] = SYNC_BYTE;
		pHolder->next_state = vRxCmd;
	}
}

/* *
 * A SYNC appeared in data stream.
 *
 * TODO: Add explanation
 *
 * */
void vWaitingForSYNCorCMD(uint8_t data, Parser_Holder_Type *pHolder)
{
	if (data == SYNC_BYTE) /* Byte with value of SYNC received,
							send it to the function waiting for a byte */
		pHolder->current_state(data, pHolder);
	else /* In not SYNC check if byte is command */
		vRxCmd(data, pHolder);
}

/* *
 * Command parser. Checks if a valid command was received.
 * */
void vRxCmd(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->current_state = vRxCmd;
	pHolder->next_state = vRxSize;
	pHolder->buffer[pHolder->buffer_count++] = data;

	switch (data)
	{
		case SYNC_BYTE: /* SYNC is not allowed as command! */
				pHolder->next_state = vRxCmd; /* If sync comes, continue running vRxCmd */
				pHolder->rx_error++;
				break;

		case Ping:
			pHolder->parser = NULL;
			pHolder->data_length = PingLength;
			break;

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
	pHolder->current_state = vRxSize;

	if ((pHolder->data_length == data) || (pHolder->data_length == 0xFF))
	{	/* If correct length or unspecified length, go to CRC8 */
		pHolder->next_state = vRxCRC8;
		pHolder->buffer[pHolder->buffer_count++] = data;
		pHolder->data_length = data;
	}
	else
		pHolder->next_state = vWaitingForSYNC;
}

/* *
 * Checks the Header CRC8.
 * */
void vRxCRC8(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->current_state = vRxCRC8;
	pHolder->buffer[pHolder->buffer_count++] = data;

	if (CRC8(pHolder->buffer, 3) == data)
	{
		/* CRC OK! */

		if (pHolder->data_length == 0)
		{	/* If no data, parse now! */
			pHolder->next_state = vWaitingForSYNC;
			pHolder->parser(pHolder);
		}
		else
			pHolder->next_state = vRxData;
	}
	else
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
	pHolder->current_state = vRxData;
	pHolder->buffer[pHolder->buffer_count++] = data;

	if (pHolder->buffer_count < (pHolder->data_length + 4))
	{
		pHolder->next_state = vRxData;
	}
	else
		pHolder->next_state = vRxCRC16;
}
/* *
 * Checks the whole message for errors and calles the parser if the
 * message was without errors.
 * */
void vRxCRC16(uint8_t data, Parser_Holder_Type *pHolder)
{
	pHolder->current_state = vRxCRC16;
	pHolder->buffer[pHolder->buffer_count++] = data;

	if (pHolder->buffer_count < (pHolder->data_length + 6))
	{
		pHolder->next_state = vRxCRC16;
	}
	else
	{
		pHolder->next_state = vWaitingForSYNC;
		/* Cast the 2 bytes containing the CRC-CCITT to a 16-bit variable */
		uint16_t crc = ((uint16_t)(pHolder->buffer[pHolder->buffer_count - 2]) << 8) | (uint16_t)pHolder->buffer[pHolder->buffer_count - 1];

		if (CRC16(pHolder->buffer, (pHolder->buffer_count - 2)) == crc)
			pHolder->parser(pHolder);
		else
			pHolder->rx_error++;
	}
}
