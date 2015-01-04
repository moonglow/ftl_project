/**
 * \file
 *
 * \brief Error control for FTL
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

#ifndef __FTL_STATUS_H_
#define __FTL_STATUS_H_

//! \brief Status used by FTL API
typedef enum {
	FTL_SUCCESS                         =0,
	FTL_FAILURE                         =1,
	FTL_ADD_FREE_BLOCK_FAILURE          =2,
	FTL_ADD_FREE_BLOCK_SUCCESS          =3,
	FTL_GET_FREE_BLOCK_FAILURE          =4,
	FTL_GET_FREE_BLOCK_SUCCESS          =5,
	FTL_HAL_INIT_SUCCESS                =6,
	FTL_HAL_INIT_FAILURE                =7,
	FTL_READ_PAGE_SUCCESS               =8,
	FTL_READ_PAGE_FAILURE               =9,
	FTL_WRITE_PAGE_SUCCESS              =10,
	FTL_WRITE_PAGE_FAILURE              =11,
	FTL_BLOCK_PAGE_MAP_EXIST            =12,
	FTL_BLOCK_PAGE_MAP_NOT_EXIST        =13,
	FTL_CREATE_PAGE_MAP_SUCCESS         =14,
	FTL_CREATE_PAGE_MAP_FAILURE         =15,
	FTL_UPDATE_PAGE_MAP_SUCCESS         =16,
	FTL_UPDATE_PAGE_MAP_FAILURE         =17,
	FTL_BLOCK_ERASE_SUCCESS             =18,
	FTL_BLOCK_ERASE_FAILURE             =19,
	FTL_GET_BLOCK_STATUS_SUCCESS        =20,
	FTL_GET_BLOCK_STATUS_FAILURE        =21,
	FTL_SET_BLOCK_STATUS_SUCCESS        =22,
	FTL_SET_BLOCK_STATUS_FAILURE        =23,
	FTL_GRABAGE_COLLECTION_SUCCESS      =24,
	FTL_GARBAGE_COLLECTION_FAILURE      =25,
	FTL_MARK_BAD_BLOCK_SUCCESS          =26,
	FTL_MARK_BAD_BLOCK_FAILURE          =27,
	FTL_GET_PHY_PAGE_SUCCESS            =28,
	FTL_GET_PHY_PAGE_FAILURE            =29,
	FTL_GET_CHIP_ID_SUCCESS             =30,
	FTL_GET_CHIP_ID_FAILURE             =31,
	FTL_CHIP_PARA_NO_EXIST              =32,
	FTL_INIT_MALLOC_FAILURE             =33,
	FTL_INIT_SUCCESS                    =34,
	FTL_INIT_FAILURE                    =35,
	FTL_COPY_PAGE_MAP_SUCCESS           =36,
	FTL_COPY_PAGE_MAP_FAILURE           =37,
	FTL_UINT_READY                      =38,
	FTL_UINT_NOT_READY                  =39,
	FTL_READ_CAPACITY_SUCCESS           =40,
	FTL_READ_CAPACITY_FAILURE           =41,
	FTL_UNIT_WR_PROTECT                 =42,
	FTL_UNIT_WR_NO_PROTECT              =43,
	FTL_UNIT_REMOVAL                    =44,
	FTL_UNIT_NO_REMOVAL                 =45,
	FTL_UNIT_UNPROTECT_SUCCESS          =46,
	FTL_UNIT_UNPROTECT_FAILURE          =47,
	FTL_GET_PHY_BLOCK_SUCCESS           =48,
	FTL_GET_PHY_BLOCK_FAILURE           =49,
	FTL_CONFLICT_RESOLVE_SUCCESS        =50,
	FTL_CONFLICT_RESOLVE_FAILURE        =51,
	FTL_DEFRAG_SUCCESS                  =52,
	FTL_DEFRAG_FAILURE                  =53

}ftl_status_t;

//! Status used for Uplayer API, FAT API need these status
typedef enum
{
	//! Success, memory ready.
	UPLAYER_SUCCESS                     =0,
	//! An error occurred.
	UPLAYER_FAILURE                     =1,
	//! Memory unplugged.
	UPLAYER_UNIT_NO_PRESENT             =2,
	//! Memory not initialized or changed.
	UPLAYER_UNIT_BUSY                   =3
} uplayer_status_t;

#endif
