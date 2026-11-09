/*
 * sd.c
 *
 *  Created on: 24 Oct 2026
 *      Author: matpj
 *
 *    ### Basic SD Driver code ###
 *
 */

#include "sd.h"
#include "main.h"      /* SD_CS / TFT_CS pin names */
#include "spi.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

static uint8_t sd_is_hc = 0; //reassigned during CMD58

static uint8_t spi_xfer(uint8_t b);
static void    sd_set_slow(void);
static void    sd_set_fast(void);
static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc);
static void    sd_deselect(void);
static uint8_t sd_init_inner(void);
static uint8_t sd_wait_ready(uint32_t timeout_ms);



uint8_t sd_init(void)
{
    sd_is_hc = 0; //reassigned during CMD58
    uint8_t rc = sd_init_inner(); // sd_init() was created and sd_init_inner() moved into it to avoid repeated sd_set_slow() call
    sd_set_fast();
    return rc;
}



static uint8_t sd_init_inner(void)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET); //de-select both to make sure pins are not in  stale-state
    sd_set_slow();

    for (int i = 0; i < 10; i++) spi_xfer(0xFF);    /* 80 clocks, CS high (sd de-selected; card needs at least 74 clocks to wake into SPI-ready state. 10 bytes is the usual round number. */

    /* ---- CMD0: go idle ---- */
    uint8_t r = sd_cmd(0, 0, 0x95);
				/* CMD0, arg = 0, CRC = 0X95
				 * expected response: 0x01 (“idle, no errors”)*/
    sd_deselect();

    //print r over usart
    char msg[64];
    int n = snprintf(msg, sizeof msg, "CMD0 R1 = 0x%02X\r\n", r);
    if (n > (int)sizeof msg - 1) n = sizeof msg - 1;
    if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

    if (r != 0x01)
    { 	//no point continuing
    	return SD_ERR_CMD0;
    }

    /* ---- CMD8: voltage check, arg = 0x1AA, CRC = 0x87 ---- */
    uint8_t r7[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t is_v2 = 0;

	r = sd_cmd(8, 0x000001AA, 0x87);

	if (r == 0x01) {   /* v2+ card: 4 more bytes follow */
		for (int i = 0; i < 4; i++)
			r7[i] = spi_xfer(0xFF);
	}
	sd_deselect();

	n = snprintf(msg, sizeof msg, "CMD8 R1 = 0x%02X R7 = %02X %02X %02X %02X\r\n", r, r7[0], r7[1], r7[2], r7[3]);
	if (n > (int)sizeof msg - 1) n = sizeof msg - 1;
	if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

	if (r == 0x01)
	{	/* r7[2] = voltage accepted (0x01 = 2.7-3.6 V), r7[3] = echo of check pattern */
		if (r7[2] != 0x01 || r7[3] != 0xAA)
			return SD_ERR_CMD8;          /* bad voltage range or echo mismatch */
		is_v2 = 1;
	}

	else if (r & 0x04)
	{
		is_v2 = 0;                       /* illegal command: v1.x SDSC card, still valid */
	}

	else
	{
		return SD_ERR_CMD8;              /* no reply (0xFF) or unexpected R1 */
	}


		/* ---- ACMD41: loop CMD55 + CMD41 until R1 == 0x00 (ready) ---- */
	uint32_t t0 = HAL_GetTick();
	uint8_t r55, r41;
	do
	{
		r55 = sd_cmd(55, 0, 0x01);      /* next command is an ACMD */
		sd_deselect();

		r41 = sd_cmd(41, is_v2 ? 0x40000000 : 0, 0x01);
		sd_deselect();
	} while (r41 != 0x00 && (HAL_GetTick() - t0) < 1000);   /* ~1 s timeout */

	uint32_t dt = HAL_GetTick() - t0;

	n = snprintf(msg, sizeof msg, "ACMD41 R1 = 0x%02X (CMD55 = 0x%02X) %lu ms\r\n",
			r41, r55, (unsigned long)dt);
	if (n > (int)sizeof msg - 1) n = sizeof msg - 1;
	if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

	if (r41 != 0x00) return SD_ERR_ACMD41;

	/* ---- CMD58: read OCR, check CCS ---- */
	uint8_t ocr[4] = {0,0,0,0};
	r = sd_cmd(58, 0, 0x01);
	if (r == 0x00)
	{   /* no longer idle */
		for (int i = 0; i < 4; i++) ocr[i] = spi_xfer(0xFF);
	}

	sd_deselect();

	if (r != 0x00 || !(ocr[0] & 0x80))
	{   /* failed, or power-up bit not set */
		if (r != 0x00)
		{
			n = snprintf(msg, sizeof msg, "CMD58 failed, R1 = 0x%02X\r\n", r);
		}
		else
		{
			n = snprintf(msg, sizeof msg, "OCR power-up bit not set, OCR[0] = 0x%02X\r\n", ocr[0]);
		}
		if (n > (int)sizeof msg - 1)
			n = sizeof msg - 1;
		if (n > 0)
			HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

		return SD_ERR_CMD58;
	}

	sd_is_hc = (ocr[0] & 0x40) ? 1 : 0;              /* bit 30 = CCS */

	n = snprintf(msg, sizeof msg, "CMD58 R1=0x%02X OCR=%02X %02X %02X %02X CCS=%u\r\n",
			r, ocr[0], ocr[1], ocr[2], ocr[3], sd_is_hc);
	if (n > (int)sizeof msg - 1)
		n = sizeof msg - 1;
	if (n > 0)
		HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

/* ---- CMD16: block length = 512 (SDSC only, SDHC ignores this CMD, so no harm) ---- */
	if (!sd_is_hc)
		{
			r = sd_cmd(16, 512, 0x01);
			sd_deselect();

			n = snprintf(msg, sizeof msg, "CMD16 R1 = 0x%02X\r\n", r);
			if (n > (int)sizeof msg - 1)
				n = sizeof msg - 1;
			if (n > 0)
				HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

			if (r != 0x00)
				return SD_ERR_CMD16;     /* 0xFF = no reply, other bits = error flags */
		}
	/* init done: */
	return SD_OK;
}

