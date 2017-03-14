/******************************************************************************
 @file nv_linux.h

 @brief TIMAC 2.0 API nv_linux.h NV implimentation for linux.

 Group: WCS LPC
 $Target Devices: Linux: AM335x, Embedded Devices: CC1310, CC1350$

 ******************************************************************************
 $License: BSD3 2016 $
  
   Copyright (c) 2015, Texas Instruments Incorporated
   All rights reserved.
  
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
  
   *  Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
  
   *  Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
  
   *  Neither the name of Texas Instruments Incorporated nor the names of
      its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
  
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 $Release Name: TI-15.4Stack Linux x64 SDK$
 $Release Date: July 14, 2016 (2.00.00.30)$
 *****************************************************************************/

#if !defined(NV_LINUX_H)
#define NV_LINUX_H

#include <stdint.h>
#include <stdbool.h>

/* forward decloration so that this file does not depend upon 'ini_file.h' */
struct ini_parser;

/* This is a linix specific CONFIG.H, in the embedded world this feature
 * is hard coded at compile time, in the Linux implimentation this feature
 * can be set via the configuration file. hence, the variable
 * linux_CONFIG_NV_RESTORE masquerades as a the compile time constant.
 */
extern bool linux_CONFIG_NV_RESTORE;
/*! Should the application restore state from the NV simulation file? */
#define CONFIG_NV_RESTORE linux_CONFIG_NV_RESTORE

/*! linux filename used for NV simulation, can be overridden by config file */
extern const char *NV_FILENAME;

/*! handle any ini file settings */
extern int NV_INI_settings(struct ini_parser *pINI, bool *handled);

/*! Debug log flag for basic NV activity */
#define LOG_DBG_NV_dbg  _bitN(LOG_DBG_NV_bitnum_first+0)

/*! Debug log flag for super verbose NV activity */
#define LOG_DBG_NV_rdwr _bitN(LOG_DBG_NV_bitnum_first+1)


/*! save local copy to disc */
extern void NV_LINUX_saveNvFile(void);

/*! Set the NV callback function pointers */
extern void NVOCTP_loadApiPtrs(NVINTF_nvFuncts_t *pfn);

/******************************************************************************
 Constants and definitions
******************************************************************************/

/* NV driver item ID definitions */
#define NVOCTP_NVID_DIAG {NVINTF_SYSID_NVDRVR, 1, 0}

/******************************************************************************
 Typedefs
******************************************************************************/

/* NV driver diagnostic data */
typedef struct
{
    uint32_t compacts;  /*!< Number of page compactions */
    uint16_t resets;    /*!< Number of driver resets (power on) */
    uint16_t available; /*!< Number of available bytes after last compaction */
    uint16_t active;    /*!< Number of active items after last compaction */
    uint16_t reserved;  /*!< Number of reserved items after last compaction */
}
NVOCTP_diag_t;

/*****************************************************************************
 Functions
******************************************************************************/

/*!
 * @fn      NVOCTP_loadApiPtrs
 *
 * @brief   Global function to return function pointers for NV driver API that
 *          are supported by this module, NULL for functions not supported.
 *
 * @param   pfn - pointer to caller's structure of NV function pointers
 *
 * @return  none
 */
extern void NVOCTP_loadApiPtrs(NVINTF_nvFuncts_t *pfnNV);

/* Exception function can be defined to handle NV corruption issues */
/* If none provided, NV module attempts to proceeed ignoring problem */

#if !defined (NVOCTP_EXCEPTION)
#define NVOCTP_EXCEPTION(pg, err)
#endif

/*!
 * @brief Save the NV simulation file to disk
 */
void NV_LINUX_save(void);

/*
 * @brief Load the NV simulation file from disk
 */
void NV_LINUX_load(void);

/*
 * @brief Process NV items from the configuration file
 * @returns 0 success
 *
 * this is used as one of the callbacks for the INI_read() function.
 */

extern int NV_LINUX_INI_settings(struct ini_parser *pINI, bool *handled);

/*
 * @brief Initialize the NV module as a whole.
 */
void NV_LINUX_init(void);

#endif

/*
 *  ========================================
 *  Texas Instruments Micro Controller Style
 *  ========================================
 *  Local Variables:
 *  mode: c
 *  c-file-style: "bsd"
 *  tab-width: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  End:
 *  vim:set  filetype=c tabstop=4 shiftwidth=4 expandtab=true
 */

