/* diskio.c is originally from FatFs.
 * I modified this to link FatFs to my driver functions an now works as glue / adapter function
 * EnjOy :) */
#include "ff.h"
#include "diskio.h"
#include "sdDriver.h"

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

