#ifndef __EXT_FLASH_H
#define __EXT_FLASH_H

/* Standard includes */
#include "stm32f4xx.h"

/* System includes */
#include "spi.h"
#include "led.h"

/* Scheduler includes. */

/* KFly includes */

/* Driver Includes */

/* Includes */

/* Defines */
#define FLASH_SYNC_WORD			0xdeadbeef	/* Sync word to detect errors in the flash structure */
#define FLASH_SYNC_WORD_REV		0xefbeadde

#define FLASH_CMD_WRSR          0x01  		/* Write Status Register instruction	*/
#define FLASH_CMD_PAGE_PROGRAM  0x02  		/* Write to Memory instruction 			*/
#define FLASH_CMD_READ          0x03  		/* Read from Memory instruction 		*/
#define FLASH_CMD_RDSR          0x05  		/* Read Status Register instruction  	*/
#define FLASH_CMD_WREN          0x06  		/* Write enable instruction 			*/
#define FLASH_CMD_PAGE_WRITE	0x0A		/* Writes a page with automatic erase 	*/

#define FLASH_CMD_BE            0xC7  		/* Bulk Erase instruction 				*/
#define FLASH_CMD_SE            0xD8  		/* Sector Erase instruction 			*/
#define FLASH_CMD_PE			0xDB		/* Page erase 							*/
#define FLASH_CMD_RDID          0x9F  		/* Read identification 					*/

#define FLASH_WIP_FLAG          0x01  		/* Write In Progress (WIP) flag 		*/

#define FLASH_M25P128_ID        0x202018
#define FLASH_M25P64_ID         0x202017
#define FLASH_M25P40_ID         0x202013
#define FLASH_M25PE40_ID        0x208013

#define M25PE40_NUM_SECTORS		7
#define FLASH_NUM_SECTORS		M25PE40_NUM_SECTORS

#define FLASH_PAGE_SIZE			0x00000100
#define FLASH_SECTOR_SIZE		0x00010000
#define FLASH_SECTOR_0			0x00000000
#define FLASH_SECTOR_1			0x00010000
#define FLASH_SECTOR_2			0x00020000
#define FLASH_SECTOR_3			0x00030000
#define FLASH_SECTOR_4			0x00040000
#define FLASH_SECTOR_5			0x00050000
#define FLASH_SECTOR_6			0x00060000
#define FLASH_SECTOR_7			0x00070000

#define EXTERNAL_FLASH_SPI		SPI1

#define FLASH_END 				0

/* Typedefs */
typedef struct {
	uint8_t *ptr;		/* Holds the pointer of where the data should be saved to/read from */
	int32_t count;		/* Holds the number of bytes to write/read */
} Flash_Save_Template_Type;


/* Macros */
#define FLASH_CS_LOW()       	SPI1_FLASH_CS_LOW()
#define FLASH_CS_HIGH()      	SPI1_FLASH_CS_HIGH()

/* Global functions */
void ExternalFlashInit(void);
void ExternalFlash_EraseBulk(void);
void ExternalFlash_EraseSector(uint32_t sector);
uint32_t ExternalFlash_ReadID(void);
ErrorStatus ExternalFlash_CheckIntegrity(const Flash_Save_Template_Type *template, uint32_t sector);
ErrorStatus ExternalFlash_SaveSettings(const Flash_Save_Template_Type *template, uint32_t sector);
ErrorStatus ExternalFlash_LoadSettings(const Flash_Save_Template_Type *template, uint32_t sector);
void ExternalFlash_WritePage(uint8_t *buffer, uint32_t address, uint16_t count);
void ExternalFlash_ReadBuffer(uint8_t *buffer, uint32_t address, uint16_t count);
void ExternalFlash_WriteEnable(void);
void ExternalFlash_WaitForWriteEnd(void);
const Flash_Save_Template_Type *ptrGetFlashSaveStructure(void);

#endif
