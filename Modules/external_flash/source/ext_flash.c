/* *
 *
 * Hardware Abstraction Layer for External Flash
 *
 * */

#include "ext_flash.h"

/* Global variable defines */

/* Private variable defines */

/* Area to hold the design of the external flash memory */
Flash_Save_Template_Type Flash_Save_Structure[] = {
	{	
		.ptr = (uint8_t *)1,
		.count = 32
	},
	{
		.ptr = (uint8_t *)2,
		.count = 40
	},
	{	/* To indicate the end of the save structure */
		.ptr = FLASH_END,
		.count = FLASH_END
	}
};

/* Private function defines */
static uint32_t SaveStructure_NumberOfBytes(Flash_Save_Template_Type *template);
static uint32_t SaveStructure_WriteToMemory(Flash_Save_Template_Type *template, uint32_t sector);

/* Private external functions */


void ExternalFlashInit(void)
{
	if (ExternalFlash_ReadID() != FLASH_M25PE40_ID)
	{
		for (int j = 0; j < 10; j++)
		{
			for (volatile uint32_t i = 0; i < 0x1FFFFF; i++);

			LEDToggle(LED_RED);
		}
	}
}

void ExternalFlash_EraseBulk(void)
{
  	/* Enable the write access to the External Flash */
	ExternalFlash_WriteEnable();

  	/* Select the External Flash: Chip Select low */
  	FLASH_CS_LOW();

	/* Send Bulk Erase instruction  */
	SPI_SendBytePolling(FLASH_CMD_BE, EXTERNAL_FLASH_SPI);

	/* Deselect the External Flash: Chip Select high */
  	FLASH_CS_HIGH();

  	/* Wait the end of Flash writing */
  	ExternalFlash_WaitForWriteEnd();
}

void ExternalFlash_EraseSector(uint32_t sector)
{
	/* Enable the write access to the External Flash */
	ExternalFlash_WriteEnable();

	/* Select the External Flash: Chip Select low */
  	FLASH_CS_LOW();

	/* Send Bulk Erase instruction  */
  	SPI_SendBytePolling(FLASH_CMD_SE, EXTERNAL_FLASH_SPI);
	
	/* Send address high nibbles */
	SPI_SendBytePolling((sector & 0xFF0000) >> 16, EXTERNAL_FLASH_SPI);
	SPI_SendBytePolling((sector & 0xFF00) >> 8, EXTERNAL_FLASH_SPI);
	SPI_SendBytePolling(sector & 0xFF, EXTERNAL_FLASH_SPI);
	
	/* Deselect the External Flash: Chip Select high */
  	FLASH_CS_HIGH();

  	/* Wait the end of Flash writing */
  	ExternalFlash_WaitForWriteEnd();
}

/**
 * @brief 		Gets the ID of the External Flash.
 *
 * @return 		External Flash ID.
 */
uint32_t ExternalFlash_ReadID(void)
{
	uint32_t t1 = 0, t2 = 0, t3 = 0;

	/* Select the External Flash: Chip Select low */
	FLASH_CS_LOW();

	/* Send "Read ID" instruction */
	SPI_SendBytePolling(FLASH_CMD_RDID, EXTERNAL_FLASH_SPI);

	/* Read the three ID bytes from the External Flash */
	t1 = SPI_SendBytePolling(SPI_DUMMY_BYTE, EXTERNAL_FLASH_SPI);
	t2 = SPI_SendBytePolling(SPI_DUMMY_BYTE, EXTERNAL_FLASH_SPI);
	t3 = SPI_SendBytePolling(SPI_DUMMY_BYTE, EXTERNAL_FLASH_SPI);

	/* Deselect the External Flash: Chip Select high */
	FLASH_CS_HIGH();

	return ((t1 << 16) | (t2 << 8) | t3);
}


ErrorStatus ExternalFlash_CheckSettingsStructure(Flash_Save_Template_Type *template, uint32_t sector)
{
	return SUCCESS;
}

ErrorStatus ExternalFlash_SaveSettings(Flash_Save_Template_Type *template, uint32_t sector)
{
	/* Check so we write to one of the sectors */
	if (sector > FLASH_NUM_SECTORS)
		return ERROR;

	/* Erase the selected sector */
	ExternalFlash_EraseSector(sector * FLASH_SECTOR_SIZE);

	/* Start saving data to the external flash memory */
	SaveStructure_WriteToMemory(template, sector);

	return SUCCESS;
}


