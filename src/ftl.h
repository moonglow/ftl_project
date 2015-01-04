/**
 * \file
 *
 * \brief FTL(Flash translation layer)
 *
 * Copyright (C) 2011 Atmel Corporation. All rights reserved.
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 * Atmel AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef __STRUCTURE_H__
#define __STRUCTURE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "ftl_status.h"

//! Structure for nor flash
typedef struct _dataflash_dev {
	//! memory id
	uint32_t id;
	//! memory block size(byte)
	uint32_t block_size;
	//! memory total size(byte)
	uint32_t chip_size;
	//! memory page size(512bytes)
	uint16_t page_size;
	//! memory description
	uint8_t *desc;
}dataflash_dev_t;

/**
 * \brief FTL Structure initialization
 *
 * This function initialize ftl structure. Read Nor flash ID
 * and build map table. This routine will erase all the blocks
 * of the chip if FTL has never been used on this data flash before.
 *
 * \return status
 *   \retval FTL_INIT_SUCCESS    Success
 *   \retval FTL_INIT_FAILURE    Failure
 */
ftl_status_t ftl_init(void);

/**
 * \brief FTL Page read from memory
 *
 * This function read one page(512bytes) from memory.
 *
 * \param sector        The logical sector number
 * \param buf           buf to store data read from memory
 *
 * \return status
 *   \retval FTL_READ_PAGE_SUCCESS    Success
 *   \retval FTL_READ_PAGE_FAILURE    Failure
 */
ftl_status_t ftl_read(uint32_t sector, uint8_t *buf);

/**
 * \brief FTL Page write to memory
 *
 * This function write one page(512bytes) to memory.
 *
 * \param sector        The logical sector number
 * \param buf           buf to store data write to memory
 *
 * \return status
 *   \retval FTL_WRITE_PAGE_SUCCESS    Success
 *   \retval FTL_WRITE_PAGE_FAILURE    Failure
 */
ftl_status_t ftl_write(uint32_t sector, uint8_t *buf);

/**
 * \brief FTL Test memory state
 *
 * This function test the memory if it's ready or not ready.
 *
 * \return status
 *   \retval FTL_UINT_READY        Ready
 *   \retval FTL_UINT_NOT_READY    Not ready
 */
ftl_status_t ftl_test_unit_ready(void);

/**
 * \brief FTL Read memory capacity
 *
 * This function read the memory capacity, store number of sectors
 * in pointer nb_sectors.
 *
 * \param nb_sector     Pointer to a variable
 *
 * \return status
 *   \retval FTL_READ_CAPACITY_SUCCESS        Success
 *   \retval FTL_READ_CAPACITY_FAILURE        Failure
 */
ftl_status_t ftl_read_capacity(uint32_t *nb_sectors);

/**
 * \brief FTL Test memory if write protect
 *
 * \return status
 *   \retval FTL_UNIT_WR_PROTECT           Write protect
 *   \retval FTL_UNIT_WR_NO_PROTECT        No protect
 */
ftl_status_t ftl_test_unit_wr_protect(void);

/**
 * \brief FTL Unprotect the memory
 *
 * \return status
 *   \retval FTL_UNIT_UNPROTECT_SUCCESS        Success
 *   \retval FTL_UNIT_UNPROTECT_FAILURE        Failure
 */
ftl_status_t ftl_unit_unprotect(void);

/**
 * \brief FTL Test if memory is removed
 *
 * \return status
 *   \retval FTL_UNIT_REMOVAL           Memory removed
 *   \retval FTL_UNIT_NO_REMOVAL        Memory not removed
 */
ftl_status_t ftl_test_unit_removal(void);

#ifdef __DEBUG__

/**
 * \brief FTL Read erase count in different blocks
 */
void wl_stat(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
