/*******************************************************************************
  @file     sd.h
  @brief    SD card driver
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef HAL_SD_SD_H_
#define HAL_SD_SD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum {
	SD_STATE_NOT_CONNECTED,	// No SD Card connected to the socket
	SD_STATE_CONNECTED,		// SD Card connected to the socket, but not initialized
	SD_STATE_INITIALIZED,	// SD Card has been initialized correctly
	SD_STATE_ERROR,			// An error occurred while using the SD Card

	SD_STATE_COUNT
} sd_state_t;

typedef enum {
	SD_FF_HARD_DISK,		// Hard disk-like file system with partition table
	SD_FF_DOS_FAT,			// DOS FAT (floppy-like) with boot sector only (no partition table)
	SD_FF_UNIVERSAL,		// Universal File Format
	SD_FF_OTHER_UNKNOWN,	// Others or Unknown file format
	SD_FF_RESERVED,			// Reserved file format

	SD_FF_COUNT
} sd_file_format_t;

typedef void (*sd_callback_t)	(void);

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/***************
 * GENERAL API *
 **************/

/*
 * @brief Initializes the driver.
 */
void sdInit(void);

/*
 * @brief 	Initializes the SD card, following the steps described by the SD physical
 * 		  	layer specification.
 * @returns Whether it could initialize the card or not.
 */
bool sdCardInit(void);

/*
 * @brief	Returns the current state of the sd card driver.
 */
sd_state_t sdGetState(void);

/*
 * @brief	Read a memory region of the sd memory card, starting in the given block address
 * 			and reading the given amount of blocks. The block size is fixed to 512 bytes.
 * @param	readBuffer		Pointer to the read buffer
 * @param	blockNumber		Number of the block to start reading
 * @param	blockCount		Amount of blocks to be read
 */
bool sdRead(uint32_t* readBuffer, uint32_t blockNumber, uint32_t blockCount);

/*
 * @brief	Write a memory region of the sd memory card, starting in the given block address
 * 			and writing the given amount of blocks. The block size is fixed to 512 bytes.
 * @param	writeBuffer		Pointer to the write buffer
 * @param	blockNumber		Number of the block to start reading
 * @param	blockCount		Amount of blocks to be written
 */
bool sdWrite(uint32_t* writeBuffer, uint32_t blockNumber, uint32_t blockCount);

/*
 * @brief	Erase a memory region of the sd memory card, starting in the given block address
 * 			and clearing data in all the given amount of blocks.
 * @param	blockNumber		Number of the block to start reading
 * @param	blockCount		Amount of blocks to be erased
 */
bool sdErase(uint32_t blockNumber, uint32_t blockCount);

/*
 * @brief	Returns whether a card is inserted in the SD socket or not.
 */
bool sdIsCardInserted(void);

/*
 * @brief	Returns the SD file format.
 */
sd_file_format_t sdGetFileFormat(void);

/*
 * @brief	Returns the sd card maximum storage size, measured in bytes.
 */
uint64_t sdGetSize(void);

/*
 * @brief	Returns the erase size supported by the SD card.
 */
uint32_t sdGetEraseSize(void);

/*
 * @brief	Returns the block size supported by the SD card.
 */
uint32_t sdGetBlockSize(void);

/*
 * @brief	Returns the block count supported by the SD card.
 */
uint32_t sdGetBlockCount(void);

/********************
 * EVENT-DRIVEN API *
 *******************/

/*
 * @brief Registers a callback to be called whenever a card is inserted.
 * @param callback	Callback to be registered
 */
void sdOnCardInserted(sd_callback_t callback);

/*
 * @brief Registers a callback to be called whenever a card is removed.
 * @param callback	Callback to be registered
 */
void sdOnCardRemoved(sd_callback_t callback);

/*******************************************************************************
 ******************************************************************************/


#endif /* HAL_SD_SD_H_ */
