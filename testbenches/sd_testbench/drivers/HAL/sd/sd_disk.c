/***************************************************************************//**
  @file     sd_disk.c
  @brief    Low level SD disk I/O interface for FatFs
  @author   G. Davidov, F. Farall, J. Gaytán, L. Kammann, N. Trozzo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include "sd.h"
#include "sd_disk.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

/*******************************************************************************
 * VARIABLES WITH GLOBAL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/


/*******************************************************************************
 * ROM CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 * STATIC VARIABLES AND CONST VARIABLES WITH FILE LEVEL SCOPE
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

DSTATUS sdDiskInitialize(void)
{
	DSTATUS status = STA_NOINIT;

	sdInit();
	switch (sdGetState())
	{
		case SD_STATE_INITIALIZED:
			status = 0;
			break;
		case SD_STATE_CONNECTED:
		case SD_STATE_ERROR:
			status = sdCardInit() ? 0 : STA_NOINIT;
			break;
		default:
			status = STA_NOINIT;
			break;
	}

	return status;
}

DSTATUS sdDiskStatus(void)
{
	return (sdGetState() == SD_STATE_INITIALIZED) ? 0 : STA_NOINIT;
}

DRESULT sdDiskRead(BYTE* buffer, DWORD sector, UINT count)
{
	return sdRead((uint32_t*)buffer, sector, count) ? RES_OK : RES_ERROR;
}

DRESULT sdDiskWrite(const BYTE* buffer, DWORD sector, UINT count)
{
	return sdWrite((uint32_t*)buffer, sector, count) ? RES_OK : RES_ERROR;
}

DRESULT sdDiskIoctl(BYTE command, void* buffer)
{
	DRESULT result = RES_PARERR;

	switch (command)
	{
		case CTRL_SYNC:
			result = RES_OK;
			break;
		case GET_SECTOR_COUNT:
			if (buffer)
			{
				*(uint32_t*)buffer = sdGetBlockCount();
				result = RES_OK;
			}
			else
			{
				result = RES_PARERR;
			}
			break;
		case GET_SECTOR_SIZE:
			if (buffer)
			{
				*(uint32_t*)buffer = sdGetBlockSize();
				result = RES_OK;
			}
			else
			{
				result = RES_PARERR;
			}
			break;
		case GET_BLOCK_SIZE:
			if (buffer)
			{
				*(uint32_t*)buffer = sdGetEraseSize();
				result = RES_OK;
			}
			else
			{
				result = RES_PARERR;

			}
			break;
		default:
			result = RES_PARERR;
			break;
	}

	return result;
}

/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

/*******************************************************************************
 *******************************************************************************
						            INTERRUPT SERVICE ROUTINES
 *******************************************************************************
 ******************************************************************************/

/******************************************************************************/
