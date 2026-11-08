# ST7789-V10

STM32F401RE (Nucleo-F401RE) learning project: an ST7789 TFT and a microSD card sharing one SPI bus, with a hand-written SD driver, a UART debug layer, and a FatFs file test.

**Branch `feature/sd-debug`** adds the SD driver, the sector-level debug tools and the FatFs test on top of the TFT code.

## Hardware

| Part | Notes |
|------|-------|
| Board | NUCLEO-F401RE (STM32F401xE, Cortex-M4) |
| Display | ST7789 TFT, 320x240, landscape, 16-bit RGB565, inversion on |
| Storage | microSD on SPI (SPI mode, not SDIO) |
| Debug | USART2 via ST-LINK virtual COM port |

### Pins (from `main.h`)

| Signal | Pin |
|--------|-----|
| TFT_CS | PB6 |
| TFT_DC | PA9 |
| TFT_RST | PC7 |
| SD_CS | PA8 |
| USART2 TX / RX | PA2 / PA3 |
| SPI1 SCK / MISO / MOSI | Nucleo default: PA5 / PA6 / PA7 |

TFT and SD share SPI1; each has its own chip-select. Both CS lines are driven high before any transfer so neither device sees stray traffic.

**UART settings**

- Speed: 115200
- Data: 8 bit
- Parity: None
- Stop bits: 1
- Flow control: None
- RTS: Enable
- DTR: Enable


## Clock and SPI speed

- HSI with PLL gives **84 MHz** SYSCLK. APB2 (SPI1) runs at 84 MHz.
- SD init runs at prescaler 256 (about 328 kHz), because the card must be initialised at 400 kHz or less.
- After init the driver switches to prescaler 4 (about 21 MHz).

## Project layout (`Core/Src`, `Core/Inc`)

| File | Role |
|------|------|
| `main.c` | Init order: TFT, SD driver init, sector dump and probe, TFT text test, FatFs test |
| `tft.c/.h` | ST7789 driver: init, fill, 5x7 font text with 1x to 4x scaling |
| `sd.c/.h` | Basic SPI-mode SD driver: `sd_init`, `sd_read_block`, `sd_write_block` |
| `sdDriver_debug.c/.h` | Debug tools using only the basic driver (no FatFs): hexdump, MBR probe, error strings |
| `FatFs_debug.c` + header | FatFs-side helpers: `fr_str()` result text and `uart_print()` printf-style UART output |
| `fs_test.c/.h` | FatFs test: mount, list root, read `HELLO.TXT`, append to `LOG.TXT` |

### Two debug layers, kept separate

- **Driver level (no FatFs):** `sdDriver_debug.*` reports `SD_*` codes through `sd_err_str()`.
- **FatFs level:** `FatFs_debug.c` reports `FRESULT` codes through `fr_str()`.

Both follow the same pattern: a `const char *const` text array plus a bounds-checked lookup that returns `"Unknown error"`.

## SD driver (`sd.c`)

Init sequence, with each step printed over UART:

1. 80 clocks with CS high.
2. `CMD0` go idle (expects R1 = 0x01).
3. `CMD8` voltage check (checks voltage range and 0xAA echo; an illegal-command reply means a v1.x card).
4. `ACMD41` (`CMD55` + `CMD41`), looped until ready, 1 s timeout.
5. `CMD58` reads the OCR and checks the power-up bit, then CCS (bit 30) to detect SDHC.
6. `CMD16` sets the block length to 512 (SDSC only).

Addressing is by byte for SDSC and by block for SDHC; the driver handles this internally. Reads and writes are single 512-byte blocks (`CMD17` / `CMD24`). Data CRC is read and ignored.

### Return codes (`sd.h`)

| Code | Name | Text from `sd_err_str()` |
|------|------|--------------------------|
| 0 | `SD_OK` | Function succeeded |
| 1 | `SD_ERR_CMD0` | CMD0: no idle reply (card missing, wiring or CS) |
| 2 | `SD_ERR_CMD8` | CMD8: bad reply, voltage or echo mismatch |
| 3 | `SD_ERR_ACMD41` | ACMD41: card not ready within 1 s |
| 4 | `SD_ERR_CMD58` | CMD58: OCR read failed or power-up bit not set |
| 5 | `SD_ERR_CMD16` | CMD16: block length 512 rejected |
| 6 | `SD_ERR_CMD17` | CMD17: read command rejected |
| 7 | `SD_ERR_READ_TOKEN` | No 0xFE data token after CMD17 |
| 8 | `SD_ERR_CMD24` | CMD24: write command rejected |
| 9 | `SD_ERR_WRITE_DATA` | Write data response not accepted |
| 10 | `SD_ERR_WRITE_BUSY` | Card stayed busy after write (timeout) |

## Debug tools (`sdDriver_debug.c`)

| Function | What it does |
|----------|--------------|
| `sd_dump_sector(n)` | Reads sector `n` and prints it as a hexdump |
| `uart_hexdump(buf, len)` | Prints `buf` as decimal offset, hex offset, 16 hex bytes and an ASCII column |
| `sd_probe_partition()` | Reads sector 0 and checks the `55 AA` signature. Detects a bare FAT boot sector (jump byte `EB` or `E9`). Otherwise decodes MBR partition 1 (type at offset 450, start LBA at 454, size at 458) and dumps its boot sector |
| `sd_err_str(rc)` | Turns an `SD_*` code into text |

## FatFs test (`fs_test.c`)

`fs_run_test()` mounts the volume, lists the root folder, prints `HELLO.TXT` in 63-byte chunks, then opens `LOG.TXT` with `FA_WRITE | FA_OPEN_APPEND` and appends one line. Every call appends, so `LOG.TXT` grows by 18 bytes on each boot.

## Example UART output

SD driver init, MBR probe and FatFs test on a 2 GB SDSC card (CCS=0, FAT16):

```
CMD0 R1 = 0x01
CMD8 R1 = 0x01 R7 = 00 00 01 AA
ACMD41 R1 = 0x00 (CMD55 = 0x01) 1 ms
CMD58 R1=0x00 OCR=80 FF 80 00 CCS=0
CMD16 R1 = 0x00
--- sector 0, rc=0 ---
 ...                                         (hexdump of the MBR)
probe: MBR type=0x0C start=2048 size=3909599
--- sector 2048, rc=0 ---
 ...                                         (hexdump of the FAT boot sector)
mount: 0 (Function succeeded)
opendir: 0 (Function succeeded)
LOST.DIR  0 bytes
SYSTEM~1  0 bytes
LOG.TXT  180 bytes
HELLO.TXT  75 bytes
open LOG.TXT: 0 (Function succeeded)
write: 0 (Function succeeded), 18 bytes
close: 0 (Function succeeded)
```

How to read the hexdump: the MBR partition entry sits at offset 446 (`0x1BE`), and the last two bytes of each sector are the `55 AA` signature.

## Build and run

1. Open the project in STM32CubeIDE.
2. Build and flash to the Nucleo (ST-LINK).
3. Open the ST-LINK virtual COM port in a serial terminal- settings as given above. 
4. Reset the board and watch the init log.

CubeMX regeneration is safe: all custom code lives in separate files, and `main.c` changes sit inside `USER CODE` blocks.

## Known limitations

- Single-block transfers only (no multi-block `CMD18` / `CMD25`).
- No retry on `CMD0` and no card-detect handling.
- The hexdump is blocking UART (100 ms timeout per line).
- The FatFs disk I/O layer (`diskio` glue to `sd_read_block` / `sd_write_block`) yet to be documented.