ErrorStatus ExternalFlash_LoadSettings(Flash_Save_Template_Type *template, uint32_t sector)
{
	uint32_t i, page_address;

	i = 0;

	/* Calculate the starting sector address */
	page_address = sector * FLASH_SECTOR_SIZE;

	/* Check so we read from one of the sectors */
	if (sector > FLASH_NUM_SECTORS)
		return ERROR;

	while (template[i].ptr != FLASH_END)
	{
		/* Read the setting from the flash */
		ExternalFlash_ReadBuffer(template[i].ptr, page_address, template[i].count);

		/* Increment Flash address and template address */
		page_address += template[i].count + 4; /* The 4 comes from the SYNC */
		i++;
	}

	return SUCCESS;
}

/**
 * @brief  			Writes data to a Flash page.
 * 
 * @param buffer 	Pointer to the buffer holding the data.
 * @param address 	Where in the Flash to save the data.
 * @param count 	Number of bytes to write (max 256 bytes).
 */
void ExternalFlash_WritePage(uint8_t *buffer, uint32_t address, uint16_t count)
{
	/* Enable the write access to the External Flash */
	ExternalFlash_WriteEnable();

  	/* Select the External Flash: Chip Select low */
  	FLASH_CS_LOW();

  	/* Send "Write to Memory" instruction */
  	SPI_SendBytePolling(FLASH_CMD_PAGE_PROGRAM, EXTERNAL_FLASH_SPI);

	/* Send address nibbles for address byte to write to */
	SPI_SendBytePolling((address & 0xFF0000) >> 16, EXTERNAL_FLASH_SPI);	
  	SPI_SendBytePolling((address & 0xFF00) >> 8, EXTERNAL_FLASH_SPI);
  	SPI_SendBytePolling(address & 0xFF, EXTERNAL_FLASH_SPI);

  	/* While there is data to be written on the External Flash */
  	while (count--)
    	SPI_SendBytePolling(*(buffer++), EXTERNAL_FLASH_SPI);

  	/* Deselect the External Flash: Chip Select high */
  	FLASH_CS_HIGH();

  	/* Wait the end of Flash writing */
  	ExternalFlash_WaitForWriteEnd();
}

/**
 * @brief 			Read a block of data from the External Flash.
 * 
 * @param buffer 	Pointer to the buffer saving the data.
 * @param address 	Where in the Flash to read the data.
 * @param count 	Number of bytes to read.
 */
void ExternalFlash_ReadBuffer(uint8_t *buffer, uint32_t address, uint16_t count)
{
	/* Select the External Flash: Chip Select low */
	FLASH_CS_LOW();

	/* Send "Read from Memory" instruction */
	SPI_SendBytePolling(FLASH_CMD_READ, EXTERNAL_FLASH_SPI);

 	/* Send address nibbles for address byte to read from */
	SPI_SendBytePolling((address & 0xFF0000) >> 16, EXTERNAL_FLASH_SPI);
	SPI_SendBytePolling((address& 0xFF00) >> 8, EXTERNAL_FLASH_SPI);
	SPI_SendBytePolling(address & 0xFF, EXTERNAL_FLASH_SPI);

	while (count--)
		*(buffer++) = SPI_SendBytePolling(SPI_DUMMY_BYTE, EXTERNAL_FLASH_SPI);

	/* Deselect the External Flash: Chip Select high */
	FLASH_CS_HIGH();
}

/**
 * @brief 	Enables the write access to the External Flash.
 */
void ExternalFlash_WriteEnable(void)
{
  	/* Select the External Flash: Chip Select low */
  	FLASH_CS_LOW();

  	/* Send "Write Enable" instruction */
  	SPI_SendBytePolling(FLASH_CMD_WREN, EXTERNAL_FLASH_SPI);

  	/* Deselect the External Flash: Chip Select high */
  	FLASH_CS_HIGH();
}

/**
 * @brief		Polls the status of the Write In Progress (WIP) flag in the External
 *         		Flash's status register until write operation has completed.
 */
