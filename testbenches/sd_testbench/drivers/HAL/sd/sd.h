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
	SD_FF_HARD_DISK,		// Hard disk-like file system with partition table
	SD_FF_DOS_FAT,			// DOS FAT (floppy-like) with boot sector only (no partition table)
	SD_FF_UNIVERSAL,		// Universal File Format
	SD_FF_OTHER_UNKNOWN,	// Others or Unknown file format
	SD_FF_RESERVED,			// Reserved file format

	SD_FF_COUNT
} sd_file_format_t;

typedef void (sd_callback_t)	(void);

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
 * @brief	Returns whether a card is inserted in the SD socket or not.
 */
bool sdIsCardInserted(void);

/*
 * @brief	Returns the sd card maximum storage size, measured in bytes.
 * 			You must have initialized the SD card previously.
 */
uint64_t sdGetSize(void);

/*
 * @brief	Returns the SD file format.
 * 			You must have initialized the SD card previously.
 */
sd_file_format_t sdGetFileFormat(void);

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
