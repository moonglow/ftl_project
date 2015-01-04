/**
 * \file
 *
 * \brief HAL(Hardware abstract layer)
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

#ifndef __HARDWARE_ABSTRACT_LAYER__H__
#define __HARDWARE_ABSTRACT_LAYER__H__

#include "ftl.h"

/**
 * \brief Memory block erase
 *
 * This function erase one block of memory, the block
 * size is 64Kbytes. 64Kbytes is used as an erase unit
 *
 * \param addr        The block physical address
 *
 * \return status
 *   \retval FTL_BLOCK_ERASE_SUCCESS    Success
 *   \retval FTL_BLOCK_ERASE_FAILURE    Failure
 */
ftl_status_t hal_block_erase(uint32_t addr);

/**
 * \brief Read memory ID
 *
 * This function read the memory ID, and store the ID in the buf
 *
 * \param buf        Pointer to buffer in ram
 *
 * \return status
 *   \retval FTL_GET_CHIP_ID_SUCCESS    Success
 *   \retval FTL_GET_CHIP_ID_FAILURE    Failure
 */
ftl_status_t hal_read_id(uint8_t *buf);

/**
 * \brief Write byte(s) to memory
 *
 * This function writes 1byte or several bytes to memory
 *
 * \param addr        The physical address to write the byte(s)
 * \param buf         Buf to store data write to memory
 * \parma count       Number of bytes to write
 *
 * \return status
 *   \retval FTL_SET_BLOCK_STATUS_SUCCESS    Success
 *   \retval FTL_SET_BLOCK_STATUS_FAILURE    Failure
 */
ftl_status_t hal_set_block_status(uint32_t addr, uint8_t *buf, uint16_t count);

/**
 * \brief Read byte(s) from memory
 *
 * This function read 1byte or several bytes from memory
 *
 * \param addr        The physical address to read the byte(s)
 * \param buf         Buf to store data read from memory
 * \parma count       Number of bytes to read
 *
 * \return status
 *   \retval FTL_GET_BLOCK_STATUS_SUCCESS    Success
 *   \retval FTL_GET_BLOCK_STATUS_FAILURE    Failure
 */
ftl_status_t hal_get_block_status(uint32_t addr, uint8_t *buf, uint16_t count);

/**
 * \brief Page write to memory
 *
 * This function write one page(512bytes) to memory.
 *
 * \param sector        The physical sector number
 * \param buf           Buf to store data write to memory
 *
 * \return status
 *   \retval FTL_WRITE_PAGE_SUCCESS    Success
 *   \retval FTL_WRITE_PAGE_FAILURE    Failure
 */
ftl_status_t hal_write_page(uint32_t sector, uint8_t *buf);

/**
 * \brief Page read from memory
 *
 * This function read one page(512bytes) from memory.
 *
 * \param sector        The physical sector number
 * \param buf           Buf to store data read from memory
 *
 * \return status
 *   \retval FTL_READ_PAGE_SUCCESS    Success
 *   \retval FTL_READ_PAGE_FAILURE    Failure
 */
ftl_status_t hal_read_page(uint32_t sector, uint8_t *buf);

/**
 * \brief Test memory if write protect
 *
 * \return status
 *   \retval FTL_UNIT_WR_PROTECT           Write protect
 *   \retval FTL_UNIT_WR_NO_PROTECT        No protect
 */
ftl_status_t hal_test_unit_wr_protect(void);

/**
 * \brief Unprotect the memory
 *
 * \return status
 *   \retval FTL_UNIT_UNPROTECT_SUCCESS        Success
 *   \retval FTL_UNIT_UNPROTECT_FAILURE        Failure
 */
ftl_status_t hal_unit_unprotect(void);

/**
 * \brief Test if memory is removed
 *
 * \return status
 *   \retval FTL_UNIT_REMOVAL           Memory removed
 *   \retval FTL_UNIT_NO_REMOVAL        Memory not removed
 */
ftl_status_t hal_test_unit_removal(void);

#endif
