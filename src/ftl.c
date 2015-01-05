#include <stdio.h>
#include "memmgt.h"
#include "hal.h"
#include "ftl.h"

#define DEVICE_ID_LEN 4
#define FTL_BLOCK_MAGIC 0x41544D4C

/*
BLOCK START:
OFFSET 000 DATA[256] ( low part actual page num, hi part is page status )
OFFSET 256 DATA[1] FLAG [PAGE_COUNT?]
OFFSET 258 DATA[2] BLOCK_INDEX
OFFSET 260 DATA[4] ERASE_COUNT
OFFSET 264 DATA[4] FTL_BLOCK_MAGIC
*/

#pragma pack(push,1)
dataflash_dev_t chip_ids[] = 
{
	{
		0x100481F,
		0x10000,
		0x800000,
		0x200,
		"Atmel AT25DF641A"
	},
	{
		0x481F,
		0x10000,
		0x800000,
		0x200,
		"Atmel AT25DF641",
	},
	{
		0x1471F,
		0x10000,
		0x400000,
		0x200,
		"Atmel AT25DF321A",
	},
	{
		0x471F,
		(64*1024u),
		(4*1024*1024u),
		512,
		"Atmel AT25DF321",
	},
	{
		0x2461F,
		0x10000,
		0x200000,
		0x200,
		"Atmel AT25DF161",
	},
	{
		0x101451F,
		0x10000,
		0x100000,
		0x200,
		"Atmel AT25DF081A",
	},
	{
		0x1441F,
		0x10000,
		0x80000,
		0x200,
		"Atmel AT25DF041A",
	},
	{
		0,
	}
};

struct hal_hander_t
{
	ftl_status_t (*read_page)(uint32_t addr, uint8_t *buf );
	ftl_status_t (*write_page)(uint32_t addr, uint8_t *buf );
	ftl_status_t (*get_block_status)(uint32_t addr, uint8_t *buf, uint16_t count);
	ftl_status_t (*set_block_status)(uint32_t addr, uint8_t *buf, uint16_t count);
	ftl_status_t (*block_erase)(uint32_t addr);
	ftl_status_t (*read_id)(uint8_t *buf);
}
g_hal_hander = { 0 };


struct chip_desc_t
{
	uint16_t dev_id;
	uint8_t block_size_shift;
	uint8_t page_shift;
	uint8_t n_pages_per_block;
	uint8_t anonymous_3;
	uint16_t page_size;
	uint32_t block_size;
	uint32_t chip_size;
}
chip_desc = { 0 };


struct block_llist_entry_t
{
	uint16_t n_block;
	uint16_t padd;
	struct block_llist_entry_t *p_next;
};

struct block_array_llist_t
{
	struct block_llist_entry_t *llist_free_blocks[8];
};

struct ftl_struct_t
{
	struct chip_desc_t *p_chip_desc;
	uint16_t n_start_block;
	/* total block in flash */
	uint16_t n_total_blocks;
	/* blocks what we can use for allocation */
	uint16_t n_blocks;
	uint16_t n_good_blocks;
	uint16_t n_free_blocks;
	uint16_t n_bad_blocks;
	struct block_description_t *p_block_desc;
	struct block_array_llist_t *p_block_llist;
	uint8_t *p_page_buffer;
	uint8_t f_is_init;
}
g_ftl_struct = { 0 };


struct block_description_t
{
	uint8_t block_status;
	uint8_t padd_bytes;
	/* yeap names is too cryptic ) will rename it soon */
	uint16_t n_block_b; /* block for pages from 1-127 */
	uint16_t n_block_a; /* block for pages from 128-254 */
};


struct cache_register_t
{
	uint8_t		phy_page[128];
	uint16_t	n_block;
};

struct page_map_t
{
	struct
	{
		uint8_t logic;
		uint8_t phys;
	}
	map[128];
	uint8_t n_pages;
};

struct cache_register_t ftl_cache[4] = { 0xFF };

uint16_t scan_point = 0;
#pragma pack(pop)

static ftl_status_t hal_init(void)
{
	g_hal_hander.read_page = hal_read_page;
	g_hal_hander.write_page = hal_write_page;
	g_hal_hander.get_block_status = hal_get_block_status;
	g_hal_hander.set_block_status = hal_set_block_status;
	g_hal_hander.block_erase = hal_block_erase;
	g_hal_hander.read_id = hal_read_id;
	return FTL_HAL_INIT_SUCCESS;
}

static void convert_id(uint8_t * buf)
{
	return;
}

static int get_shift( uint32_t val )
{
	char bit_pos = 0;

	if (val == 0)
		return 0;

	for (;;)
	{
		val >>= 1;
		if (val == 0)
				break;
		++bit_pos;
	}
	return bit_pos;
}

static dataflash_dev_t *get_chip_para(uint32_t cheap_id)
{
	int n;
	int i;
	uint32_t id;

	n = 0;
	for (i = 0;; ++i)
	{
		id = chip_ids[n].id;
		if (!id)
			break;
		++n;
		if ( id == cheap_id)
			return &chip_ids[i];
	}
	return 0;
}

ftl_status_t ftl_test_unit_ready( void )
{
	if (g_ftl_struct.f_is_init)
		return FTL_UINT_READY;

	return FTL_UINT_NOT_READY;
}

ftl_status_t ftl_test_unit_wr_protect( void )
{
	return hal_test_unit_wr_protect();
}

ftl_status_t ftl_unit_unprotect( void )
{
	return hal_unit_unprotect();
}

ftl_status_t ftl_test_unit_removal( void )
{
	return hal_test_unit_removal();
}

ftl_status_t ftl_test_flag( void )
{
	int n_block;
	uint32_t block_magic;

	n_block = 0;
	for (;;)
	{
		uint32_t block_phys_addr;
		block_phys_addr = (n_block << g_ftl_struct.p_chip_desc->block_size_shift) + 264;
		if (g_hal_hander.get_block_status(block_phys_addr, (uint8_t *)&block_magic, sizeof(uint32_t)) != FTL_GET_BLOCK_STATUS_SUCCESS)
			return FTL_INIT_FAILURE;
		if (block_magic == FTL_BLOCK_MAGIC)
			break;
		n_block += 10;
		if (n_block == 30)
			return FTL_FAILURE;
	}
	return FTL_SUCCESS;
}