void ExternalFlash_WaitForWriteEnd(void)
{
  	uint8_t flashstatus = 0;

  	/* Select the External Flash: Chip Select low */
  	FLASH_CS_LOW();

  	/* Send "Read Status Register" instruction */
  	SPI_SendBytePolling(FLASH_CMD_RDSR, EXTERNAL_FLASH_SPI);

  	/* Loop as long as the memory is busy with a write cycle */
  	while (SPI_SendBytePolling(SPI_DUMMY_BYTE, EXTERNAL_FLASH_SPI) & FLASH_WIP_FLAG);

  	/* Deselect the External Flash: Chip Select high */
  	FLASH_CS_HIGH();
}


Flash_Save_Template_Type *ptrGetFlashSaveStructure(void)
{
	return Flash_Save_Structure;
}


static uint32_t SaveStructure_NumberOfBytes(Flash_Save_Template_Type *template)
{
	uint32_t i = 0, num_bytes = 0;

	/* While there is an valid pointer add its byte count to the total */
	while (template[i].ptr != 0)
		num_bytes += template[i++].count;

	return num_bytes;
}

/**
 * @brief 			Processes a Flash_Save structure to determine the
 * 					correct way of writing it to memory.
 * 
 * @param template 	Pointer to the Flash_Save template.
 * @return 			Number of bytes to save.
 */
static uint32_t SaveStructure_WriteToMemory(Flash_Save_Template_Type *template, uint32_t sector)
{
	int32_t i, j, num_bytes_written_to_page, page_address;

	i = 0;
	num_bytes_written_to_page = 0;
	page_address = sector * FLASH_SECTOR_SIZE;

	/* Enable the write access to the External Flash */
	ExternalFlash_WriteEnable();

	/* Select the External Flash: Chip Select low */
	FLASH_CS_LOW();

	/* Send "Write to Memory" instruction */
	SPI_SendBytePolling(FLASH_CMD_PAGE_PROGRAM, EXTERNAL_FLASH_SPI);

	/* Send address nibbles for address byte to write to */
	SPI_SendBytePolling((page_address & 0xFF0000) >> 16, EXTERNAL_FLASH_SPI);
	SPI_SendBytePolling((page_address & 0xFF00) >> 8, EXTERNAL_FLASH_SPI);
	SPI_SendBytePolling(page_address & 0xFF, EXTERNAL_FLASH_SPI);

	while (template[i].ptr != FLASH_END)
	{
		for (j = -4; j < template[i].count; j++)
		{
			if (++num_bytes_written_to_page >= FLASH_PAGE_SIZE)
			{
				/* Wait for write complete and start a new write cycle */

				/* Deselect the External Flash: Chip Select high */
			  	FLASH_CS_HIGH();

			  	/* Wait the end of Flash writing */
			  	ExternalFlash_WaitForWriteEnd();

			  	/* Start new write cycle and increment the address */
			  	page_address += FLASH_PAGE_SIZE;

			  	/* Enable the write access to the External Flash */
				ExternalFlash_WriteEnable();

			  	/* Select the External Flash: Chip Select low */
			  	FLASH_CS_LOW();

			  	/* Send "Write to Memory" instruction */
			  	SPI_SendBytePolling(FLASH_CMD_PAGE_PROGRAM, EXTERNAL_FLASH_SPI);

				/* Send address nibbles for address byte to write to */
				SPI_SendBytePolling((page_address & 0xFF0000) >> 16, EXTERNAL_FLASH_SPI);
			  	SPI_SendBytePolling((page_address & 0xFF00) >> 8, EXTERNAL_FLASH_SPI);
			  	SPI_SendBytePolling(page_address & 0xFF, EXTERNAL_FLASH_SPI);

			  	/* Set to one for the byte to be written directly after */
				num_bytes_written_to_page = 1;
			}

			/* Send SYNC first, then the data */


			if (j < 0)
				SPI_SendBytePolling((FLASH_SYNC_WORD >> (-j - 1) * 8) & 0xff, EXTERNAL_FLASH_SPI);
			else
				SPI_SendBytePolling(template[i].ptr[j], EXTERNAL_FLASH_SPI);
		}

		i++;
	}

	/* Deselect the External Flash: Chip Select high */
	FLASH_CS_HIGH();

	/* Wait the end of Flash writing */
	ExternalFlash_WaitForWriteEnd();
}

