
#include "hal.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

uint8_t *flash_mem = 0;
#define FLASH_SIZE			(4*1024*1024)
#define FLASH_SECTOR_SIZE	(64*1024)
#define FLASH_PAGE_SIZE		(512)



static int simulate_flash_write(uint8_t *to, uint8_t *from, size_t size)
{
	while (size--)
	{
		if ((to[size] & from[size]) != from[size])
			return -1;

		to[size] &= from[size];
	}
	return 0;
}

ftl_status_t hal_read_page(uint32_t sector, uint8_t *buf)
{
	/* transform page number to page address */
	sector = sector * FLASH_PAGE_SIZE;

	memcpy(buf, &flash_mem[sector], FLASH_PAGE_SIZE);
	return FTL_READ_PAGE_SUCCESS;
}

ftl_status_t hal_write_page(uint32_t sector, uint8_t *buf)
{
	/* transform page number to page address */
	sector = sector * FLASH_PAGE_SIZE;

	//memcpy(&flash_mem[sector], buf, FLASH_PAGE_SIZE);
	if (simulate_flash_write(&flash_mem[sector], buf, FLASH_PAGE_SIZE) < 0)
		return FTL_WRITE_PAGE_FAILURE;
	return FTL_WRITE_PAGE_SUCCESS;
}

ftl_status_t hal_get_block_status(uint32_t addr, uint8_t *buf, uint16_t count)
{
	memcpy(buf, &flash_mem[addr], count);
	return FTL_GET_BLOCK_STATUS_SUCCESS;
}

ftl_status_t hal_set_block_status(uint32_t addr, uint8_t *buf, uint16_t count)
{
	//memcpy(&flash_mem[addr], buf, count);
	if (simulate_flash_write(&flash_mem[addr], buf, count) < 0)
		return FTL_SET_BLOCK_STATUS_FAILURE;
	return FTL_SET_BLOCK_STATUS_SUCCESS;
}

ftl_status_t hal_block_erase(uint32_t addr)
{
	memset(&flash_mem[addr], 0xFF, FLASH_SECTOR_SIZE);
	return FTL_BLOCK_ERASE_SUCCESS;
}

ftl_status_t hal_read_id(uint8_t *buf)
{
	static char f_was_init = 0;
	*(uint32_t*)buf = 0x471F;
	if (f_was_init == 0)
	{
		flash_mem = malloc(FLASH_SIZE);

		FILE *f = fopen("flash.mem", "rb");
		if (f == 0)
		{
			memset(flash_mem, 0xFF, FLASH_SIZE);
		}
		else
		{
			printf("Flash mem read %d bytes\n", fread(flash_mem, 1, FLASH_SIZE, f));
			fclose(f);
		}

		f_was_init = 1;
	}
	return FTL_GET_CHIP_ID_SUCCESS;
}

ftl_status_t hal_test_unit_wr_protect(void)
{
	return FTL_UNIT_WR_NO_PROTECT;
}

ftl_status_t hal_unit_unprotect(void)
{
	return FTL_UNIT_UNPROTECT_SUCCESS;
}

ftl_status_t hal_test_unit_removal(void)
{
	return FTL_UNIT_NO_REMOVAL;
}


void hal_save_mem_dump(void)
{
	FILE *f = fopen("flash.mem", "wb+");
	if (fwrite(flash_mem, 1, FLASH_SIZE, f) != FLASH_SIZE)
	{
		printf("fatal: flash mem write i/o error\n");
	}
	fclose(f);
}