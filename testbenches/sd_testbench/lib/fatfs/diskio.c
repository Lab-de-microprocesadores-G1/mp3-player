/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various existing      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"					/* Obtains integer types */
#include "diskio.h"				/* Declarations of disk functions */
#include "drivers/HAL/sd/sd.h"	/* SD Card controller or driver */

/* Definitions of physical drive number for each drive */
#define	DEVICE_SD		0

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	DSTATUS stat = STA_NODISK;

	switch (pdrv)
	{
		case DEVICE_SD:
			stat = (sdGetState() == SD_STATE_INITIALIZED) ? 0 : STA_NOINIT;
			break;
	}

	return stat;
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat = STA_NODISK;

	switch (pdrv)
	{
		case DEVICE_SD:
			stat = sdCardInit() ? 0 : STA_NOINIT;
			break;
	}

	return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_PARERR;

	switch (pdrv)
	{
		case DEVICE_SD:
			res = sdRead((uint32_t*)buff, sector, count) ? RES_OK : RES_ERROR;
			break;
	}

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_PARERR;

	switch (pdrv)
	{
		case DEVICE_SD:
			res = sdWrite((uint32_t*)buff, sector, count) ? RES_OK : RES_ERROR;
			break;
	}

	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_PARERR;

	switch (pdrv)
	{
		case DEVICE_SD:
			switch (cmd)
			{
				case CTRL_SYNC:
					res = RES_OK;
					break;
				case GET_SECTOR_COUNT:
					if (buff)
					{
						*(uint32_t*)buff = sdGetBlockCount();
						res = RES_OK;
					}
					else
					{
						res = RES_PARERR;
					}
					break;
				case GET_SECTOR_SIZE:
					if (buff)
					{
						*(uint32_t*)buff = sdGetBlockSize();
						res = RES_OK;
					}
					else
					{
						res = RES_PARERR;
					}
					break;
				case GET_BLOCK_SIZE:
					if (buff)
					{
						*(uint32_t*)buff = sdGetEraseSize();
						res = RES_OK;
					}
					else
					{
						res = RES_PARERR;

					}
					break;
				default:
					res = RES_PARERR;
					break;
			}
			break;
	}

	return res;
}