ftl_status_t mark_bad_block( int n_block )
{
	uint32_t block_phys_addr;

	block_phys_addr = n_block << (g_ftl_struct.p_chip_desc->block_size_shift - g_ftl_struct.p_chip_desc->page_shift);

	ftl_memset( g_ftl_struct.p_page_buffer, 0x00, g_ftl_struct.p_chip_desc->page_size );
	if ( g_hal_hander.write_page(block_phys_addr, g_ftl_struct.p_page_buffer) == FTL_WRITE_PAGE_SUCCESS )
		return FTL_MARK_BAD_BLOCK_SUCCESS;
	
	return FTL_MARK_BAD_BLOCK_FAILURE;
}

ftl_status_t  ftl_block_erase_all( void )
{
	uint16_t n_block;
	uint32_t block_water_mark = FTL_BLOCK_MAGIC;

	n_block = 0;
	for (;;)
	{
		uint32_t block_phys_addr;
		if ( n_block >= g_ftl_struct.n_total_blocks )
			return FTL_BLOCK_ERASE_SUCCESS;

		block_phys_addr = n_block << g_ftl_struct.p_chip_desc->block_size_shift;
		if (g_hal_hander.block_erase(block_phys_addr) != FTL_BLOCK_ERASE_SUCCESS)
		{
			mark_bad_block(n_block);
			++n_block;
			continue;
		}
		block_phys_addr = (n_block << g_ftl_struct.p_chip_desc->block_size_shift) + 264;
		if (g_hal_hander.set_block_status( block_phys_addr, (uint8_t *)&block_water_mark, sizeof( uint32_t )) != FTL_SET_BLOCK_STATUS_SUCCESS)
			return FTL_BLOCK_ERASE_FAILURE;
		++n_block;
	}
}


