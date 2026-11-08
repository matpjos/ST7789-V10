/*
 * sd_debug.c
 *
 *  Created on: 24 Oct 2026
 *      Author: matpj
 *
 * Reading SD sectors for debug purpose. Only using basic drivers- Not FatFs.
 *
 * ## Important functions ##
 * sd_probe_partition() : decoding sector-0 bytes
 * sd_dump_sector()     : Reads data of a sector, calls uart_hexdump()
 * uart_hexdump()       : UART printing of buf[]
 */
#include "sd_debug.h"
#include "sd.h"
#include "main.h"
#include "usart.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static uint8_t sd_buf[512];
static uint32_t le32(const uint8_t *p);



/* 21/10/2026
 * Prints hex from buf[] * */
void uart_hexdump(const uint8_t *buf, uint16_t len)
{
	static const char hex[] = "0123456789ABCDEF";
    char line[96];

    /* header row, aligned with the columns below */
    const char *hdr = "  DEC  HEX   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)hdr, strlen(hdr), 100);

    for (uint16_t off = 0; off < len; off += 16)
    {
        /* decimal offset (width 5), then hex offset (4 digits) */
        int p = snprintf(line, sizeof line, "%5u  %04X  ", (unsigned)off, (unsigned)off);

        /* hex bytes, with an extra gap after byte 7 */
        for (int i = 0; i < 16; i++)
        {
            if (off + i < len) {
                uint8_t b = buf[off + i];
                line[p++] = hex[b >> 4];
                line[p++] = hex[b & 0xF];
            }
            else {
                line[p++] = ' '; line[p++] = ' ';
            }

            line[p++] = ' ';
            if (i == 7)
            	line[p++] = ' ';
        }

        /* ASCII column */
        line[p++] = '|';
        for (int i = 0; i < 16 && off + i < len; i++)
        {
            uint8_t c = buf[off + i];
            line[p++] = (c >= 32 && c <= 126) ? (char)c : '.';
        }
        line[p++] = '|';
        line[p++] = '\r'; line[p++] = '\n';

        HAL_UART_Transmit(&huart2, (uint8_t *)line, p, 100);
    }
}



/* if a sector is given, loads data to sd_buf
 * then triggers the uart_hexdump() to print the the data in that sector*/
void sd_dump_sector(uint32_t sector)
{
    char m[48];
    uint8_t rc = sd_read_block(sector, sd_buf);
    int k = snprintf(m, sizeof m, "--- sector %lu, rc=%u ---\r\n", (unsigned long)sector, rc);
    if (k > (int)sizeof m - 1) k = sizeof m - 1;
    if (k > 0) HAL_UART_Transmit(&huart2, (uint8_t *)m, k, 100);
    if (rc == 0) uart_hexdump(sd_buf, 512);
}



/*Reads LE from 4 consecutive bytes and converts to single 32bit number*/
static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}


/* Check partition type by decoding "sector-0" bytes
 * Finally calls sd_dump_sector(). */
void sd_probe_partition(void)
{
    char m[80];
    int k;

    /* offset 0   = 0xEB / 0xE9 indicates FAT boot sector
     * offset 510 = 0x55 and
     * offset 511 = 0xAA indicates boot signature
     * offset 450 = type (in MBR, SECTOR-0)
     * offset 454 = Partition start sector, little-endian; (in MBR, SECTOR-0) */
    if (sd_read_block(0, sd_buf) != 0) {
        k = snprintf(m, sizeof m, "probe: read sector 0 failed\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)m, k, 100);
        return;
    	}
    if (sd_buf[510] != 0x55 || sd_buf[511] != 0xAA) {
        k = snprintf(m, sizeof m, "probe: no 55 AA signature\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)m, k, 100);
        return;
    }

    /* A FAT boot sector starts with a jump instruction (EB xx 90 or E9 xx xx) */
    if (sd_buf[0] == 0xEB || sd_buf[0] == 0xE9) {
        k = snprintf(m, sizeof m, "probe: no MBR, sector 0 is a FAT boot sector\r\n");
        HAL_UART_Transmit(&huart2, (uint8_t *)m, k, 100);
        return;
    }

    /* MBR partition entry 1 sits at offset 446 */
    uint8_t  type  = sd_buf[450];
    uint32_t start = le32(&sd_buf[454]);
    uint32_t count = le32(&sd_buf[458]);

    k = snprintf(m, sizeof m, "probe: MBR type=0x%02X start=%lu size=%lu\r\n",
                 type, (unsigned long)start, (unsigned long)count);
    if (k > (int)sizeof m - 1) k = sizeof m - 1;
    HAL_UART_Transmit(&huart2, (uint8_t *)m, k, 100);

    sd_dump_sector(start);              /* the FAT boot sector, if the table is sane */
}






