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

uint32_t  test = 0;
uint8_t buffer[2048];
uint8_t buffer_test[512];

FATFS FatFs;
FIL fil;
FRESULT fat_result;

int main(void)
{
	int i, n;
	DWORD nclst = 0;

#ifndef NO_FTL
	ftl_init();
#endif


	fat_result = f_mount( &FatFs, "", 1 );
	if (fat_result == FR_NO_FILESYSTEM)
	{
		fat_result = f_mkfs("", 1, 512);
		if (fat_result != FR_OK)
		{
			for (;;);
		}
		fat_result = f_mount(&FatFs, "", 1);
	}
	else if (fat_result != FR_OK)
	{
		for (;;);
	}

	srand( 0xBAADBEEF );
#define WRITE_FILE_SIZE 512
	uint64_t pure_data_wr = 0;
	n = 256;
	while (n--)
	{
		putchar('*');
		fat_result = f_open(&fil, "test.txt", FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
		if (fat_result != FR_OK)
		{
			for (;;);
		}
		/* 1 - 3 MB*/
		int num_of_mb = ((rand() % 3) + 1);
		i = (num_of_mb * 1024 * 1024) / WRITE_FILE_SIZE;
		while (i--)
		{
			UINT bw = 0;
			memset(buffer, rand() & 0xFF, WRITE_FILE_SIZE );
			if (f_write(&fil, buffer, WRITE_FILE_SIZE , &bw) != FR_OK)
				break;
			if (WRITE_FILE_SIZE != bw)
				break;

			pure_data_wr += bw;
		}
		
		fat_result = f_close(&fil);
		if (fat_result != FR_OK)
		{
			for (;;);
		}
#if 1
		fat_result = f_unlink("test.txt");
		if (fat_result != FR_OK)
		{
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