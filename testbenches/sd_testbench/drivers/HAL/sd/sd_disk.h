/***************************************************************************//**
  @file     sd_disk.h
  @brief    Low level SD disk I/O interface for FatFs
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

#ifndef HAL_SD_SD_DISK_H_
#define HAL_SD_SD_DISK_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "lib/fatfs/ff.h"
#include "lib/fatfs/diskio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLE PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

/*
 * @brief Performs a disk initialization on a sd.
 */
DSTATUS sdDiskInitialize(void);

/*
 * @brief Returns the sd physical drive.
 */
DSTATUS sdDiskStatus(void);

/*
 * @brief Performs a sector read on a sd.
 * @param buffer	Pointer to the memory buffer
 * @param sector	Number of the sector
 * @param count		Amount of sectors to be read
 */
DRESULT sdDiskRead(BYTE* buffer, DWORD sector, UINT count);

/*
 * @brief Performs a sector write on a sd.
 * @param buffer	Pointer to the memory buffer
 * @param sector	Number of the sector
 * @param count		Amount of sectors to be written
 */
DRESULT sdDiskWrite(const BYTE* buffer, DWORD sector, UINT count);

/*
 * @brief Performs a control communication with the sd.
 * @param command	Control command
 * @param buffer	Pointer to the memory buffer
 */
DRESULT sdDiskIoctl(BYTE command, void* buffer);

/*******************************************************************************
 ******************************************************************************/

#endif /* HAL_SD_SD_DISK_H_ */
