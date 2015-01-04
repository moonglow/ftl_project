#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftl.h"
#include "ff10c\src\ff.h"

ftl_status_t ftl_deinit(void);
ftl_status_t  wl_stat(void);
void hal_save_mem_dump(void);
void check_for_mem_leaks(void);

uint8_t buffer[_MAX_SS];
FATFS FatFs;
FIL fil;
FRESULT result;

uint32_t crc32( uint32_t sum, uint8_t  *p, size_t len)
{
#define POLY 0x4C11DB7

	while (len--)
	{
		int i;
		uint8_t byte = *p++;

		for (i = 0; i < 8; ++i)
		{
			uint32_t osum = sum;
			sum <<= 1;
			if (byte & 0x80)
				sum |= 1;
			if (osum & 0x80000000)
				sum ^= POLY;
			byte <<= 1;
		}
	}
	return sum;
}

#define REMOVE_FILE		1
#define DATA_READBACK	1

int main(void)
{
	int i, n;
	uint64_t pure_data_wr = 0;
	DWORD nclst = 0;
	char file_name[] = "test.txt";
	uint32_t crc32_to, crc32_from;

#ifndef NO_FTL
	ftl_init();
#endif

	result = f_mount( &FatFs, "", 1 );
	if (result == FR_NO_FILESYSTEM)
	{
		result = f_mkfs("", 1, 512);
		if (result != FR_OK)
		{
			printf("f_mkfs error  %d\n", result);
			for (;;);
		}
		result = f_mount(&FatFs, "", 1);
	}
	
	if (result != FR_OK)
	{
		printf("f_mount error  %d\n", result);
		for (;;);
	}

	srand( 0xBAADBEEF );
	
	n = 128;
	while (n--)
	{
		putchar('*');
		crc32_to = 0xFFFFFFFF;
		crc32_from = 0xFFFFFFFF;

		result = f_open(&fil, file_name, FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
		if (result != FR_OK)
		{
			printf("f_open error  %d\n", result);
			for (;;);
		}
		/* 1 - 2 MB*/
		int data_blocks = (((rand() % 2) + 1) * 1024 * 1024) / _MAX_SS;
		i = data_blocks;
		while (i--)
		{
			UINT bw = 0;
			memset(buffer, rand() & 0xFF, _MAX_SS);
			if (f_write(&fil, buffer, _MAX_SS, &bw) != FR_OK)
			{
				printf("f_write error  %d\n", result);
				break;
			}
			if (_MAX_SS != bw)
			{
				printf("write size mismatch  %d\n", bw);
				break;
			}

			pure_data_wr += bw;
			crc32_to = crc32(crc32_to, buffer, _MAX_SS);
		}
		result = f_close(&fil);
		if (result != FR_OK)
		{
			printf("f_close error  %d\n", result);
			for (;;);
		}

#if DATA_READBACK
		result = f_open(&fil, file_name, FA_OPEN_EXISTING | FA_READ );
		if (result != FR_OK)
		{
			printf("f_open readback error  %d\n", result);
			for (;;);
		}

		i = data_blocks;
		while (i--)
		{
			UINT br = 0;
			if (f_read(&fil, buffer, _MAX_SS, &br) != FR_OK)
			{
				printf("f_read readback error  %d\n", result);
				break;
			}
			if (_MAX_SS != br)
			{
				printf("read size mismatch  %d\n", br );
				break;
			}

			crc32_from = crc32(crc32_from, buffer, _MAX_SS);
		}

		result = f_close(&fil);
		if (result != FR_OK)
		{
			printf("f_close readback error  %d\n", result);
			for (;;);
		}
		if (crc32_to != crc32_from)
		{
			printf("crc mismatch %.8X %.8X\n", crc32_to, crc32_from );
			for (;;);
		}
#endif

#if REMOVE_FILE
		result = f_unlink(file_name );
		if (result != FR_OK)
		{
			printf("f_unlink error  %d\n", result);
			for (;;);
		}
#endif
	}
	
	printf( "\nPure data wiritten = %ulMB\n", pure_data_wr/1024u/1024u );

#ifndef NO_FTL
	wl_stat();
	ftl_deinit();
	check_for_mem_leaks();
	hal_save_mem_dump();
#endif
	
	return 0;
}