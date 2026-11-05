/* diskio.c is originally from FatFs.
 * I modified this to link FatFs to my driver functions an now works as glue / adapter function
 * EnjOy :) */
#include "ff.h"
#include "diskio.h"
#include "sd.h"

static DSTATUS stat = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv)
{
    return pdrv ? STA_NOINIT : stat;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv) return STA_NOINIT;
    stat = (sd_init() == SD_OK) ? 0 : STA_NOINIT;
    return stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv || !count) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++)
        if (sd_read_block(sector + i, buff + 512 * i) != SD_OK)
            return RES_ERROR;
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv) return RES_PARERR;
    return (cmd == CTRL_SYNC) ? RES_OK : RES_PARERR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv || !count) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++)
        if (sd_write_block(sector + i, buff + 512 * i) != SD_OK)
            return RES_ERROR;
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

/*#include "ff.h"			 Basic definitions of FatFs
#include "diskio.h"		 Declarations FatFs MAI

 Example: Declarations of the platform and disk functions in the project
#include "platform.h"
#include "storage.h"

 Example: Mapping of physical drive number for each drive
#define DEV_FLASH	0	 Map FTL to physical drive 0
#define DEV_MMC		1	 Map MMC/SD card to physical drive 1
#define DEV_USB		2	 Map USB MSD to physical drive 2


-----------------------------------------------------------------------
 Get Drive Status
-----------------------------------------------------------------------

DSTATUS disk_status (
	BYTE pdrv		 Physical drive nmuber to identify the drive
)
{
	DSTATUS stat;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		result = RAM_disk_status();

		// translate the reslut code here

		return stat;

	case DEV_MMC :
		result = MMC_disk_status();

		// translate the reslut code here

		return stat;

	case DEV_USB :
		result = USB_disk_status();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



-----------------------------------------------------------------------
 Inidialize a Drive
-----------------------------------------------------------------------

DSTATUS disk_initialize (
	BYTE pdrv				 Physical drive nmuber to identify the drive
)
{
	DSTATUS stat;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		result = RAM_disk_initialize();

		// translate the reslut code here

		return stat;

	case DEV_MMC :
		result = MMC_disk_initialize();

		// translate the reslut code here

		return stat;

	case DEV_USB :
		result = USB_disk_initialize();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



-----------------------------------------------------------------------
 Read Sector(s)
-----------------------------------------------------------------------

DRESULT disk_read (
	BYTE pdrv,		 Physical drive nmuber to identify the drive
	BYTE *buff,		 Data buffer to store read data
	LBA_t sector,	 Start sector in LBA
	UINT count		 Number of sectors to read
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here

		result = RAM_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_MMC :
		// translate the arguments here

		result = MMC_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_USB :
		// translate the arguments here

		result = USB_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



-----------------------------------------------------------------------
 Write Sector(s)
-----------------------------------------------------------------------

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			 Physical drive nmuber to identify the drive
	const BYTE *buff,	 Data to be written
	LBA_t sector,		 Start sector in LBA
	UINT count			 Number of sectors to write
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here

		result = RAM_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_MMC :
		// translate the arguments here

		result = MMC_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_USB :
		// translate the arguments here

		result = USB_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}

#endif


-----------------------------------------------------------------------
 Miscellaneous Functions
-----------------------------------------------------------------------

DRESULT disk_ioctl (
	BYTE pdrv,		 Physical drive nmuber (0..)
	BYTE cmd,		 Control code
	void *buff		 Buffer to send/receive control data
)
{
	DRESULT res;
	int result;

	switch (pdrv) {
	case DEV_RAM :

		// Process of the command for the RAM drive

		return res;

	case DEV_MMC :

		// Process of the command for the MMC/SD card

		return res;

	case DEV_USB :

		// Process of the command the USB drive

		return res;
	}

	return RES_PARERR;
}*/

