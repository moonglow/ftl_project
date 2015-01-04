#include "fat_io.h"

#include "ff10c\src\diskio.h"
#include "ff10c\src\ff.h"
#include "ftl.h"

#include <string.h>

static uint8_t *flash_mem = 0;
#define FLASH_SIZE			(4*1024*1024)

DSTATUS disk_initialize( BYTE pdrv )
{
#ifdef NO_FTL
	static uint8_t f_was_init = 0;
	if (f_was_init == 0)
	{
		flash_mem = malloc(FLASH_SIZE);
		memset(flash_mem, 0xFF, FLASH_SIZE);
		printf("alloc flash mem %p\n", flash_mem);
		f_was_init = 1;
	}
#endif
	return 0;
}

DSTATUS disk_status(BYTE pdrv)
{
	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
	ftl_status_t result;
	uint8_t buffer[_MAX_SS];

	while (count--)
	{
#ifdef NO_FTL
		memcpy( buffer, &flash_mem[sector*512], _MAX_SS );
#else
		result = ftl_read(sector, buffer);
		if (result != FTL_READ_PAGE_SUCCESS)
			return RES_ERROR;
#endif
		memcpy(buff, buffer, _MAX_SS);
		buff += _MAX_SS;
	}
	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
	ftl_status_t result;
	uint8_t buffer[_MAX_SS];

	while (count--)
	{
		memcpy(buffer, buff, _MAX_SS);
#ifdef NO_FTL
		memcpy(&flash_mem[sector * 512], buffer, _MAX_SS);
#else
		result = ftl_write(sector, buffer);
		if (result != FTL_WRITE_PAGE_SUCCESS)
			return RES_ERROR;
#endif
		buff += _MAX_SS;
	}
	return RES_OK;
}


#if _USE_IOCTL
DRESULT disk_ioctl( BYTE drv, BYTE cmd, void *buff )
{
	DRESULT res;

	if (drv) return RES_PARERR;

	res = RES_ERROR;

	switch (cmd) 
	{
		case CTRL_SYNC:		/* Wait for end of internal write process of the drive */
			res = RES_OK;
		break;
		case GET_SECTOR_COUNT:	/* Get drive capacity in unit of sector (DWORD) */
#ifdef NO_FTL
			*(uint32_t*)buff = (FLASH_SIZE / 512);
#else
			ftl_read_capacity((uint32_t*)buff);
#endif
			res = RES_OK;
		break;
		case GET_BLOCK_SIZE:	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 512;
			res = RES_OK;
		break;
		case CTRL_TRIM:	/* Erase a block of sectors (used when _USE_ERASE == 1) */
			break;
		default:
			res = RES_PARERR;
	}
	return res;
}
#endif