ftl_status_t ftl_block_erase(unsigned int n_block, uint32_t *erase_count)
{
	uint32_t phys_addr;
	uint32_t block_water_mark = FTL_BLOCK_MAGIC;
	uint32_t tmp_erases;

	if (g_ftl_struct.n_total_blocks < n_block)
	{
		return FTL_BLOCK_ERASE_FAILURE;
	}
	if (g_ftl_struct.n_start_block > n_block)
	{
		return FTL_BLOCK_ERASE_FAILURE;
	}
	phys_addr = n_block << g_ftl_struct.p_chip_desc->block_size_shift;

	if (g_hal_hander.get_block_status(phys_addr + 260, (uint8_t *)&tmp_erases, sizeof(uint32_t)) != FTL_GET_BLOCK_STATUS_SUCCESS )
	{
		return FTL_BLOCK_ERASE_FAILURE;
	}
	if (g_hal_hander.block_erase(phys_addr) != FTL_BLOCK_ERASE_SUCCESS)
	{
		return FTL_BLOCK_ERASE_FAILURE;
	}
	if (tmp_erases == 0xFFFFFFFF)
		tmp_erases = 0;

	++tmp_erases;

	if (g_hal_hander.set_block_status(phys_addr + 260, (uint8_t *)&tmp_erases, sizeof(uint32_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
	{
			return FTL_BLOCK_ERASE_FAILURE;
	}
	else
	{
		*erase_count = tmp_erases;
		if (g_hal_hander.set_block_status(phys_addr + 264, (uint8_t *)&block_water_mark, sizeof(uint32_t)) == FTL_SET_BLOCK_STATUS_SUCCESS)
		{
			return FTL_BLOCK_ERASE_SUCCESS;
		}
	}
	return FTL_BLOCK_ERASE_FAILURE;
}

ftl_status_t add_free_block( uint32_t nblock, uint32_t block_erase_count )
{
	uint32_t block_llist_index;

	struct block_llist_entry_t *p_block_entry;
	struct block_llist_entry_t *p_head;

	if (g_ftl_struct.n_total_blocks < nblock)
		return FTL_ADD_FREE_BLOCK_FAILURE;
	if (g_ftl_struct.n_start_block > nblock)
		return FTL_ADD_FREE_BLOCK_FAILURE;

	p_block_entry = ftl_malloc(sizeof(struct block_llist_entry_t) );
	if (p_block_entry == 0)
	{
		return FTL_ADD_FREE_BLOCK_FAILURE;
	}
	if (block_erase_count == 0xFFFFFFFF)
	{
		block_erase_count = 0;
	}

	ftl_memset(p_block_entry, 0x00, sizeof(struct block_llist_entry_t) );
	block_llist_index = block_erase_count >> 15;
	p_block_entry->p_next = p_block_entry;
	if (block_llist_index >= 3)
		block_llist_index = 3;
	p_head = g_ftl_struct.p_block_llist->llist_free_blocks[block_llist_index];
	p_block_entry->n_block = nblock;
	if (p_head)
	{
		p_block_entry->p_next = p_head->p_next;
		p_head->p_next = p_block_entry;
	}
	g_ftl_struct.p_block_llist->llist_free_blocks[block_llist_index] = p_block_entry;
	return FTL_ADD_FREE_BLOCK_SUCCESS;
}

ftl_status_t get_free_block( uint16_t *p_n_block )
{
	int i; 
	struct block_llist_entry_t *p_head;
	struct block_llist_entry_t *p_next;

	if (g_ftl_struct.p_block_llist)
	{
		i = 0;
		while (1)
		{
			p_head = g_ftl_struct.p_block_llist->llist_free_blocks[i];
			if (p_head)
				break;
			if ( ++i == 4)
				return FTL_GET_FREE_BLOCK_FAILURE;
		}
		p_next = p_head->p_next;
		*p_n_block = p_next->n_block;
		if ( p_next == p_head )
		{
			ftl_free( p_next );
			g_ftl_struct.p_block_llist->llist_free_blocks[i] = 0;
		}
		else
		{
			p_head->p_next = p_next->p_next;
			ftl_free(p_next);
		}
		return FTL_GET_FREE_BLOCK_SUCCESS;
	}
	return FTL_GET_FREE_BLOCK_FAILURE;
}

void init_page_map( void )
{
	int n;

	for (n = 0; n < 4; n++ )
	{
		ftl_cache[n].n_block = 0xFFFF;
		ftl_memset( &ftl_cache[n], 0xFF, g_ftl_struct.p_chip_desc->n_pages_per_block );
	} 
}

ftl_status_t update_page_map(uint16_t n_block, uint8_t n_phy_page, uint8_t n_logic_page)
{
	if (ftl_cache[n_block & 3].n_block == n_block)
	{
		ftl_cache[n_block & 3].phy_page[n_logic_page] = n_phy_page;
		return FTL_UPDATE_PAGE_MAP_SUCCESS;
	}
	return FTL_BLOCK_PAGE_MAP_NOT_EXIST;
}

void  reset_page_map( uint16_t n_block)
{
	if (ftl_cache[n_block & 3].n_block == n_block)
	{
		ftl_cache[n_block & 3].n_block = 0xFFFFu;
		ftl_memset( &ftl_cache[n_block & 3], 0xFF, g_ftl_struct.p_chip_desc->n_pages_per_block);
	}
}


ftl_status_t conflict_resolve( uint16_t n_block_logic, uint32_t n_block, uint8_t stat)
{
	printf("TODO: conflict_resolve\n");
	for (;;);
	return FTL_CONFLICT_RESOLVE_SUCCESS;
}


ftl_status_t create_page_map( uint16_t n_block)
{
	int i;
	uint8_t n_page;
	uint8_t block_size_shift, n_pages_per_block;
	uint32_t phy_addr;

	block_size_shift = g_ftl_struct.p_chip_desc->block_size_shift;
	n_pages_per_block = g_ftl_struct.p_chip_desc->n_pages_per_block;

	for (i = 0; i < 4;i++)
	{
		if ( ftl_cache[i].n_block == n_block )
			return FTL_CREATE_PAGE_MAP_SUCCESS;
	} 

	if (g_ftl_struct.n_total_blocks <  g_ftl_struct.p_block_desc[n_block].n_block_b)
		return FTL_CREATE_PAGE_MAP_FAILURE;

	if (g_ftl_struct.n_start_block >  g_ftl_struct.p_block_desc[n_block].n_block_b)
		return FTL_CREATE_PAGE_MAP_FAILURE;
	/* convert to sector address num_of_block*size_of_block*/
	phy_addr = g_ftl_struct.p_block_desc[n_block].n_block_b << block_size_shift;
	if (g_hal_hander.get_block_status(phy_addr, g_ftl_struct.p_page_buffer, 268) != FTL_GET_BLOCK_STATUS_SUCCESS)
		return FTL_CREATE_PAGE_MAP_FAILURE;

	/* init cache with specific block */
	ftl_cache[n_block & 3].n_block = n_block;
	ftl_memset( &ftl_cache[n_block & 3], 0xFF, g_ftl_struct.p_chip_desc->n_pages_per_block);
	/* fill actual phys page number to cache phy_page array */
	for (i = 1; i < g_ftl_struct.p_chip_desc->n_pages_per_block; i++ )
	{
		n_page = g_ftl_struct.p_page_buffer[i];
		if ( n_page  >= n_pages_per_block)
		{
			/* just erased flash part ?*/
			if (n_page != 0xFF)
			{
				return  FTL_CREATE_PAGE_MAP_FAILURE;
			}
			break;
		}
		if (g_ftl_struct.p_page_buffer[i + 128] == 0xA5 )
		{
			ftl_cache[(n_block & 3)].phy_page[n_page] = i;
		}
	}

	if (g_ftl_struct.p_block_desc[n_block].n_block_a == 0xFFFF)
		return FTL_CREATE_PAGE_MAP_SUCCESS;

	if (g_ftl_struct.n_total_blocks < g_ftl_struct.p_block_desc[n_block].n_block_a )
		return FTL_CREATE_PAGE_MAP_FAILURE;
	
	if( g_ftl_struct.n_start_block > g_ftl_struct.p_block_desc[n_block].n_block_a )
		return FTL_CREATE_PAGE_MAP_FAILURE;

	/* convert to sector address num_of_block*size_of_block*/
	phy_addr = g_ftl_struct.p_block_desc[n_block].n_block_a << block_size_shift;

	if (g_hal_hander.get_block_status(phy_addr, g_ftl_struct.p_page_buffer, 268) != FTL_GET_BLOCK_STATUS_SUCCESS)
		return FTL_CREATE_PAGE_MAP_FAILURE;

	for (i = 1; i < g_ftl_struct.p_chip_desc->n_pages_per_block; i++)
	{
		n_page = g_ftl_struct.p_page_buffer[i];
		if (n_page >= n_pages_per_block)
		{
			if (n_page == 0xFF)
				return FTL_CREATE_PAGE_MAP_SUCCESS;
			return  FTL_CREATE_PAGE_MAP_FAILURE;
		}
		if (g_ftl_struct.p_page_buffer[i+128] == 0xA5 )
		{
			ftl_cache[(n_block & 3)].phy_page[n_page] = n_pages_per_block + i - 1;
		}
	}
	return FTL_CREATE_PAGE_MAP_SUCCESS;
}

ftl_status_t copy_page_map( uint16_t n_block, uint8_t *page_mem)
{
	uint8_t i;
	uint8_t n; 

	if (ftl_cache[n_block & 3].n_block != n_block)
	{
		if (create_page_map(n_block) != FTL_CREATE_PAGE_MAP_SUCCESS)
		{
			return FTL_COPY_PAGE_MAP_FAILURE;
		}
	}


	i = 0;
	n = 0;
	while (i < g_ftl_struct.p_chip_desc->n_pages_per_block )
	{
		if (ftl_cache[n_block & 3].phy_page[i] != 0xFF)
		{
			page_mem[2 * n] = i;
			page_mem[2 * n + 1] = ftl_cache[n_block & 3].phy_page[i];
			++n;
		}
		++i;
	}
	page_mem[256] = n;
	return FTL_COPY_PAGE_MAP_SUCCESS;
}

ftl_status_t ftl_deinit( void )
{
	int i;
	struct block_llist_entry_t  *p_block_llist_entry;
	struct block_llist_entry_t *p_next;

	ftl_free( g_ftl_struct.p_block_desc );
	ftl_free( g_ftl_struct.p_page_buffer );
	i = 0;
	do
	{
		p_block_llist_entry = g_ftl_struct.p_block_llist->llist_free_blocks[i];
		if (p_block_llist_entry)
		{
			while (1)
			{
				p_next = p_block_llist_entry->p_next;
				if (p_next == p_block_llist_entry)
					break;
				p_next = p_next->p_next;
				ftl_free(p_block_llist_entry->p_next);
				p_block_llist_entry->p_next = p_next;
			}
			ftl_free( p_block_llist_entry );
			g_ftl_struct.p_block_llist->llist_free_blocks[i] = 0;
		}
		++i;
	} 
	while (i != 4);
	ftl_free(g_ftl_struct.p_block_llist);

	chip_desc.dev_id = 0;
	chip_desc.page_size = 0;
	chip_desc.block_size = 0;
	chip_desc.chip_size = 0;
	chip_desc.page_shift = 0;
	chip_desc.block_size_shift = 0;
	chip_desc.n_pages_per_block = 0;

	g_ftl_struct.p_chip_desc = 0;
	g_ftl_struct.n_start_block = 0;
	g_ftl_struct.n_total_blocks = 0;
	g_ftl_struct.n_blocks = 0;
	g_ftl_struct.n_bad_blocks = 0;
	g_ftl_struct.n_free_blocks = 0;
	g_ftl_struct.n_good_blocks = 0;
	g_ftl_struct.p_block_desc = 0;
	g_ftl_struct.p_page_buffer = 0;
	g_ftl_struct.p_block_llist = 0;
	g_ftl_struct.f_is_init = 0;

	return FTL_SUCCESS;
}

ftl_status_t ftl_init(void)
{
	uint32_t flash_dev_id = 0;
	dataflash_dev_t *p_flash_dev;
	ftl_status_t result;

	if (g_ftl_struct.f_is_init)
		return FTL_INIT_SUCCESS;

	if (hal_init() != FTL_HAL_INIT_SUCCESS)
		return FTL_INIT_FAILURE;

	if (g_hal_hander.read_id((uint8_t*)&flash_dev_id) != FTL_GET_CHIP_ID_SUCCESS)
		return FTL_INIT_FAILURE;

	convert_id((uint8_t*)&flash_dev_id);
	p_flash_dev = get_chip_para(flash_dev_id);
	if (!p_flash_dev)
		return FTL_INIT_FAILURE;

	chip_desc.dev_id = flash_dev_id;
	chip_desc.chip_size = p_flash_dev->chip_size;
	chip_desc.page_size = p_flash_dev->page_size;
	chip_desc.block_size = p_flash_dev->block_size;

	/* get power of two for page size */
	chip_desc.page_shift = get_shift(chip_desc.page_size);
	/* power of two for block size */
	chip_desc.block_size_shift = get_shift(chip_desc.block_size);

	g_ftl_struct.n_total_blocks = (chip_desc.chip_size >> chip_desc.block_size_shift );
	g_ftl_struct.n_blocks = (chip_desc.chip_size >> chip_desc.block_size_shift)-2;

	chip_desc.n_pages_per_block = 1 << (chip_desc.block_size_shift - chip_desc.page_shift);
	g_ftl_struct.p_chip_desc = &chip_desc;

	g_ftl_struct.n_start_block = 0;
	g_ftl_struct.n_bad_blocks = 0;
	g_ftl_struct.n_free_blocks = 0;
	g_ftl_struct.n_good_blocks = 0;
	
	g_ftl_struct.p_block_desc = ftl_malloc( sizeof( struct block_description_t ) * g_ftl_struct.n_blocks);
	if (g_ftl_struct.p_block_desc == 0)
		return FTL_INIT_FAILURE;
	ftl_memset(g_ftl_struct.p_block_desc, 0xFF, sizeof(struct block_description_t) * g_ftl_struct.n_blocks);

	g_ftl_struct.p_page_buffer = ftl_malloc( chip_desc.page_size );
	if ( g_ftl_struct.p_page_buffer == 0 )
		return FTL_INIT_FAILURE;

	g_ftl_struct.p_block_llist = ftl_malloc(32);
	if (g_ftl_struct.p_block_llist == 0)
		return FTL_INIT_FAILURE;

	ftl_memset(g_ftl_struct.p_block_llist, 0x00, 32);

	if (hal_test_unit_wr_protect() == FTL_UNIT_WR_PROTECT)
	{
		if (hal_unit_unprotect() != FTL_UNIT_UNPROTECT_SUCCESS)
			return FTL_INIT_FAILURE;
	}

	if (ftl_test_flag() == FTL_FAILURE)
	{
		ftl_block_erase_all();
	}

	uint32_t n_block;
	uint32_t phys_addr;
	uint8_t block_stat = 0;
	uint16_t logic_block_n = 0;
	uint32_t block_erase_count = 0;

	for (n_block = 0; g_ftl_struct.n_total_blocks > n_block; ++n_block)
	{
		phys_addr = (n_block << chip_desc.block_size_shift) + 256;
		if (g_hal_hander.get_block_status(phys_addr, &block_stat, sizeof(uint8_t)) != FTL_GET_BLOCK_STATUS_SUCCESS)
		{
			return FTL_INIT_FAILURE;
		}
		if (block_stat == 0)
		{
			++g_ftl_struct.n_bad_blocks;
			continue;
		}

		phys_addr = (n_block << chip_desc.block_size_shift) + 258;
		if (g_hal_hander.get_block_status(phys_addr, (uint8_t *)&logic_block_n, 2) != FTL_GET_BLOCK_STATUS_SUCCESS)
			return FTL_INIT_FAILURE;

		phys_addr = (n_block << chip_desc.block_size_shift) + 260;
		if (g_hal_hander.get_block_status(phys_addr, (uint8_t *)&block_erase_count, 4) != FTL_GET_BLOCK_STATUS_SUCCESS)
			return FTL_INIT_FAILURE;

		if (logic_block_n == 0xFFFFu)
		{
			if (add_free_block(n_block, block_erase_count) != FTL_ADD_FREE_BLOCK_SUCCESS)
				return FTL_INIT_FAILURE;
			++g_ftl_struct.n_free_blocks;
			continue;
		}
		/* status start condition ( 0b1010 ) broken */
		if ((block_stat & 0xF0) != 0xA0)
		{
			if (ftl_block_erase(n_block, &block_erase_count ) != FTL_BLOCK_ERASE_SUCCESS)
			{
				result = mark_bad_block(n_block);
				++g_ftl_struct.n_bad_blocks;
				if (result != FTL_MARK_BAD_BLOCK_SUCCESS)
				{
					return FTL_INIT_FAILURE;
				}
			}
			if (add_free_block(n_block, block_erase_count) != FTL_ADD_FREE_BLOCK_SUCCESS)
				return FTL_INIT_FAILURE;
			++g_ftl_struct.n_free_blocks;
			continue;
		}
		if (g_ftl_struct.n_blocks < logic_block_n)
		{
			return FTL_INIT_FAILURE;
		}
		uint8_t block_status, f_ext_block;
		/* check for ext pages bit ( 0x04 ) */
		f_ext_block = (block_stat >> 2) & 3;
		block_status = g_ftl_struct.p_block_desc[logic_block_n].block_status;
		if (block_status == 0xFF)
		{
			g_ftl_struct.p_block_desc[logic_block_n].block_status = block_stat;
		}
		/* block status conflict */
		else if ((block_status ^ block_stat) & 3)
		{
			if (conflict_resolve(logic_block_n, n_block, block_stat) != FTL_CONFLICT_RESOLVE_SUCCESS)
				return FTL_INIT_FAILURE;
			continue;
		}
		if (f_ext_block)
		{
			if (f_ext_block != 1)
			{
				return FTL_INIT_FAILURE;
			}
			g_ftl_struct.p_block_desc[logic_block_n].n_block_a = n_block;
		}
		else
		{
			g_ftl_struct.p_block_desc[logic_block_n].n_block_b = n_block;
		}
		++g_ftl_struct.n_good_blocks;
	}

	int i;
	uint16_t hi_pages = 0, lo_pages = 0;
	uint16_t f_swap = 0;
	for (i = 0;; ++i )
	{
		if (g_ftl_struct.n_blocks <= i)
		{
			init_page_map();
			g_ftl_struct.f_is_init = 1;
			return FTL_INIT_SUCCESS;
		}
		hi_pages = g_ftl_struct.p_block_desc[i].n_block_a;
		lo_pages = g_ftl_struct.p_block_desc[i].n_block_b;
		f_swap = hi_pages + 1;

		if (hi_pages != 0xFFFF)
			f_swap = 1;

		if (lo_pages != 0xFFFF)
			f_swap = 0;
		else
			f_swap = f_swap & 1;

		if (f_swap == 0)
			continue;

		phys_addr = (hi_pages << chip_desc.block_size_shift) + 256;
		if (g_hal_hander.get_block_status(phys_addr, &block_stat, sizeof( uint8_t ) ) == FTL_GET_BLOCK_STATUS_SUCCESS)
		{
			block_stat &= 0xF3u;
			if (g_hal_hander.set_block_status(phys_addr, &block_stat, sizeof(uint8_t) ) == FTL_SET_BLOCK_STATUS_SUCCESS)
			{
				g_ftl_struct.p_block_desc[i].n_block_b = hi_pages;
				g_ftl_struct.p_block_desc[i].n_block_a = 0xFFFFu;
				continue;
			}
		}
		return FTL_GARBAGE_COLLECTION_FAILURE;
	}
}

ftl_status_t get_phy_page( uint16_t n_block, uint8_t n_logic_page, uint8_t *p_n_page )
{
	if (create_page_map(n_block) == FTL_CREATE_PAGE_MAP_SUCCESS)
	{
		*p_n_page = ftl_cache[n_block & 3].phy_page[n_logic_page];
		return FTL_GET_PHY_PAGE_SUCCESS;
	}

	return FTL_GET_PHY_PAGE_FAILURE;
}


ftl_status_t ftl_read_capacity(uint32_t *nb_sectors)
{
	if (g_ftl_struct.f_is_init == 0)
		return FTL_READ_CAPACITY_FAILURE;

	*nb_sectors = (~g_ftl_struct.n_blocks) + (g_ftl_struct.n_blocks << (g_ftl_struct.p_chip_desc->block_size_shift - g_ftl_struct.p_chip_desc->page_shift));
	
	return FTL_READ_CAPACITY_SUCCESS;
}


ftl_status_t  wl_stat( void )
{
	uint16_t n_start;
	uint16_t n_total_blocks;
	struct chip_desc_t *p_desc;
	uint32_t *p_block_uses;
	uint32_t erase_count; 
	ftl_status_t result;

	n_start = g_ftl_struct.n_start_block;
	n_total_blocks = g_ftl_struct.n_total_blocks;
	p_desc = g_ftl_struct.p_chip_desc;
	p_block_uses = (uint32_t *)ftl_malloc(sizeof(uint32_t) * (n_total_blocks - n_start));
	while (n_start < n_total_blocks)
	{
		result = g_hal_hander.get_block_status( (n_start << p_desc->block_size_shift) + 260, (uint8_t *)&erase_count, sizeof( uint32_t ) );
		if (result != FTL_GET_BLOCK_STATUS_SUCCESS)
			return result;
		if ( erase_count == 0xFFFFFFFF)
			erase_count = 0;
		p_block_uses[n_start] = erase_count;
		++n_start;
	}
#if 1
	int i;
	printf("wl_stat output:\n");
	for (i = g_ftl_struct.n_start_block; i < (n_total_blocks - g_ftl_struct.n_start_block); i++)
	{
		printf("SECTOR %.2d\t%d\r\n", i, p_block_uses[i]);
	}
#endif
	ftl_free( p_block_uses );
	return result;
}

ftl_status_t ftl_get_phy_block(uint16_t n_block, uint8_t f_extended, uint8_t stat)
{
	uint8_t		block_size_shift;
	uint16_t	n_free_block;
	uint32_t	phy_addr;

	block_size_shift = g_ftl_struct.p_chip_desc->block_size_shift;

	if (get_free_block(&n_free_block) != FTL_GET_FREE_BLOCK_SUCCESS)
		return FTL_GET_PHY_BLOCK_FAILURE;

	if (g_ftl_struct.n_total_blocks < n_free_block)
		return FTL_GET_PHY_BLOCK_FAILURE;
	if (g_ftl_struct.n_start_block > n_free_block)
		return FTL_GET_PHY_BLOCK_FAILURE;

	--g_ftl_struct.n_free_blocks;

	phy_addr = (n_free_block << block_size_shift) + 258;
	/* set logical block number to physical block*/
	if (g_hal_hander.set_block_status( phy_addr, (uint8_t *)&n_block, sizeof(uint16_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
		return FTL_GET_PHY_BLOCK_FAILURE;

	stat |= 0xA0;
	stat |= (4 * f_extended);

	phy_addr = (n_free_block << block_size_shift) + 256;
	if (g_hal_hander.set_block_status(phy_addr, &stat, sizeof(uint8_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
		return FTL_GET_PHY_BLOCK_FAILURE;
	
	if (f_extended)
		g_ftl_struct.p_block_desc[n_block].n_block_a = n_free_block;
	else
		g_ftl_struct.p_block_desc[n_block].n_block_b = n_free_block;

	++g_ftl_struct.n_good_blocks;
	return FTL_GET_PHY_BLOCK_SUCCESS;
}


ftl_status_t garbage_collect(uint16_t n_block, uint16_t *p_block_for_use, uint8_t *p_page_num)
{
	uint8_t page_map_mem[257];
	uint8_t last_page;
	uint8_t n_pages_per_block;
	uint8_t block_size_shift;
	uint8_t page_shift;
	uint16_t n_block_free;
	uint32_t phy_addr;
	uint16_t current_block;
	uint8_t block_status;
	uint32_t block_erase_count;
	struct block_description_t *p_block_desc;
	ftl_status_t result;
	int i, n;

	n_pages_per_block = g_ftl_struct.p_chip_desc->n_pages_per_block;
	block_size_shift = g_ftl_struct.p_chip_desc->block_size_shift;
	page_shift = g_ftl_struct.p_chip_desc->page_shift;

	if (create_page_map(n_block) != FTL_CREATE_PAGE_MAP_SUCCESS)
		return FTL_GARBAGE_COLLECTION_FAILURE;

	ftl_memset(page_map_mem, 0, 257);

	if (copy_page_map(n_block, page_map_mem) != FTL_COPY_PAGE_MAP_SUCCESS)
		return FTL_GARBAGE_COLLECTION_FAILURE;

	n = 0;
	do
	{
		last_page = n;
		if (n >= page_map_mem[256])
			break;
		++n;
	}
	/* only for extended pages */
	while (page_map_mem[2 * n - 1] > (n_pages_per_block - 1));

	if (last_page == (n_pages_per_block - 1))
	{
		current_block = g_ftl_struct.p_block_desc[n_block].n_block_b;
		if (ftl_block_erase(current_block, &block_erase_count) == FTL_BLOCK_ERASE_SUCCESS)
		{
			if (add_free_block(current_block, block_erase_count) != FTL_ADD_FREE_BLOCK_SUCCESS)
				return FTL_GARBAGE_COLLECTION_FAILURE;
			
			++g_ftl_struct.n_free_blocks;

			current_block = g_ftl_struct.p_block_desc[n_block].n_block_a;
			phy_addr = (current_block << block_size_shift) + 256;
			if (g_hal_hander.get_block_status(phy_addr, (uint8_t *)&block_status, sizeof(uint8_t)) != FTL_GET_BLOCK_STATUS_SUCCESS)
				return FTL_GARBAGE_COLLECTION_FAILURE;

			block_status &= 0xF3u;
			if (g_hal_hander.set_block_status(phy_addr, (uint8_t *)&block_status, sizeof(uint8_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
				return FTL_GARBAGE_COLLECTION_FAILURE;

			p_block_desc = &g_ftl_struct.p_block_desc[n_block];
			p_block_desc->n_block_b = current_block;
			p_block_desc->n_block_a = 0xFFFFu;

			if (p_page_num == 0)
				return FTL_GRABAGE_COLLECTION_SUCCESS;

			if (ftl_get_phy_block(n_block, 1, block_status & 3) == FTL_GET_PHY_BLOCK_SUCCESS)
			{
				*p_page_num = 1;
				*p_block_for_use = g_ftl_struct.p_block_desc[n_block].n_block_a;
				return FTL_GRABAGE_COLLECTION_SUCCESS;
			}
			return FTL_WRITE_PAGE_FAILURE;	
		}
		else
		{
			result  = mark_bad_block( current_block );
			++g_ftl_struct.n_bad_blocks;
			if ( result  != FTL_MARK_BAD_BLOCK_SUCCESS)
				return FTL_GARBAGE_COLLECTION_FAILURE;

			current_block = g_ftl_struct.p_block_desc[n_block].n_block_a;
			phy_addr = (current_block << block_size_shift) + 256;
			if (g_hal_hander.get_block_status( phy_addr, (uint8_t *)&block_status, sizeof( uint8_t ) ) != FTL_GET_BLOCK_STATUS_SUCCESS)
				return FTL_GARBAGE_COLLECTION_FAILURE;

			block_status &= 0xF3u;
			if (g_hal_hander.set_block_status(phy_addr, (uint8_t *)&block_status, sizeof(uint8_t) ) != FTL_SET_BLOCK_STATUS_SUCCESS)
				return FTL_GARBAGE_COLLECTION_FAILURE;

			p_block_desc = &g_ftl_struct.p_block_desc[n_block];
			p_block_desc->n_block_b = current_block;
			p_block_desc->n_block_a = 0xFFFFu;

			if ( p_page_num == 0 )
				return FTL_GRABAGE_COLLECTION_SUCCESS;

			if (ftl_get_phy_block(n_block, 1, block_status & 3) == FTL_GET_PHY_BLOCK_SUCCESS)
			{
				*p_page_num = 1;
				*p_block_for_use = g_ftl_struct.p_block_desc[n_block].n_block_a;
				return FTL_GRABAGE_COLLECTION_SUCCESS;
			}
			return FTL_WRITE_PAGE_FAILURE;
		}
		for (;;);
	}

	if (get_free_block(&n_block_free) != FTL_GET_FREE_BLOCK_SUCCESS)
		return FTL_GARBAGE_COLLECTION_FAILURE;
	--g_ftl_struct.n_free_blocks;

	phy_addr = (n_block_free << block_size_shift) + 258;
	if (g_hal_hander.set_block_status(phy_addr, (uint8_t *)&n_block, sizeof(uint16_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
		return FTL_GARBAGE_COLLECTION_FAILURE;

	block_status = 0xA5;

	for (i = 0; page_map_mem[256] > i; i++)
	{
		uint8_t * p_page_buffer;
		uint8_t phy_page, logic_page;
		int offset_in_pages;

		p_page_buffer = g_ftl_struct.p_page_buffer;

		p_block_desc = &g_ftl_struct.p_block_desc[n_block];
		phy_page = page_map_mem[2 * i + 1];
		/* current page from hi part pages 128-254 ?*/
		if (phy_page > (n_pages_per_block - 1))
		{
			current_block = p_block_desc->n_block_a;
			phy_page -= (n_pages_per_block - 1);
		}
		else
		{
			/* low part pages 1-127 */
			current_block = p_block_desc->n_block_b;
		}
		/* calculate page number */
		offset_in_pages = (current_block << (block_size_shift - page_shift)) + phy_page;

		if (g_hal_hander.read_page(offset_in_pages, p_page_buffer) != FTL_READ_PAGE_SUCCESS)
			return FTL_GARBAGE_COLLECTION_FAILURE;

		logic_page = page_map_mem[2 * i];

		phy_addr = (n_block_free << block_size_shift) + i + 1;
		if (g_hal_hander.set_block_status(phy_addr, &logic_page, sizeof(uint8_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
			return FTL_GARBAGE_COLLECTION_FAILURE;

		phy_addr = (n_block_free << (block_size_shift - page_shift)) + i + 1;
		if (g_hal_hander.write_page(phy_addr, p_page_buffer) != FTL_WRITE_PAGE_SUCCESS)
			return FTL_GARBAGE_COLLECTION_FAILURE;

		phy_addr = (n_block_free << block_size_shift) + i + 129;
		/* update page status ! */
		if (g_hal_hander.set_block_status(phy_addr, (uint8_t *)&block_status, sizeof(uint8_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
			return FTL_GARBAGE_COLLECTION_FAILURE;
	}

	if (p_page_num)
	{
		*p_page_num = i + 1;
		*p_block_for_use = n_block_free;
	}

	/* low pages */
	current_block = g_ftl_struct.p_block_desc[n_block].n_block_b;
	phy_addr = (current_block << block_size_shift) + 256;
	if (g_hal_hander.get_block_status(phy_addr, (uint8_t *)&block_status, sizeof(uint8_t)) != FTL_GET_BLOCK_STATUS_SUCCESS)
		return FTL_GARBAGE_COLLECTION_FAILURE;

	block_status &= 3;
	if (block_status == 3)
	{
		block_status = 0;
	}
	else
	{
		++block_status;
	}
	block_status |= 0xA0;

	phy_addr = (n_block_free << block_size_shift) + 256;
	if (g_hal_hander.set_block_status(phy_addr, (uint8_t *)&block_status, sizeof(uint8_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
		return FTL_GARBAGE_COLLECTION_FAILURE;

	g_ftl_struct.p_block_desc[n_block].block_status = block_status;
	g_ftl_struct.p_block_desc[n_block].n_block_b = n_block_free;
	reset_page_map(n_block);

	if (ftl_block_erase(current_block, &block_erase_count) == FTL_BLOCK_ERASE_SUCCESS)
	{
		if (add_free_block(current_block, block_erase_count) != FTL_ADD_FREE_BLOCK_SUCCESS)
			return FTL_GARBAGE_COLLECTION_FAILURE;
		++g_ftl_struct.n_free_blocks;
	}
	else
	{
		result = mark_bad_block(current_block);
		++g_ftl_struct.n_bad_blocks;
		if (result != FTL_MARK_BAD_BLOCK_SUCCESS)
			return FTL_GARBAGE_COLLECTION_FAILURE;
	}

	if (p_page_num)
	{
		if (*p_page_num == n_pages_per_block)
		{
			current_block = g_ftl_struct.p_block_desc[n_block].n_block_a;
			if (ftl_get_phy_block(n_block, 1, block_status & 0x5F) != FTL_GET_PHY_BLOCK_SUCCESS)
				return FTL_WRITE_PAGE_FAILURE;
			*p_page_num = 1;
			*p_block_for_use = g_ftl_struct.p_block_desc[n_block].n_block_a;
		}
		else
		{
			current_block = g_ftl_struct.p_block_desc[n_block].n_block_a;
			g_ftl_struct.p_block_desc[n_block].n_block_a = 0xFFFFu;
		}
	}
	else
	{
		current_block = g_ftl_struct.p_block_desc[n_block].n_block_a;
		g_ftl_struct.p_block_desc[n_block].n_block_a = 0xFFFFu;
	}

	if (ftl_block_erase(current_block, &block_erase_count) == FTL_BLOCK_ERASE_SUCCESS)
	{
		if (add_free_block(current_block, block_erase_count) == FTL_ADD_FREE_BLOCK_SUCCESS)
		{
			++g_ftl_struct.n_free_blocks;
			return FTL_GRABAGE_COLLECTION_SUCCESS;
		}
		return FTL_GARBAGE_COLLECTION_FAILURE;
	}
	if (mark_bad_block(current_block) == FTL_MARK_BAD_BLOCK_SUCCESS)
		result = FTL_GRABAGE_COLLECTION_SUCCESS;
	else
		result = FTL_GARBAGE_COLLECTION_FAILURE;
	++g_ftl_struct.n_bad_blocks;
	return result;
}

ftl_status_t do_defrag(int tmp_scan_point)
{
	char f_grb_col;

	if (g_ftl_struct.p_block_desc[tmp_scan_point].n_block_b == 0xFFFF)
		f_grb_col = 0;
	else
		f_grb_col = 1;

	if (g_ftl_struct.p_block_desc[tmp_scan_point].n_block_a == 0xFFFF)
		f_grb_col = 0;
	
	if (f_grb_col)
	{
		if (garbage_collect(tmp_scan_point, 0, 0) == FTL_GRABAGE_COLLECTION_SUCCESS)
			return FTL_DEFRAG_SUCCESS;

		return FTL_DEFRAG_FAILURE;
	}
	return FTL_DEFRAG_SUCCESS;
}

ftl_status_t ftl_defrag(void)
{
	uint16_t start_block;
	uint16_t next_block;
	ftl_status_t result;

	start_block = scan_point;
	do
	{
		result = do_defrag(scan_point);
		if (result != FTL_DEFRAG_SUCCESS)
			break;
		next_block = ++scan_point;
		if (g_ftl_struct.n_free_blocks > 3u)
			break;
		/* last block ?*/
		if (next_block == g_ftl_struct.n_blocks)
			scan_point = 0;
	} 
	/* overflow control */
	while (scan_point != start_block);
	return result;
}

/* sector - logical address, buf - sizeof page  */
ftl_status_t  ftl_read(unsigned int sector, uint8_t *buf)
{
	uint16_t n_block;
	uint16_t block_current;
	uint8_t block_size_shift;
	uint8_t page_shift;
	uint8_t n_pages_per_block;
	uint8_t n_phys_page;

	block_size_shift = g_ftl_struct.p_chip_desc->block_size_shift;
	page_shift = g_ftl_struct.p_chip_desc->page_shift;
	n_pages_per_block = g_ftl_struct.p_chip_desc->n_pages_per_block;

	/* sector address is valid ? */
	if (sector >= ((uint32_t)g_ftl_struct.n_blocks * (n_pages_per_block - 1)))
		return FTL_READ_PAGE_FAILURE;

	n_block = sector / (n_pages_per_block - 1);
	block_current = g_ftl_struct.p_block_desc[n_block].n_block_b;
	/* block was never used, so just emulate reading */
	if (block_current == 0xFFFF)
	{
		ftl_memset(buf, 0xFF, g_ftl_struct.p_chip_desc->page_size);
		return FTL_READ_PAGE_SUCCESS;
	}

	/* get physical page address */
	if (get_phy_page(n_block, (sector - ((n_pages_per_block - 1) * n_block)), &n_phys_page) != FTL_GET_PHY_PAGE_SUCCESS)
	{
		return FTL_READ_PAGE_FAILURE;
	}
	/* never write before to this page, just emulate reading */
	if (n_phys_page == 0xFF)
	{
		ftl_memset(buf, 0xFF, g_ftl_struct.p_chip_desc->page_size);
		return FTL_READ_PAGE_SUCCESS;
	}

	/* extended pages ? */
	if (n_phys_page > (n_pages_per_block - 1))
	{
		block_current = g_ftl_struct.p_block_desc[n_block].n_block_a;
		n_phys_page = n_phys_page - (n_pages_per_block - 1);
	}

	return g_hal_hander.read_page((block_current << (block_size_shift - page_shift)) + n_phys_page, buf);
}

ftl_status_t  ftl_write( uint32_t sector, uint8_t *buf )
{
	uint8_t *	p_page_buffer;
	uint8_t		n_pages_per_block;
	uint8_t		block_size_shift;
	uint8_t		page_shift;
	uint16_t	n_block;
	uint8_t		n_page;
	uint8_t		page_status;
	struct		block_description_t *p_block_desc;
	uint16_t	n_block_a, n_block_b, block_tmp;
	uint8_t		i;

	p_page_buffer = g_ftl_struct.p_page_buffer;
	n_pages_per_block = g_ftl_struct.p_chip_desc->n_pages_per_block;
	block_size_shift = g_ftl_struct.p_chip_desc->block_size_shift;
	page_shift = g_ftl_struct.p_chip_desc->page_shift;
	n_block = sector / (n_pages_per_block - 1);
	n_page = sector - (n_pages_per_block - 1) * n_block;
	/* mark page as used, good pattern 0b1010 0101 */
	page_status = 0xA5u;

	if (sector >= ((uint32_t)(n_pages_per_block - 1) * g_ftl_struct.n_blocks) )
		return FTL_WRITE_PAGE_FAILURE;

	if (g_ftl_struct.n_free_blocks <= 2 )
	{
		char f_defrag;

		p_block_desc = &g_ftl_struct.p_block_desc[n_block];
		/* we have filled hi part ? */
		block_tmp = p_block_desc->n_block_a;
		if (block_tmp == 0xFFFF)
			block_tmp = p_block_desc->n_block_b;
		i = 1;

		if ( block_tmp != 0xFFFF )
		{
			if (g_hal_hander.get_block_status(block_tmp << block_size_shift, g_ftl_struct.p_page_buffer, 268)  != FTL_GET_BLOCK_STATUS_SUCCESS )
				return FTL_WRITE_PAGE_FAILURE;
			while ( i < n_pages_per_block)
			{
				if ( p_page_buffer[i] == 0xFF)
					break;
				++i;
			}
		}

		if (i == n_pages_per_block)
			f_defrag = 1;
		else
			f_defrag = (block_tmp == 0xFFFF);

		if (f_defrag && ftl_defrag() != FTL_DEFRAG_SUCCESS)
			return FTL_WRITE_PAGE_FAILURE;
	}
		
	n_block_b = g_ftl_struct.p_block_desc[n_block].n_block_b;
	if (n_block_b == 0xFFFF)
	{
		if (ftl_get_phy_block(n_block, 0, 0) != FTL_GET_PHY_BLOCK_SUCCESS)
			return FTL_WRITE_PAGE_FAILURE;
		p_block_desc = &g_ftl_struct.p_block_desc[n_block];
		p_block_desc->block_status = 0xA0u;
		n_block_b = p_block_desc->n_block_b;
	}
	n_block_a = g_ftl_struct.p_block_desc[n_block].n_block_a;
	if (n_block_a != 0xFFFF)
		n_block_b = g_ftl_struct.p_block_desc[n_block].n_block_a;

	block_tmp = n_block_b;

	if (g_hal_hander.get_block_status(n_block_b << block_size_shift, p_page_buffer, 268) != FTL_GET_BLOCK_STATUS_SUCCESS)
		return FTL_WRITE_PAGE_FAILURE;
	i = 1;
	/* get free entry */
	while (i < n_pages_per_block)
	{
		if (p_page_buffer[i] == 0xFF)
			break;
		++i;
	}
	if (i == n_pages_per_block)
	{
		if (n_block_a != 0xFFFF)
		{
			if (garbage_collect(n_block, &block_tmp, &i) != FTL_GRABAGE_COLLECTION_SUCCESS)
				return FTL_WRITE_PAGE_FAILURE;
		}
		else
		{
			if (ftl_get_phy_block(n_block, 1, p_page_buffer[256] & 3) != FTL_GET_PHY_BLOCK_SUCCESS)
				return FTL_WRITE_PAGE_FAILURE;
			p_block_desc = &g_ftl_struct.p_block_desc[n_block];
			n_block_a = p_block_desc->n_block_a;
			i = 1;
			block_tmp = p_block_desc->n_block_a;
		}
	}

	if ( g_hal_hander.set_block_status( (block_tmp << block_size_shift) + i, &n_page, sizeof( uint8_t ) ) != FTL_SET_BLOCK_STATUS_SUCCESS )
		return FTL_WRITE_PAGE_FAILURE;
	if (g_hal_hander.write_page((block_tmp << (block_size_shift - page_shift)) + i, buf) != FTL_WRITE_PAGE_SUCCESS)
		return FTL_WRITE_PAGE_FAILURE;
	if (g_hal_hander.set_block_status((block_tmp << block_size_shift) + 128 + i, &page_status, sizeof(uint8_t)) != FTL_SET_BLOCK_STATUS_SUCCESS)
		return FTL_WRITE_PAGE_FAILURE;


	if (n_block_a != 0xFFFF)
		i = (n_pages_per_block - 1) + i;

	/* if we have cache for this block, update phy_page for logical page entry */
	if (ftl_cache[n_block & 3].n_block == n_block)
		ftl_cache[n_block & 3].phy_page[n_page] = i;

	return FTL_WRITE_PAGE_SUCCESS;
}