static uint8_t spi_xfer(uint8_t b)
{
    uint8_t rx = 0xFF;
    HAL_SPI_TransmitReceive(&hspi1, &b, &rx, 1, 100);
    return rx;
}

static void sd_set_slow(void)
{
    __HAL_SPI_DISABLE(&hspi1);                      /* BR must not change while SPE=1 */
    MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_256);
    __HAL_SPI_ENABLE(&hspi1);
}

static void sd_set_fast(void)
{
    __HAL_SPI_DISABLE(&hspi1);
    MODIFY_REG(hspi1.Instance->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_4);
    __HAL_SPI_ENABLE(&hspi1);
}


/* send a command to sd
 * using spi_xfer underneath*/
static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
    spi_xfer(0x40 | cmd);
    spi_xfer((uint8_t)(arg >> 24));
    spi_xfer((uint8_t)(arg >> 16));
    spi_xfer((uint8_t)(arg >> 8));
    spi_xfer((uint8_t)arg);
    spi_xfer(crc);

    uint8_t r = 0xFF;                               /* 0xFF = no reply */
    for (int i = 0; i < 10; i++) {
        r = spi_xfer(0xFF);
        if ((r & 0x80) == 0) break;
        /* R1 always has bit 7 = 0
         * so bit7==0 indicates we got the correct r received from sd
         * hence break the loop */
    }
    return r;
}


 /* basic driver to read a block; CMD17*/
uint8_t sd_read_block (uint32_t sector, uint8_t *buf)
	{
	uint32_t addr = sd_is_hc ? sector : sector * 512;
	/* block vs byte addressing */

	uint8_t r = sd_cmd(17, addr, 0x01);
	if (r != 0x00) // r=0 indicates cmd17 ok
	{
		// r!=0 indicates cmd17 not-ok
		sd_deselect();

		char msg[64];
		int n = snprintf(msg, sizeof msg, "Error, CMD17 R1 = 0x%02X (expected 0x00)\r\n", r);
		if (n > (int)sizeof msg - 1) n = sizeof msg - 1;
		if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

		return SD_ERR_CMD17;
	}

	uint8_t tok;
	uint32_t t0 = HAL_GetTick();
	do {
	   tok = spi_xfer(0xFF); /* wait for the data token */
		}
	while (tok == 0xFF && (HAL_GetTick() - t0) < 200);

	if (tok != 0xFE)
	{
	    sd_deselect();

	    char msg[96];
		int n = snprintf(msg, sizeof msg, "CMD17 data token = 0x%02X (expected 0xFE)%s\r\n", tok, (tok == 0xFF) ? ", timed out" : "");
		if (n > (int)sizeof msg - 1) n = sizeof msg - 1;
		if (n > 0) HAL_UART_Transmit(&huart2, (uint8_t *)msg, n, 100);

	    return SD_ERR_READ_TOKEN; } /* 0xFE = start of data */

	for (int i = 0; i < 512; i++)
		buf[i] = spi_xfer(0xFF); // loading data to buf[]
	spi_xfer(0xFF);  /* 2 CRC16 bytes: read, ignore */
	spi_xfer(0xFF);

	sd_deselect();
	return SD_OK;
	}



/* wait until the card releases MISO, <returns 1> (reads 0xFF) or timeout (ms) <returns 0> */
static uint8_t sd_wait_ready(uint32_t timeout_ms)
{
	uint32_t t0 = HAL_GetTick();
	uint8_t b;
	do
	{
		b = spi_xfer(0xFF);
		if (b == 0xFF) return 1; // released MISO, success
	} while ((HAL_GetTick() - t0) < timeout_ms);
	return 0; //timeout return
}

/* Basic write driver function
 * CMD24*/
uint8_t sd_write_block(uint32_t sector, const uint8_t *buf)
{
    uint32_t addr = sd_is_hc ? sector : sector * 512;

    uint8_t r = sd_cmd
    			(
    			    24, //CMD
					addr, 0x01 // CRC
				);
    if (r != 0x00) // need to be 0x00 to be "accepted"
    	{ sd_deselect(); return SD_ERR_CMD24; } // to-do

    spi_xfer(0xFF);                 /* one gap byte */
    spi_xfer(0xFE);                 /* start-of-data token */
    for (int i = 0; i < 512; i++)
        spi_xfer(buf[i]);			// send data
    spi_xfer(0xFF);                 /* 2 dummy CRC bytes */
    spi_xfer(0xFF);

    uint8_t resp = spi_xfer(0xFF);  /* data response */
    if ((resp & 0x1F) != 0x05) { sd_deselect(); return SD_ERR_WRITE_DATA; }

    if (!sd_wait_ready(500)) { sd_deselect(); return SD_ERR_WRITE_BUSY; }   /* Response Timeout error */

    sd_deselect();
    return SD_OK;
}


static void sd_deselect(void)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET); //SD_CS HIGH
    spi_xfer(0xFF);                                 /* 8 extra clocks release MISO */
}
