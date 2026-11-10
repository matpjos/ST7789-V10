/*
 * fs_test.c
 *
 *  Created on: 30 Oct 2026
 *      Author: matpj
 *
 * Works on top of FatFs.
 * Reads content from HELLO.TXT (if available) saved in the SD card and displays over UART.
 * Write a string to 'LOG.TXT' file SD card; Later to develop this to accept log data to save to SD card
 */

#include "fs_test.h"
#include <FatFs_debug.h>
#include "ff.h"

static FATFS fs;  //Filesystem object structure (FATFS); work area for the volume (holds a 512-byte window)
static FIL fil;   //File object structure; one open file at a time (also holds a 512-byte buffer)

void fs_run_test(void)
{
	FRESULT fr = f_mount
			  (   //f_mount Function gives "work area" to the FatFs module.
			      &fs, // fs is FATFS structure
				  "",	//logical drive number; this is default
				  1		// Initialisation option; 1= Force-mounted the volume
				  	  	// to check if it is ready to work.
			  );

	uart_print("mount: %d (%s)\r\n", fr, fr_str(fr));
	if (fr != FR_OK) return;

	/* ---- list the root folder ---- */
	if (fr == FR_OK)
	{
	    DIR dir;     // Directory object structure (DIR)
		FILINFO fno; // File/directory information structure (FILINFO)

	    fr = f_opendir(&dir, "/"); // fills dir and puts its cursor at the first entry of the root folder.
	    uart_print("opendir: %d (%s)\r\n", fr, fr_str(fr));

	    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) // f_readdir(...) runs, so the call itself is what advances the cursor and fills fno; fno.fname[0] fails if no more file object exists.
	    	uart_print("%s  %lu bytes\r\n", fno.fname, (unsigned long)fno.fsize);
		  /* f_readdir(&dir, &fno) does two things:
		   * 1. It copies the entry at the cursor into fno (name, size, date, attributes).
		   * 2. It moves the cursor to the next entry.
		   * When there are no entries left, it returns FR_OK and sets fno.fname[0]
		   * to 0 (an empty name). That is the end-of-list signal.*/

	    f_closedir(&dir);
	}


	/* ---- read HELLO.TXT in chunks ---- */
	char line[64];
	UINT br;

	fr = f_open  // FATFS function
		   (
			   &fil, //Pointer to the file object structure
			   "HELLO.TXT", // File name
			   FA_READ // Mode flag
		   );
	uart_print("open HELLO.TXT: %d (%s)\r\n", fr, fr_str(fr));
	if (fr == FR_OK)
	{
		 do
		 {
		     f_read
			 (
			     &fil, //[IN]  File object
				 line, //[OUT] Buffer to store read data
				 sizeof line - 1, // [IN] Number of bytes to read
				 &br	//[OUT] Number of bytes read
			 );
			 line[br] = 0; //null character at the end of string
			 uart_print("%s", line);
		 } while (br == sizeof line - 1); // check if more chunk(s) left to read, repeat until all finish
		 uart_print("\r\n");
	     f_close(&fil);
	}


	// start- write to SD test code
	UINT bw;
	const char txt[] = "hello from STM32\r\n";

	fr = f_open(&fil, "LOG.TXT", FA_WRITE | FA_OPEN_APPEND); //to-do replaced &wf
	uart_print("open LOG.TXT: %d (%s)\r\n", fr, fr_str(fr));
	if (fr == FR_OK)
	{
	    fr = f_write(&fil, txt, sizeof txt - 1, &bw);
		uart_print("write: %d (%s), %u bytes\r\n", fr, fr_str(fr), bw); //to-do replaced &wf
		fr = f_close(&fil);                 /* close flushes everything to the card */
		uart_print("close: %d (%s)\r\n", fr, fr_str(fr));
	}
	//end- write to SD test code
}
