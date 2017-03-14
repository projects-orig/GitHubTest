/******************************************************************************
 @file nv_linux.c

 @brief TIMAC 2.0 API Linux version of NV module

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

/******************************************************************************
 Design Overview
 *****************************************************************************

This driver implements a non-volatile (NV) memory system that utilizes 2 pages
(consecutive) of on-chip Flash memory. After initialization, one page is ACTIVE
and the other page is available for "compaction" when the ACTIVE page does not
have enough empty space for data write operation. Compaction can occur 'just in
time' during a data write operation or 'on demand' by application request. The
compaction process is designed to survive a power cycle before it completes. It
will resume where it was interrupted and complete the process.

This driver makes the following assumptions and uses them to optimize the code
and data storage design: (1) Flash memory is addressable at individual, 1-byte
resolution so no padding or word-alignment is necessary (2) an individual byte
can be modified up to 8 times if each write changes only one bit from 1 to 0.

Each Flash page has a "page header" which indicates its current state (ERASED,
ACTIVE, or XFER), located at the first byte of the Flash page. The remainder of
the Flash page contains NV data items which are packed together following the
page header. Each NV data item has two parts, (1) a data block which is stored
first (lower memory address), (2) immediately followed by item header (higher
memory address). The item header contains information necessary to traverse the
packed data items, as well as, current status of each data item. Obsolete items
marked accordingly but a search for the newest instance of an item is sped up
by starting the search at the last entry in the page (higher memory address).
*/

/******************************************************************************
 Includes
*****************************************************************************/

#include "nvintf.h"
#include "nv_linux.h"

#include "stream.h"
#include "log.h"
#include "mutex.h"
#include "fatal.h"
#include "ini_file.h"
#include "bitsnbits.h"

#include <string.h>

#include <malloc.h>    /* for calloc */

bool linux_CONFIG_NV_RESTORE;
static const char _nv_default_filename[] = "nv-simulation.bin";
static const char *NV_filename = _nv_default_filename;
static uint8_t    *NV_ramSim;
static unsigned    NV_ramLength;

const struct ini_flag_name nv_log_flags[] = {
    { .name = "nv-debug" , .value = LOG_DBG_NV_dbg  },
    { .name = "nv-rdwr"  , .value = LOG_DBG_NV_rdwr },

    /* terminate */
    { .name = NULL }
};

/******************************************************************************
 Constants and definitions
*****************************************************************************/

#define FAPI_STATUS_SUCCESS true

/* Optional feature compile flags - disable to reduce code size */
#if !defined (NVOCTP_MINIMAL)
#define NVOCTP_CHECKPARAMS
#define NVOCTP_DIAGNOSTICS
#define NVOCTP_POWERMONITOR
#endif

/* Maximum ID parameters - must be coordinated with header compression,
   Increasing these limits requires modification of the readHdr() function */
#define NVOCTP_MAXSYSID   0x003F  /*  6 bits */
#define NVOCTP_MAXITEMID  0x03FF  /* 10 bits */
#define NVOCTP_MAXSUBID   0x03FF  /* 10 bits */
#define NVOCTP_MAXLEN     0x03FF  /* 10 bits */

/* Contents of an erased Flash memory locations */
#define NVOCTP_ERASEDBYTE  0xFF
#define NVOCTP_ERASEDWORD  0xFFFFFFFF

/* Invalid NV page - if 0 is ever used, change this definition */
#define NVOCTP_NULLPAGE  (nvEndPage + 1)

/* Invalid NV item - if 0 is ever used, change this definition */
#define NVOCTP_NULLITEM  0

/* Invalid compressed NV item ID (see NVOCTP_CMPRID() macro) */
#define NVOCTP_INVCMPID  0xFFFFFFFF

/* Data change values */
#define NVOCTP_NOCHANGE   0
#define NVOCTP_ONETOZERO  1
#define NVOCTP_ZEROTOONE  2
#define NVOCTP_NEWLENGTH  3

/* Block size for Flash-Flash XFER */
#define NVOCTP_XFERBLKMAX  8

/* NV item ID for driver diagnostics */
static const NVINTF_itemID_t diagId = NVOCTP_NVID_DIAG;

/******************************************************************************
 Macros
******************************************************************************/

/*****************************************************************************
 Page and Header Definitions
******************************************************************************/

#define EMBEDDED_CONST  /* rd/wr variable on linux & windows */

#if !defined (SNV_FIRST_PAGE)
#define SNV_FIRST_PAGE  0x00
#endif
static EMBEDDED_CONST uint32_t nvBegPage = SNV_FIRST_PAGE;
static EMBEDDED_CONST uint32_t nvEndPage = (SNV_FIRST_PAGE + 1);

#if !defined (FLASH_PAGE_SIZE)
// Common page size for all TARGET devices
#define FLASH_PAGE_SIZE  0x1000
#endif
static EMBEDDED_CONST uint32_t nvPageSize = FLASH_PAGE_SIZE;

#if !defined (NVOCTP_VERSION)
/* Version of NV page format (do not use 0xFF) */
#define NVOCTP_VERSION  0x01
#endif
static EMBEDDED_CONST uint8_t nvVersion = NVOCTP_VERSION;

#if !defined (NVOCTP_SIGNATURE)
/* Page header validation byte (do not use 0xFF) */
#define NVOCTP_SIGNATURE  0x96
#endif
static EMBEDDED_CONST uint8_t nvSignature = NVOCTP_SIGNATURE;

/* Page header structure */
typedef struct
{
    uint8_t state;      /* PGCLEAR, PGERASED, PGACTIVE, PGXFER */
    uint8_t cycle;      /* Rolling page compaction count (0x00, 0xFF not used) */
    uint8_t version;    /* Version of NV page format */
    uint8_t signature;  /* Signature for formatted NV page */
} NVOCTP_pageHdr_t;

/* Page header size (bytes) */
#define NVOCTP_PGHDRLEN  (sizeof(NVOCTP_pageHdr_t))

/* Page header offsets (from 1st byte of page) */
#define NVOCTP_PGHDROFS  0
#define NVOCTP_PGHDRPST  0  /* Page state */
#define NVOCTP_PGHDRCYC  1  /* Cycle count */
#define NVOCTP_PGHDRVER  2  /* Format version */
#define NVOCTP_PGHDRSIG  3  /* Page signature */

/* Page data size, offset into page */
#define NVOCTP_PGDATAOFS  (NVOCTP_PGHDRLEN)
#define NVOCTP_PGDATAEND  ((uint16_t)(nvPageSize - 1))
#define NVOCTP_PGDATALEN  ((uint16_t)(nvPageSize - NVOCTP_PGHDRLEN))

/* Page header state values - transitions change 1 bit in each nybble */
#define NVOCTP_PGCLEAR   0xFF  /* Erase started, not verified */
#define NVOCTP_PGERASED  0xB7  /* Erase verified, page ready for use */
#define NVOCTP_PGACTIVE  0xA5  /* Current active page */
#define NVOCTP_PGXFER    0x24  /* Active page being compacted */

/* Page compaction cycle count limits (0x00 and 0xFF not used) */
#define NVOCTP_MINCYCLE  0x01  /* Minimum cycle count (after rollover) */
#define NVOCTP_MAXCYCLE  0xFE  /* Maximum cycle count (before rollover) */

/******************************************************************************
 Item Header Definitions
*******************************************************************************/

/* Item header structure */
typedef struct
{
    uint32_t cmpid; /* Compressed ID */
    uint16_t subid; /* Sub ID */
    uint16_t itmid; /* Item ID */
    uint8_t  sysid; /* System ID */
    uint8_t  stats; /* Status 'marks' */
    uint16_t hofs;  /* Header offset */
    uint16_t len;   /* Data length */
} NVOCTP_itemHdr_t;

/* Length (bytes) of compressed header */
#define NVOCTP_ITEMHDRLEN  5

/* Compressed item header information
 * Byte: 44444444 33333333 22222222 11111111 00000000
 * Item: AVSSSSSS IIIIIIII IISSSSSS SSSSLLLL LLLLLLVA
 *
 * Bit(s)  Bit Field Description
 * =============================
 *    39:  active id mark (1=active)
 *    38:  valid id mark (0=valid)
 * 32-37:  system id (0-63)
 * 22-31:  nv item id (0-1023)
 * 12-21:  item sub id (0-1023)
 * 02-11:  data length (0-1023)
 *    01:  valid length mark (0=valid)
 *    00:  active length mark (0=active)
 */

/* Bit39 in compressed header - '1' indicates active NV item */
#define NVOCTP_ACTIVEIDBIT   0x80
/* Bit38 in compressed header - '0' indicates valid NV item */
#define NVOCTP_VALIDIDBIT    0x40
/* Bit01 in compressed header - '0' indicates valid NV length */
#define NVOCTP_VALIDLENBIT   0x02
/* Bit00 in compressed header - '0' indicates active NV header */
#define NVOCTP_ACTIVEHDRBIT  0x01

/* Index of last item header byte */
#define NVOCTP_ITEMHDREND (NVOCTP_ITEMHDRLEN - 1)

/* Compressed item header byte array */
typedef uint8_t cmpIH_t[NVOCTP_ITEMHDRLEN];

/* Item write parameters */
typedef struct
{
    NVOCTP_itemHdr_t *iHdr; /* Ptr to item header */
    uint16_t          dOfs; /* Source data offset */
    uint16_t          bOfs; /* Buffer data offset */
    uint16_t          bLen; /* Buffer data length */
    uint8_t          *pBuf; /* Ptr to data buffer */
} NVOCTP_itemWrp_t;

/******************************************************************************
 Local variables
*******************************************************************************/

/* Flash Pages */

/* Active page */
static uint32_t activePg;

/* Active page offset to next available location to write an item */
static uint32_t pgOff;

/* Active page compaction cycle count. Used to select the 'newest' active page */
/* at device reset, in the very unlikely scenario that both pages are active. */
static uint32_t pgCycle;

/* Flag to indicate that a fatal error occurred while writing to or erasing the */
/* Flash memory. If flag is set, it's unsafe to attempt another write or erase. */
/* This flag locks writes to Flash until the next system reset. */
static uint8_t failF = NVINTF_NOTREADY;

/* Flag to indicate that a non-fatal error occurred while writing to or erasing */
/* Flash memory. If flag is set, it's safe to attempt another write or erase. */
/* This flag is reset by any API calls that cause an erase/write to Flash. */
static uint8_t failW = 0;

static intptr_t nvMutex;

/******************************************************************************
 Local Function Prototypes
*******************************************************************************/

/* NV API driver functions */

static uint8_t NVOCTP_initNvApi(void *param);

/* NV API data item functions */
static uint8_t NVOCTP_createItemApi(NVINTF_itemID_t id,
                                    uint32_t len,
                                    void *buf);

static uint8_t NVOCTP_deleteItemApi(NVINTF_itemID_t id);

static uint32_t NVOCTP_getItemLenApi(NVINTF_itemID_t id);

static uint8_t NVOCTP_readItemApi(NVINTF_itemID_t id,
                                  uint16_t ofs,
                                  uint16_t len,
                                  void *buf);

static uint8_t NVOCTP_writeItemApi(NVINTF_itemID_t id,
                                   uint16_t len,
                                   void *buf);

static uint8_t NVOCTP_writeItemExApi(NVINTF_itemID_t id,
                                     uint16_t ofs,
                                     uint16_t len,
                                     void *buf);

/* Flash I/O utility functions */
static void NVOCTP_disableCache(uint32_t *mode);

static void NVOCTP_flashRead(uint32_t pg,
                             uint32_t ofs,
                             uint32_t num,
                             uint8_t *buf);

static void NVOCTP_flashWrite(uint32_t pg,
                              uint32_t ofs,
                              uint32_t num,
                              uint8_t *buf);

static uint8_t NVOCTP_readByte(uint32_t pg,
                               uint32_t ofs);

static void NVOCTP_restoreCache(uint32_t mode);

static void NVOCTP_writeByte(uint32_t pg,
                             uint32_t ofs,
                             uint8_t bwv);

/* NV driver utility functions */
static uint8_t NVOCTP_checkItem(NVINTF_itemID_t id,
                                uint32_t len,
                                NVOCTP_itemHdr_t *iHdr);

static int32_t NVOCTP_compactPage(uint32_t srcPg,
                                  NVOCTP_itemWrp_t *iWrp);

static void NVOCTP_copyItem(uint32_t xPg,
                            uint32_t sOfs,
                            uint32_t dOfs,
                            uint32_t len);

static uint32_t NVOCTP_dataChange(uint32_t ofs,
                                  uint32_t len,
                                  uint8_t *pBuf);

static void NVOCTP_erasePage(uint32_t pg);

static int32_t NVOCTP_findItem(uint32_t pg,
                               uint32_t ofs,
                               uint32_t cid);

static uint32_t NVOCTP_findOffset(uint32_t pg,
                                  uint32_t bOfs);

static bool NVOCTP_isErased(uint32_t pg,
                            uint32_t ofs,
                            uint32_t num);

static uint8_t NVOCTP_newItem(NVOCTP_itemHdr_t *iHdr,
                              uint8_t *pBuf);

static void NVOCTP_readHeader(uint32_t pg,
                              uint32_t ofs,
                              NVOCTP_itemHdr_t *iHdr);

static void NVOCTP_setItemInactive(uint32_t hOfs);

static void NVOCTP_setPageActive(uint32_t pg);

static uint8_t NVOCTP_updateItem(NVOCTP_itemHdr_t *iHdr,
                                 uint32_t bOfs,
                                 uint32_t bLen,
                                 void *pBuf);

static void NVOCTP_writeItem(NVOCTP_itemHdr_t *iHdr,
                             uint32_t dstPg,
                             uint32_t sOfs,
                             uint32_t bOfs,
                             uint32_t bLen,
                             uint8_t *pBuf);

static void NVOCTP_writeHeader(uint32_t pg,
                               uint32_t ofs,
                               uint8_t *cHdr);

/******************************************************************************
 API Functions - NV driver
*******************************************************************************/

/*!
 * @brief simulate embedded macro to calculate flash byte location
 * @param pg - page
 * @param ofs - offset into page
 * @returns pointer to the specific byte
 */
static uint8_t *NVOCTP_FLASHADDR(uint32_t pg, uint32_t ofs)
{
    uint8_t *p;
    uint32_t x;

    x = (((pg) * FLASH_PAGE_SIZE) + ofs);
    p = NV_ramSim;
    p = p + x;
    return (p);
}

/*!
 * @brief Simulate embeded macro whether TARGET voltage sufficent to erase/wr NV
 */
static bool NVOCTP_FLASHACCESS(void)
{
    return (true);
}

/*!
 * @brief Simulate embedded macro to Lock driver access via mutex
 */
static void NVOCTP_LOCK(void)
{
    MUTEX_lock(nvMutex, -1);
}

/*!
 * @brief Simulate embedded macro to Lock driver access via mutex
 */
static void NVOCTP_UNLOCK(void)
{
    MUTEX_unLock(nvMutex);
}

/*!
 *@brief Simulate embedded macro to generate a compressed NV ID
 * (NOTE: bit31 must be zero)
 */
static uint32_t NVOCTP_CMPRID(uint32_t s, uint32_t i, uint32_t b)
{
    return ((uint32_t)((((((s) & NVOCTP_MAXSYSID) << 12)  |
                         ((i) & NVOCTP_MAXITEMID)) << 12) |
                       ((b) & NVOCTP_MAXSUBID)));
}

/*!
 * @fn      NVOCTP_compactNvApi
 *
 * @brief   API function to force NV active page compaction
 *
 * @param   minAvail - threshold size of available bytes on Flash page to do
 *                     compaction: 0 = always, >0 = minimum remaining bytes
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_compactNvApi(uint16_t minAvail)
{
    uint8_t err;

    /* Prevent RTOS thread contention */
    NVOCTP_LOCK();

    err = failF;
    if(err == NVINTF_SUCCESS)
    {
        int32_t left;

        /* Number of bytes left on active page */
        left = (nvPageSize - pgOff);

        /* Time to do a compaction? */
        if((left < minAvail) || (minAvail == 0))
        {
            /* Transfer all items to non-ACTIVE page */
            (void)NVOCTP_compactPage(activePg, NULL);
            /* 'failW' indicates compaction status */
            err = failW;
        }
        else
        {
            /* Indicate "bad" minAvail value */
            err = NVINTF_BADPARAM;
        }
    }
    if(err == NVINTF_SUCCESS)
    {
        NV_LINUX_save();
    }
    NVOCTP_UNLOCK();

    return (err);
}

/******************************************************************************
 API Functions - NV Data Items
******************************************************************************/

/*!
 * @fn      NVOCTP_createItemApi
 *
 * @brief   API function to create a new NV item in Flash memory
 *
 * @param   id - NV item type identifier
 * @param   len - length of NV data block
 * @param   buf - pointer to caller's initialization data buffer
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_createItemApi(NVINTF_itemID_t id,
                                    uint32_t bLen,
                                    void *pBuf)
{
    uint8_t err;
    NVOCTP_itemHdr_t iHdr;

    /* Prevent RTOS thread contention */
    NVOCTP_LOCK();

    err = NVOCTP_checkItem(id, bLen, &iHdr);
    if(err == NVINTF_NOTFOUND)
    {
        /* Create the new item */
        err = NVOCTP_newItem(&iHdr, pBuf);
    }
    else
    {
        if(err == NVINTF_SUCCESS)
        {
            /* Item already exists */
            err = NVINTF_FAILURE;
        }
    }

    if(err == NVINTF_SUCCESS)
    {
        NV_LINUX_save();
    }

    NVOCTP_UNLOCK();

    return (err);
}

/*!
 * @fn      NVOCTP_deleteItemApi
 *
 * @brief   API function to delete an existing NV item from Flash memory
 *
 * @param   id - NV item type identifier
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_deleteItemApi(NVINTF_itemID_t id)
{
    uint8_t err;
    NVOCTP_itemHdr_t iHdr;

#if defined (NVOCTP_DIAGNOSTICS)
    if(id.systemID == NVINTF_SYSID_NVDRVR)
    {
        /* Protect NV driver item(s) */
        return (NVINTF_BADSYSID);
    }
#endif

    /* Prevent RTOS thread contention */
    NVOCTP_LOCK();

    err = NVOCTP_checkItem(id, 0, &iHdr);
    if(err == NVINTF_SUCCESS)
    {
        int32_t hOfs;

        /* Mark this item as inactive */
        NVOCTP_setItemInactive(iHdr.hofs);

        /* Verify that item has been removed */
        hOfs = NVOCTP_findItem(activePg, pgOff, iHdr.cmpid);

        /* If item did get deleted, report 'failW' status */
        err = (hOfs <= 0) ? failW : NVINTF_CORRUPT;
    }

    if(err == NVINTF_SUCCESS)
    {
        NV_LINUX_save();
    }

    NVOCTP_UNLOCK();

    return (err);
}

/*!
 * @fn      NVOCTP_getItemLenApi
 *
 * @brief   API function to return the length of an NV data item
 *
 * @param   id - NV item type identifier
 *
 * @return  NV item length or 0 if item not found
 */
static uint32_t NVOCTP_getItemLenApi(NVINTF_itemID_t id)
{
    uint8_t err;
    uint32_t len;
    NVOCTP_itemHdr_t iHdr;

    /* Prevent RTOS thread contention */
    NVOCTP_LOCK();

    err = NVOCTP_checkItem(id, 0, &iHdr);

    /* If there was any error, report zero length */
    len = (err != NVINTF_SUCCESS) ? 0 : iHdr.len;

    NVOCTP_UNLOCK();

    return (len);
}

/*!
 * @fn      NVOCTP_readItemApi
 *
 * @brief   API function to read data from an NV item
 *
 * @param   id   - NV item type identifier
 * @param   bOfs - offset into NV data block
 * @param   bLen - length of NV data to return
 * @param   pBuf - pointer to caller's read data buffer
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_readItemApi(NVINTF_itemID_t id,
                                  uint16_t bOfs,
                                  uint16_t bLen,
                                  void *pBuf)
{
    uint8_t err;
    uint32_t dOfs;
    NVOCTP_itemHdr_t iHdr;

    /* Prevent RTOS thread contention */
    NVOCTP_LOCK();

    err = NVOCTP_checkItem(id, bLen, &iHdr);
    if(err == NVINTF_SUCCESS)
    {
        /* Offset to start of item data */
        dOfs = (iHdr.hofs - iHdr.len) + bOfs;
        if((dOfs + bLen) <= iHdr.hofs)
        {
            /* Copy NV data block to caller's buffer */
            NVOCTP_flashRead(activePg, dOfs, bLen, (uint8_t *)pBuf);
        }
        else
        {
            /* Bad length or offset */
            err = (bLen > iHdr.len) ? NVINTF_BADLENGTH : NVINTF_BADOFFSET;
        }
    }

    NVOCTP_UNLOCK();

    return (err);
}

/*!
 * @fn      NVOCTP_writeItemApi
 *
 * @brief   API function to write data NV item, create if not already existing
 *
 * @param   id   - NV item type identifier
 * @param   bLen - data buffer length to write into NV block
 * @param   pBuf - pointer to caller's data buffer to write
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_writeItemApi(NVINTF_itemID_t id,
                                   uint16_t bLen,
                                   void *pBuf)
{
    uint8_t err;
    uint16_t oOfs;
    NVOCTP_itemHdr_t iHdr;

    /* Prevent RTOS thread contention */
    NVOCTP_LOCK();

    /* Assume updating existing item */
    oOfs = 0;

    err = NVOCTP_checkItem(id, bLen, &iHdr);
    if(err == NVINTF_SUCCESS)
    {
        /* If item length not changing, */
        if(iHdr.len == bLen)
        {
            /* Update the existing item */
            err = NVOCTP_updateItem(&iHdr, 0, bLen, pBuf);
        }
        else
        {
            /* Length of new item */
            iHdr.len = (uint16_t)bLen;
            /* Offset of old item */
            oOfs = iHdr.hofs;
            /* Force new item creation */
            err = NVINTF_NOTFOUND;
        }
    }

    /* If item not found or was deleted, */
    if(err == NVINTF_NOTFOUND)
    {
        /* Create a new item */
        err = NVOCTP_newItem(&iHdr, pBuf);
        if(oOfs != 0)
        {
            /* Mark old item as inactive */
            NVOCTP_setItemInactive(oOfs);
            err = failW;
        }
    }

    if(err == NVINTF_SUCCESS)
    {
        NV_LINUX_save();
    }

    NVOCTP_UNLOCK();

    return (err);
}

/*!
 * @fn      NVOCTP_writeItemExApi
 *
 * @brief   API function to write data to an existing NV item
 *
 * @param   id   - NV item type identifier
 * @param   bOfs - data offset into Flash NV block
 * @param   bLen - data buffer length to write into NV block
 * @param   pBuf - pointer to caller's data buffer to write
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_writeItemExApi(NVINTF_itemID_t id,
                                     uint16_t bOfs,
                                     uint16_t bLen,
                                     void *pBuf)
{
    uint8_t err;
    NVOCTP_itemHdr_t iHdr;

    /* Prevent RTOS thread contention */
    NVOCTP_LOCK();

    err = NVOCTP_checkItem(id, bLen, &iHdr);
    if(err == NVINTF_SUCCESS)
    {
        uint32_t dOfs;

        /* Offset to start of item data */
        dOfs = (iHdr.hofs - iHdr.len) + bOfs;
        if((dOfs + bLen) <= iHdr.hofs)
        {
            /* Copy caller's data to NV data block */
            err = NVOCTP_updateItem(&iHdr, bOfs, bLen, pBuf);
        }
        else
        {
            /* Bad offset or length */
            err = NVINTF_BADOFFSET;
        }
    }

    if(err == NVINTF_SUCCESS)
    {
        NV_LINUX_save();
    }

    NVOCTP_UNLOCK();

    return (err);
}

/******************************************************************************
 Local NV Driver Utility Functions
******************************************************************************/

/*
 * @fn      NVOCTP_checkItem
 *
 * @brief   Local function to check parameters and locate existing item
 *
 * @param   id   - NV item type identifier
 * @param   bLen - NV item data length
 * @param   pHdr - pointer to header buffer
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_checkItem(NVINTF_itemID_t id,
                                uint32_t bLen,
                                NVOCTP_itemHdr_t *pHdr)
{
    int32_t ofs;
    uint32_t cid;

#if defined (NVOCTP_CHECKPARAMS)
    if(bLen > NVOCTP_MAXLEN)
    {
        /* Item data is too long */
        return (NVINTF_BADLENGTH);
    }
    if(id.systemID > NVOCTP_MAXSYSID)
    {
        /* Too large for compressed header */
        return (NVINTF_BADSYSID);
    }
    if(id.itemID > NVOCTP_MAXITEMID)
    {
        /* Too large for compressed header */
        return (NVINTF_BADITEMID);
    }
    if(id.subID > NVOCTP_MAXSUBID)
    {
        /* Too large for compressed header */
        return (NVINTF_BADSUBID);
    }
#endif

    if(failF == NVINTF_NOTREADY)
    {
        /* NV driver has not been initialized */
        return (NVINTF_NOTREADY);
    }

    /* Reset erase/write fail indicator for current transaction */
    failW = NVINTF_SUCCESS;

    cid = NVOCTP_CMPRID(id.systemID, id.itemID, id.subID);

    ofs = NVOCTP_findItem(activePg, pgOff, cid);
    if(ofs <= 0)
    {
        /* Item does not exist yet */
        pHdr->len = (uint16_t)bLen;
        pHdr->hofs = 0;
        pHdr->cmpid = cid;
        pHdr->subid = id.subID;
        pHdr->itmid = id.itemID;
        pHdr->sysid = id.systemID;
        return (NVINTF_NOTFOUND);
    }

    /* Read and decompress item header */
    NVOCTP_readHeader(activePg, (uint16_t)ofs, pHdr);

    return (failW);
}

/*!
 * @fn      NVOCTP_newItem
 *
 * @brief   Local function to check for adequate space and create a new item
 *
 * @param   iHdr - pointer to header buffer
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_newItem(NVOCTP_itemHdr_t *iHdr,
                              uint8_t *pBuf)
{
    uint8_t err;
    uint16_t iLen;

    iLen = NVOCTP_ITEMHDRLEN + iHdr->len;
    if((pgOff + iLen) > nvPageSize)
    {
        /* Won't fit on the active page, compact and check again */
        if(NVOCTP_compactPage(activePg, NULL) < iLen)
        {
            /* Failure means there's no place to put this item */
            err = (failW != NVINTF_SUCCESS) ? failW : NVINTF_BADLENGTH;
            return (err);
        }
    }

    /* Create the new NV item */
    NVOCTP_writeItem(iHdr, activePg, 0, 0, iHdr->len, pBuf);

    /* Status of writing/erasing Flash */
    return (failW);
}

/*!
 * @fn      NVOCTP_updateItem
 *
 * @brief   Local function to determine update type and write item to NV
 *
 * @param   iHdr - pointer to item header
 * @param   bOfs - data offset into existing item data
 * @param   bLen - data buffer length to write into NV block
 * @param   pBuf - pointer to caller's data buffer to write
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_updateItem(NVOCTP_itemHdr_t *iHdr,
                                 uint32_t bOfs,
                                 uint32_t bLen,
                                 void *pBuf)
{
    uint32_t chg;
    uint32_t dOfs;
    uint32_t hOfs;

    /* Offset to start of original item data */
    hOfs = iHdr->hofs;
    dOfs = hOfs - iHdr->len;

    /* Check for change(s) between original and incoming data */
    chg = NVOCTP_dataChange(dOfs + bOfs, bLen, (uint8_t *)pBuf);

    if(chg == NVOCTP_NOCHANGE)
    {
        /* No change, nothing to write */
        return (NVINTF_SUCCESS);
    }
    if(chg == NVOCTP_ONETOZERO)
    {
        /* Only bit changes from 1 to 0, update original "in place" */
        NVOCTP_flashWrite(activePg, dOfs + bOfs, bLen, (uint8_t *)pBuf);
    }
    else
    {
        uint16_t iLen;

        iLen = NVOCTP_ITEMHDRLEN + iHdr->len;
        if((pgOff + iLen) <= nvPageSize)
        {
            /* Will fit on active page, copy and update the item */
            NVOCTP_writeItem(iHdr, activePg, dOfs, bOfs, bLen, (uint8_t *)pBuf);
            /* Mark this item as INACTIVE */
            NVOCTP_setItemInactive(hOfs);
        }
        else
        {
            NVOCTP_itemWrp_t iWrp;

            /* Updating the item won't fit on active page, */
            /* Gather item parameters so compactor can do the write */
            iWrp.iHdr = iHdr;
            iWrp.dOfs = (uint16_t)dOfs;
            iWrp.bOfs = (uint16_t)bOfs;
            iWrp.bLen = (uint16_t)bLen;
            iWrp.pBuf = pBuf;
            /* Compact NV and then write the entire updated item */
            (void)NVOCTP_compactPage(activePg, &iWrp);
        }
    }

    return (failW);
}

/*!
 * @fn      NVOCTP_writeItem
 *
 * @brief   Write entire NV item to new location on active Flash page.
 *          Each call to flashWrite() does a read-back to verify. If an
 *          error is detected, the 'failW' flag is set to inhibit further
 *          flash write attempts until the next NV transaction.
 *
 * @param   pHdr  - Pointer to caller's item header buffer
 * @param   dtsPg - Destination NV Flash page
 * @param   sOfs  - Offset into the source NV Flash page
 * @param   dOfs  - Data offset into the destination NV Flash page
 * @param   bLen  - Length of data in caller's buffer to write
 * @param   pBuf  - Pointer to caller's buffer to write & verify
 *
 * @return  none
 */
static void NVOCTP_writeItem(NVOCTP_itemHdr_t *pHdr,
                             uint32_t dstPg,
                             uint32_t sOfs,
                             uint32_t dOfs,
                             uint32_t bLen,
                             uint8_t *pBuf)
{
    uint32_t iLen;

    /* Total length of this item */
    iLen = NVOCTP_ITEMHDRLEN + pHdr->len;

    if((pgOff + iLen) <= nvPageSize)
    {
        cmpIH_t cHdr;
        uint32_t hOfs;

        /* Compressed item header information */
        /* Byte: 44444444 33333333 22222222 11111111 00000000 */
        /* Item: AVSSSSSS IIIIIIII IISSSSSS SSSSLLLL LLLLLLVA */
        cHdr[0] = ((pHdr->sysid) & 0x3F);
        cHdr[1] = ((pHdr->itmid >> 2) & 0xFF);
        cHdr[2] = ((pHdr->subid >> 4) & 0x3F) | ((pHdr->itmid & 0x03) << 6);
        cHdr[3] = ((pHdr->len >> 6) & 0x0F) | ((pHdr->subid & 0x0F) << 4);
        cHdr[4] = ((pHdr->len & 0x3F) << 2);

        /* Header is located after the item data */
        hOfs = pgOff + pHdr->len;

        /* Clear all 'valid' bits, write the header, set length 'valid' */
        NVOCTP_writeHeader(dstPg, hOfs, (uint8_t *)cHdr);

        /* Write/merge the data block */
        if(pBuf != NULL)
        {
            uint32_t len = dOfs;
            uint32_t my_dOfs = pgOff;

            if(len > 0)
            {
                /* Copy existing 'before' data block */
                NVOCTP_copyItem(dstPg, sOfs, my_dOfs, len);
                my_dOfs += len;
                sOfs += len;
            }

            /* Merge caller's data block */
            NVOCTP_flashWrite(dstPg, my_dOfs, bLen, (uint8_t *)pBuf);
            my_dOfs += bLen;

            len = pHdr->len - (my_dOfs - pgOff);
            if(len > 0)
            {
                /* Copy existing 'after' data block */
                sOfs += bLen;
                NVOCTP_copyItem(dstPg, sOfs, my_dOfs, len);
            }
        }

        /* Mark the IDs as VALID */
        cHdr[0] &= ~NVOCTP_VALIDIDBIT;
        NVOCTP_writeByte(dstPg, hOfs, cHdr[0]);

        if(NVOCTP_readByte(dstPg, hOfs + NVOCTP_ITEMHDREND) != NVOCTP_ERASEDBYTE)
        {
            /* Advance offset to next available location */
            pgOff += iLen;
        }
    }
    else
    {
        /* Not enough room on page */
        failW = NVINTF_BADLENGTH;
    }
}

/*!
 * @fn      NVOCTP_writeHeader
 *
 * @brief   Mark header IDs/len invalid, write header, mark length valid
 *
 * @param   pg   - A valid NV Flash page
 * @param   ofs  - A valid header offset into the page
 * @param   cHdr - Pointer to caller's compressed header
 *
 * @return  none
 */
static void NVOCTP_writeHeader(uint32_t pg,
                               uint32_t ofs,
                               uint8_t *cHdr)
{
    int8_t i;

    /* Mark the IDs and length invalid */
    cHdr[0] |= (NVOCTP_VALIDIDBIT | NVOCTP_ACTIVEIDBIT);
    cHdr[NVOCTP_ITEMHDREND] |= NVOCTP_VALIDLENBIT;

    /* Write the header(with INVALID marks) */
    for(i = NVOCTP_ITEMHDREND; i >= 0; i--)
    {
        /* One byte at a time, starting with last one */
        NVOCTP_writeByte(pg, ofs + i, cHdr[i]);
    }

    /* Remove the length INVALID mark */
    cHdr[NVOCTP_ITEMHDREND] &= ~NVOCTP_VALIDLENBIT;
    NVOCTP_writeByte(pg, ofs + NVOCTP_ITEMHDREND, cHdr[NVOCTP_ITEMHDREND]);
}

/*!
 * @fn      NVOCTP_readHeader
 *
 * @brief   Read header block from NV and expand into caller's buffer
 *
 * @param   pg   - A valid NV Flash page
 * @param   ofs  - A valid offset into the page
 * @param   pHdr - Pointer to caller's item header buffer
 *
 * @return  none
 */
static void NVOCTP_readHeader(uint32_t pg,
                              uint32_t ofs,
                              NVOCTP_itemHdr_t *pHdr)
{
    cmpIH_t cHdr;

    /* Get item header from Flash */
    NVOCTP_flashRead(pg, ofs, NVOCTP_ITEMHDRLEN, (uint8_t *)cHdr);

    /* Offset to compressed header */
    pHdr->hofs = (uint16_t)ofs;

    /* Uncompress the header */
    /* Byte: 44444444 33333333 22222222 11111111 00000000 */
    /* Item: AVSSSSSS IIIIIIII IISSSSSS SSSSLLLL LLLLLLVA */
    pHdr->stats = cHdr[4] & (NVOCTP_VALIDLENBIT | NVOCTP_ACTIVEHDRBIT);
    pHdr->len = ((cHdr[4] >> 2) & 0x3F) | ((cHdr[3] & 0x0F) << 6);
    pHdr->subid = ((cHdr[3] >> 4) & 0x0F) | ((cHdr[2] & 0x3F) << 4);
    pHdr->itmid = ((cHdr[2] >> 6) & 0x03) | ((cHdr[1] & 0xFF) << 2);
    pHdr->sysid = cHdr[0] & 0x3F;
    pHdr->stats |= cHdr[0] & (NVOCTP_VALIDIDBIT | NVOCTP_ACTIVEIDBIT);

    pHdr->cmpid = NVOCTP_CMPRID(pHdr->sysid, pHdr->itmid, pHdr->subid);
}

/*!
 * @fn      NVOCTP_setItemInactive
 *
 * @brief   Mark an item as inactive
 *
 * @param   iOfs - Offset to item header in active page
 *
 * @return  none
 */
static void NVOCTP_setItemInactive(uint32_t iOfs)
{
    uint8_t tmp;

    /* Get first byte of item header */
    tmp = NVOCTP_readByte(activePg, iOfs);

    /* Remove ACTIVE_IDS_MARK */
    tmp &= ~NVOCTP_ACTIVEIDBIT;

    /* Mark the item as inactive */
    NVOCTP_writeByte(activePg, iOfs, tmp);
}

/*!
 * @fn      NVOCTP_setPageActive
 *
 * @brief   Set specified NV page to active state
 *
 * @param   pg - NV page to activate
 *
 * @return  none
 */
static void NVOCTP_setPageActive(uint32_t pg)
{
    uint8_t cycle;

    /* Bump the compaction cycle counter, wrap-around if at maximum */
    cycle = (uint8_t)((pgCycle < NVOCTP_MAXCYCLE) ? (pgCycle + 1) : NVOCTP_MINCYCLE);

    /* Start with unique NV page signature, */
    NVOCTP_writeByte(pg, NVOCTP_PGHDRSIG, (uint8_t)nvSignature);
    /* then NV page format version, */
    NVOCTP_writeByte(pg, NVOCTP_PGHDRVER, (uint8_t)nvVersion);
    /* then page compaction cycle counter, */
    NVOCTP_writeByte(pg, NVOCTP_PGHDRCYC, (uint8_t)cycle);
    /* finish with NV page state */
    NVOCTP_writeByte(pg, NVOCTP_PGHDRPST, (uint8_t)NVOCTP_PGACTIVE);

    if(failW == NVINTF_SUCCESS)
    {
        /* No errors, switch active page */
        activePg = pg;
        pgCycle = cycle;
    }
}

/*!
 * @fn      NVOCTP_findOffset
 *
 * @brief   Find the offset to next available empty space in specified page
 *
 * @param   pg   - Valid NV page on which to find offset to next available data
 * @param   bOfs - Beginning offset to start search
 *
 * @return  Number of bytes from start of page to next available item location
 */
static uint32_t NVOCTP_findOffset(uint32_t pg,
                                  uint32_t bOfs)
{
    uint32_t i,j;
    uint32_t tmp;

    tmp = 0;
    /* Find first non-erased 4-byte location */
    while(bOfs >= sizeof(tmp))
    {
        bOfs -= sizeof(tmp);
        NVOCTP_flashRead(pg, bOfs, sizeof(tmp), (uint8_t *)&tmp);
        if(tmp != NVOCTP_ERASEDWORD)
        {
            break;
        }
    }

    /* Starting with LSB, look for non-erased byte */
    for(i = j = 1; i <= 4; i++)
    {
        if((tmp & 0xFF) != NVOCTP_ERASEDBYTE)
        {
            /* Last non-erased byte so far */
            j = i;
        }
        tmp >>= 8;
    }

    return (bOfs + j);
}

/*!
 * @fn      NVOCTP_findItem
 *
 * @brief   Find a valid item from designated page and offset
 *
 * @param   pg  - Valid NV page
 * @param   ofs - Offset in NV page from where to start search
 * @param   cid - Compressed NV item ID to search for
 *
 * @return  When >0, offset to the item header for found item
 *          When <=0, -number of items searched when item not found
 *
 */
static int32_t NVOCTP_findItem(uint32_t pg,
                               uint32_t ofs,
                               uint32_t cid)
{
    uint32_t items = 0;

    while(ofs >= (NVOCTP_PGHDRLEN + NVOCTP_ITEMHDRLEN))
    {
        NVOCTP_itemHdr_t iHdr;

        /* Align to start of item header */
        ofs -= NVOCTP_ITEMHDRLEN;

        /* Read and decompress item header */
        NVOCTP_readHeader(pg, ofs, &iHdr);

        if((cid == iHdr.cmpid) &&
           (iHdr.stats & NVOCTP_ACTIVEIDBIT) &&
           !(iHdr.stats & NVOCTP_VALIDIDBIT))
        {
            /* Item found - return offset of item header */
            return (ofs);
        }
        else if(!(iHdr.stats & NVOCTP_VALIDLENBIT))
        {
            /* Item length appears to be valid */
            if(iHdr.len < ofs)
            {
                /* Adjust offset for next try */
                ofs -= iHdr.len;
            }
            else
            {
                /* Should never get here - item is corrupt */
                failF = failW = NVINTF_BADLENGTH;
                NVOCTP_EXCEPTION(pg, failF);
                break;
            }
        }
        else
        {
            /* Length is invalid, find offset to previous item */
            ofs = NVOCTP_findOffset(pg, ofs - 1);
        }

        /* Running count of items searched */
        items += 1;
    }

    /* Item not found (negate number of items searched) */
    return (-(int16_t)items);
}

/*
 * @fn      NVOCTP_compactPage
 *
 * @brief   Compact specified page by copying active items to other page
 *
 *          Compaction occurs under two circumstances: (1) 'maintenance'
 *          activity where an NV page is packed to remove out-of-use items,
 *          (2) 'update' activity where an NV page is packed to make room
 *          for an item being written. The 'update' mode is performed by
 *          writing the item after the rest of the page has been compacted
 *
 * @param   srcPg - Valid NV page to compact from
 * @param   iWrtp - Ptr to item write parameters
 *
 * @return  Number of available bytes on compacted page, -1 if error
 */
static int32_t NVOCTP_compactPage(uint32_t srcPg,
                                  NVOCTP_itemWrp_t *iWrtp)
{
    uint32_t loop;
    uint32_t dstPg;
    uint32_t dstOff;
    uint32_t endOff;
    uint32_t nonOff;
    uint32_t srcOff;
    uint32_t uicid;
#if defined (NVOCTP_DIAGNOSTICS)
    uint32_t nvcid;
    uint32_t aitems = 0;
    uint32_t ritems = 0;
    uint32_t nvdOfs = 0;
#endif

    /* Reset Flash erase/write fail indicator */
    failW = NVINTF_SUCCESS;

    /* Select the destination page */
    uicid = (iWrtp == NULL) ? NVOCTP_INVCMPID : iWrtp->iHdr->cmpid;

    /* Select the destination page */
    dstPg = (srcPg == nvBegPage) ? nvEndPage : nvBegPage;

    /* Ensure that destination page is ready */
    NVOCTP_erasePage(dstPg);

    /* Mark the specified page to be in XFER state */
    NVOCTP_writeByte(srcPg, NVOCTP_PGHDROFS, (uint8_t)NVOCTP_PGXFER);

    /* Destination items start right after page header */
    dstOff = NVOCTP_PGDATAOFS;
#if defined (NVOCTP_DIAGNOSTICS)
    /* Create a compressed item ID for NV diagnostic */
    nvcid = NVOCTP_CMPRID(NVINTF_SYSID_NVDRVR, 1, 0);
    /* Reserved space for NV driver diagnostic item */
    dstOff += NVOCTP_ITEMHDRLEN + sizeof(NVOCTP_diag_t);
#endif

    nonOff = pgOff;
    endOff = NVOCTP_ITEMHDRLEN;

    for(loop = 0; loop < 2; loop++)
    {
        /* Source items start with the last written item */
        srcOff = pgOff;

        while(srcOff > endOff)
        {
            if(failW == NVINTF_SUCCESS)
            {
                NVOCTP_itemHdr_t srcHdr;
                uint32_t dataLen;
                uint32_t itemSize;

                /* Align to start of item header */
                srcOff -= NVOCTP_ITEMHDRLEN;

                /* Read and decompress item header */
                NVOCTP_readHeader(srcPg, srcOff, &srcHdr);
                dataLen = srcHdr.len;
                itemSize = NVOCTP_ITEMHDRLEN + dataLen;

                if(!(srcHdr.stats & NVOCTP_VALIDIDBIT))
                {
                    /* Only search for 'valid' items not being updated */
                    if((srcHdr.stats & NVOCTP_ACTIVEIDBIT) &&
                       (srcHdr.cmpid != uicid) &&
                       (NVOCTP_findItem(dstPg, dstOff, srcHdr.cmpid) <= 0))
                    {
                        uint32_t xfrOfd = 0;

                        /* This item has not been transferred yet, */
                        if(loop == 0)
                        {
                            /* 1st loop, copy only 'reserved' items (0xFF data) */
                            if(NVOCTP_isErased(srcPg, srcOff - 1, dataLen))
                            {
                                /* Prepare to copy 'reserved' item header only */
                                xfrOfd = dstOff;
                                dstOff += itemSize;
                            }
                            else
                            {
                                /* Location of last 'non-reserved' item found */
                                nonOff = srcOff - dataLen;
#if defined (NVOCTP_DIAGNOSTICS)
                                if(srcHdr.cmpid == nvcid)
                                {
                                    /* Reserve space for NV diagnostic item, */
                                    /* always at beginning of compacted page */
                                    xfrOfd = NVOCTP_PGDATAOFS;
                                    /* Location of source NV diagnostic data */
                                    nvdOfs = nonOff;
                                }
#endif
                            }
                        }
                        else
                        {
                            /* 2nd loop, copy 'active' items (non-erased data) */
                            xfrOfd = dstOff;
                            dstOff += itemSize;
                        }

                        if(xfrOfd > 0)
                        {
                            cmpIH_t cHdr;
                            uint32_t hdrOfd = xfrOfd + dataLen;

                            /* Get copy of compressed header */
                            NVOCTP_flashRead(srcPg, srcOff, NVOCTP_ITEMHDRLEN,
                                             &cHdr[0]);
                            /* Clear all 'valid' bits, write the header */
                            NVOCTP_writeHeader(dstPg, hdrOfd, (uint8_t *)cHdr);
                            if(loop > 0)
                            {
                                /* Copy data to the XFER page */
                                NVOCTP_copyItem(dstPg, srcOff - dataLen,
                                                xfrOfd, itemSize);
                            }
#if defined (NVOCTP_DIAGNOSTICS)
                            else
                            {
                                /* Count a reserved item */
                                ritems += 1;
                            }
                            aitems += 1;
#endif
                            /* Mark the IDs as VALID */
                            cHdr[0] &= ~NVOCTP_VALIDIDBIT;
                            NVOCTP_writeByte(dstPg, hdrOfd, cHdr[0]);
                        }
                    }
                    srcOff -= dataLen;
                }
                else
                {
                    /* Invalid entry, ignore this item header */
                    if(srcHdr.stats & NVOCTP_VALIDLENBIT)
                    {
                        /* Length not validated, scan for next possible header */
                        srcOff = NVOCTP_findOffset(srcPg, srcOff);
                    }
                    else
                    {
                        if(itemSize <= srcOff)
                        {
                            /* Data not validated, ignore this item entry */
                            srcOff -= dataLen;
                        }
                        else
                        {
                            /* Invalid length, source page must be corrupt. */
                            failF = failW = NVINTF_BADLENGTH;
                            NVOCTP_EXCEPTION(srcPg, failW);
                            return ((uint32_t)(-1));
                        }
                    }
                }
            }
            else
            {
                /* Failure during item xfer makes next findItem() unreliable */
                return ((uint32_t)(-1));
            }
        }
        endOff = nonOff;
    }

    /* Check for compaction due to item update */
    if(uicid != NVOCTP_INVCMPID)
    {
        uint32_t tmpOff = pgOff;
        /* Use the destination page offset */
        pgOff = dstOff;
        /* Copy the item which caused the compaction */
        NVOCTP_writeItem(iWrtp->iHdr, dstPg, iWrtp->dOfs, iWrtp->bOfs,
                         iWrtp->bLen, iWrtp->pBuf);
        /* Restore source page offset in case there's an error */
        dstOff = pgOff;
        pgOff = tmpOff;
#if defined (NVOCTP_DIAGNOSTICS)
        aitems += 1;
    }
    if(nvdOfs > 0)
    {
        NVOCTP_diag_t diags;
        /* Get the diagnostic info */
        NVOCTP_flashRead(srcPg, nvdOfs, sizeof(diags), (uint8_t *)&diags);
        /* One more erase/compaction is complete */
        diags.compacts += 1;
        /* Number of items copied */
        diags.active = (uint16_t)aitems;
        /* Number of reserved items */
        diags.reserved = (uint16_t)ritems;
        /* Available space after this item uddate */
        diags.available = (uint16_t)(nvPageSize - dstOff);
        /* Update with current info */
        NVOCTP_flashWrite(dstPg, NVOCTP_PGDATAOFS, sizeof(diags),
                          (uint8_t *)&diags);
#endif
    }

    /* All items have been copied - activate the new page */
    NVOCTP_setPageActive(dstPg);

    if(failW != NVINTF_SUCCESS)
    {
        /* Something bad happened when trying to compact the page */
        return ((uint32_t)(-1));
    }

    /* Next item offset for activePg */
    pgOff = dstOff;
    /* Erase the previous active page */
    NVOCTP_erasePage(srcPg);

    /* Tell caller how much room is left on the active page */
    return ((uint32_t)(nvPageSize - dstOff));
}

/*!
 * @fn      NVOCTP_copyItem
 *
 * @brief   Copy an NV item from active page to specified destination page
 *
 * @param   dstPg - Destination page
 * @param   sOfs  - Source page offset of original data in active page
 * @param   dOfs  - Destination page offset to tranfered copy of the item
 * @param   len   - Length of data to copy
 *
 * @return  none.
 */
static void NVOCTP_copyItem(uint32_t dstPg,
                            uint32_t sOfs,
                            uint32_t dOfs,
                            uint32_t len)
{
    uint32_t num;
    uint8_t tmp[NVOCTP_XFERBLKMAX];

    /* Copy over the data: Flash to RAM, then RAM to Flash */
    while(len > 0)
    {
        /* Number of bytes to transfer in this block */
        num = (len < NVOCTP_XFERBLKMAX) ? len : NVOCTP_XFERBLKMAX;

        /* Get block of bytes from Flash */
        NVOCTP_flashRead(activePg, sOfs, num, (uint8_t *)&tmp);

        /* Get block of bytes from Flash */
        NVOCTP_flashWrite(dstPg, dOfs, num, (uint8_t *)&tmp);

        dOfs += num;
        sOfs += num;
        len -= num;
    }
}

/*!
 * @fn      NVOCTP_dataChange
 *
 * @brief   Compare active page NV data block against user's buffer
 *
 * @param   ofs  - A valid offset into the page
 * @param   len  - Length of data block to compare
 * @param   pBuf - Pointer to data buffer
 *
 * @return  Type of change: NO_CHANGE, ONE_TO_ZERO, ZERO_TO_ONE
 */
static uint32_t NVOCTP_dataChange(uint32_t ofs,
                                  uint32_t len,
                                  uint8_t *pBuf)
{
    uint32_t tmp;
    uint32_t rtn = NVOCTP_NOCHANGE;

    while(len--)
    {
        /* Get a byte from Flash */
        tmp = NVOCTP_readByte(activePg, ofs++);

        /* See if any bit(s) are different */
        tmp = (tmp ^ ((uint32_t)(*pBuf))) & 0x0ff;
        if(tmp != 0)
        {
            /* At least one bit changed, */
            if((tmp & *pBuf) != 0)
            {
                /* And at least one went 0 -> 1 */
                rtn = NVOCTP_ZEROTOONE;
                /* No need to look anymore */
                break;
            }
            else
            {
                /* And at least one went 1 -> 0 */
                rtn = NVOCTP_ONETOZERO;
            }
        }
        pBuf++;
    }

    return (rtn);
}

/*!
 * @fn      NVOCTP_readByte
 *
 * @brief   Read one byte from Flash memory
 *
 * @param   pg  - NV Flash page
 * @param   ofs - Offset into the page
 *
 * @return  byte read from flash memory
 */
static uint8_t NVOCTP_readByte(uint32_t pg,
                               uint32_t ofs)
{
    uint8_t *p;

    p = NVOCTP_FLASHADDR(pg, ofs);

    return (*p);
}

/*!
 * @fn      NVOCTP_flashRead
 *
 * @brief   Copy data block from NV memory into caller's buffer
 *
 * @param   pg   - A valid NV Flash page
 * @param   ofs  - A valid offset into the page
 * @param   num  - Number of data bytes to read
 * @param   pBuf - Pointer to caller's data buffer
 *
 * @return  none
 */
static void NVOCTP_flashRead(uint32_t pg,
                             uint32_t ofs,
                             uint32_t num,
                             uint8_t *pBuf)
{
    /* Aboslute address of Flash data block */
    uint8_t *addr = NVOCTP_FLASHADDR(pg, ofs);
    memmove((void *)(pBuf), (void *)(addr), num);
    LOG_printf(LOG_DBG_NV_rdwr, "read: pg:%d, ofs=0x%04x, num=%d\n",pg,ofs,num);
    LOG_hexdump(LOG_DBG_NV_rdwr, (pg * nvPageSize) + ofs, pBuf, num);
}

/*!
 * @fn      NVOCTP_writeByte
 *
 * @brief   Write one byte to Flash and read back to verify
 *
 * @param   pg  - NV Flash page
 * @param   ofs - offset into the page
 * @param   bwv - byte to write & verify
 *
 * @return  none ('failF' or 'failW' will be set if write fails)
 */
static void NVOCTP_writeByte(uint32_t pg,
                             uint32_t ofs,
                             uint8_t bwv)
{
    NVOCTP_flashWrite(pg, ofs, 1, (uint8_t *)&bwv);
}

/*!
 * @fn      NVOCTP_flashWrite
 *
 * @brief   Copy data block from caller's buffer into NV memory and verify
 *
 * @param   pg   - A valid NV Flash page
 * @param   ofs  - A valid offset into the page
 * @param   num  - Number of data bytes to write
 * @param   pBuf - Pointer to source data buffer
 *
 * @return  none (sets 'failF' if verify fails, sets 'failW' if write fails)
 */
static void NVOCTP_flashWrite(uint32_t pg,
                              uint32_t ofs,
                              uint32_t num,
                              uint8_t *pSrc)
{
    uint8_t *pDst;

    if(!NVOCTP_FLASHACCESS())
    {
        /* Power monitor indicates low voltage */
        failW = NVINTF_LOWPOWER;
    }

    if(failW != NVINTF_SUCCESS)
    {
        return;
    }

    pDst = NVOCTP_FLASHADDR(pg, ofs);
    LOG_printf(LOG_DBG_NV_rdwr, "write: pg:%d, ofs=0x%04x, num=%d\n",pg,ofs,num);
    LOG_hexdump(LOG_DBG_NV_rdwr, (pg * nvPageSize) + ofs, pSrc, num);
    memmove(pDst, pSrc, num);

    if(0 != memcmp(pSrc, pDst, num))
    {
        /* Miscompare - give up now */
        failF = failW = NVINTF_CORRUPT;
    }
}

/*!
 * @fn      NVOCTP_erasePage
 *
 * @brief   Erases a page in Flash if the page is not completely erased.
 *
 * @param   pg - NV page to erase.
 *
 * @return  none (sets 'failF' if erase fails, sets 'failW' if write fails)
 */
static void NVOCTP_erasePage(uint32_t pg)
{
    bool err;
    uint8_t pgState;
    uint8_t *pBuf;

    err = false;
    pgState = NVOCTP_readByte(pg, NVOCTP_PGHDRPST);

    if((pgState == NVOCTP_PGERASED) &&
       (NVOCTP_isErased(pg, NVOCTP_PGDATAEND, NVOCTP_PGDATALEN) == true))
    {
        /* we are done */
        return;
    }

    if(!NVOCTP_FLASHACCESS())
    {
        failW = NVINTF_LOWPOWER;
        return;
    }

    pBuf = NVOCTP_FLASHADDR(pg, 0);
    memset((void *)(pBuf), NVOCTP_ERASEDBYTE, nvPageSize);

    if(err)
    {
        /* Attempt to erase failed */
        failF = failW = NVINTF_CORRUPT;
        return;
    }
    /* Looks good so far, mark it as erased */
    NVOCTP_writeByte(pg, NVOCTP_PGHDROFS, (uint8_t)NVOCTP_PGERASED);
}

/*!
 * @fn      NVOCTP_isErased
 *
 * @brief   Checks an NV item data memory block to see if it's erased
 *          NOTE: this function scans memory bytes backwards (high to low)
 *
 * @param   pg - Flash page to check
 * @param   ofs - Flash page offset of first byte to check
 * @param   num - Number of bytes to check
 *
 * @return  true if erased, false if not erased
 */
static bool NVOCTP_isErased(uint32_t pg,
                            uint32_t ofs,
                            uint32_t num)
{
    uint8_t *pBuf;
    uint8_t *pEnd;
    bool ok;

    pBuf = NVOCTP_FLASHADDR(pg, ofs);
    pEnd = NVOCTP_FLASHADDR(pg, ofs + num);
    ok = true;
    while(pBuf < pEnd)
    {
        if(*pBuf != NVOCTP_ERASEDBYTE)
        {
            ok = false;
            break;
        }

        pBuf++;
    }
    return (ok);
}

/*!
 * @fn      NVOCTP_disableCache
 *
 * @brief   Save current TARGET cache mode and disable cache
 *
 * @param   vm - pointer to cache mode
 *
 * @return  none
 */
static void NVOCTP_disableCache(uint32_t *vm)
{
    (void)(vm);
    /* do nothing */
}

/*!
 * @fn      NVOCTP_restoreCache
 *
 * @brief   Restore TARGET cache to previous mode
 *
 * @param   vm - cache mode
 *
 * @return  none
 */
static void NVOCTP_restoreCache(uint32_t vm)
{
    (void)(vm);
    /* do nothing */
}

/*!
 * @brief  Save the NV simulation to disk
 */
void NV_LINUX_save(void)
{
    intptr_t s;
    int r;

    LOG_printf(LOG_DBG_NV_dbg, "nvram: save: %s, length=%d\n",
               NV_filename,
               NV_ramLength);

    s = STREAM_createWrFile(NV_filename);
    if(s == 0)
    {
        FATAL_perror(NV_filename);
    }
    r = STREAM_wrBytes(s, NV_ramSim, NV_ramLength, 0);
    STREAM_close(s);

    if(r != (int)NV_ramLength)
    {
        FATAL_printf("%s: Cannot write %d bytes, wrote: %d instead\n",
                     NV_filename,
                     NV_ramLength,
                     r);
    }
}

/*!
 * @brief Load the NV simulation file from disk.
 */
void NV_LINUX_load(void)
{
    int r;
    int64_t filesize;
    intptr_t s;

    NV_ramLength = (nvEndPage - nvBegPage + 1) * nvPageSize;

    /* Get memory for the NV implimentation */
    NV_ramSim = calloc(1,NV_ramLength);
    if(!NV_ramSim)
    {
        FATAL_printf("NV no ram\n");
    }

    /* simuate erased data */
    memset(NV_ramSim, NVOCTP_ERASEDBYTE, NV_ramLength);

    /* Load Linux NV RAM simulation */

    if(NV_filename == NULL)
    {
        /* the filename should be specified in the ini file */
        BUG_HERE("missing nv filename");
    }

    if(!CONFIG_NV_RESTORE)
    {
        LOG_printf(LOG_DBG_NV_dbg, "config: No load NV, clearing old NV file\n");
        goto over_write;
    }

    filesize = STREAM_FS_getSize(NV_filename);
    /* if the file exists then load it */
    if(filesize == NV_ramLength)
    {
        s = STREAM_createRdFile(NV_filename);
        if(s == 0)
        {
            FATAL_perror(NV_filename);
        }
        r = STREAM_rdBytes(s, NV_ramSim, NV_ramLength, 0);
        if(r != ((int)NV_ramLength))
        {
            FATAL_printf("nvram: %s, expected %d, got %d\n",
                         NV_filename,
                         NV_ramLength,
                         r);
        }
        LOG_printf(LOG_DBG_NV_dbg,
                   "nvram: Loaded: %s, length=%d\n",
                   NV_filename,
                   NV_ramLength);
    }
    else
    {
    over_write:
        LOG_printf(LOG_DBG_NV_dbg,
                   "nvram: creating: %s\n", NV_filename);
        /* we just write it */
        NV_LINUX_save();
    }
}

/*
   Process the INI file settings

   Public functio nin nv_linux.h
 */
int NV_LINUX_INI_settings(struct ini_parser *pINI, bool *handled)
{
    if(INI_itemMatches(pINI, "nv", "filename"))
    {
        if(NV_filename)
        {
            /*
               Thou shall not 'free' a constant string!
               aka: the default filename
             */
            if(NV_filename != _nv_default_filename)
            {
                free_const((const void *)NV_filename);
            }
            NV_filename = NULL;
        }
        NV_filename = INI_itemValue_strdup(pINI);
        *handled = true;
        return (0);
    }

    if(INI_itemMatches(pINI, "nv", "page-size-bytes"))
    {
        nvPageSize = INI_valueAsInt(pINI);
        *handled = true;
        return (0);
    }

    if(INI_itemMatches(pINI, "nv", "num-pages"))
    {
        nvEndPage = nvBegPage + INI_valueAsInt(pINI);
        *handled = true;
        return (0);
    }

    if(INI_itemMatches(pINI, "nv", "reserved-pages"))
    {
        /* how many pages do we have now? */
        int n;
        n = nvEndPage - nvBegPage;
        nvBegPage = INI_valueAsInt(pINI);
        nvEndPage = nvBegPage + n;

        *handled = true;
        return (0);
    }

    /* unknown */
    return (0);
}

/*!
 * @fn      NVOCTP_initNvApi
 *
 * @brief   API function to initialize the specified NV Flash pages
 *
 * @param   param - pointer to caller's structure of NV init parameters
 *
 * @return  NVINTF_SUCCESS or specific failure code
 */
static uint8_t NVOCTP_initNvApi(void *param)
{
    uint32_t pg;
    uint32_t xferPg;

    /* we don't use the param */
    (void)(param);

    NV_LINUX_init();

    failW = failF;

    /* Note: System initalization has failF set to NVINTF_NOTREADY */
    if(failF != NVINTF_NOTREADY)
    {
        return (failW);
    }

    /* Only one init per device reset */
    failF = failW = NVINTF_SUCCESS;
    xferPg = NVOCTP_NULLPAGE;
    activePg = NVOCTP_NULLPAGE;

    /* Look for active page and clean up the other if necessary */
    for(pg = nvBegPage; pg <= nvEndPage; pg++)
    {
        NVOCTP_pageHdr_t pHdr;

        NVOCTP_flashRead(pg, NVOCTP_PGHDROFS, NVOCTP_PGHDRLEN,
                         (uint8_t *)&pHdr);
        if((pHdr.state == NVOCTP_PGACTIVE) &&
           (pHdr.cycle <= NVOCTP_MAXCYCLE) &&
           (pHdr.signature == nvSignature))
        {

            /* Looks like a valid page header */
            if(pHdr.version != nvVersion)
            {
                /* NV page and NV driver versions are different */
                NVOCTP_EXCEPTION(pg, NVINTF_BADVERSION);
                LOG_printf(LOG_DBG_NV_dbg,
                           "  >> ++ NV page and NV driver versions are different \n");

            }

            if(activePg == NVOCTP_NULLPAGE)
            {
                /* First active page found */
                activePg = pg;
                pgCycle = pHdr.cycle;

            }
            else
            {
                /* Both pages are in the active state, pick one... */
                if(((pgCycle > pHdr.cycle) &&
                    ((pgCycle != NVOCTP_MAXCYCLE) &&
                     (pHdr.cycle != NVOCTP_MINCYCLE))) ||
                   ((pgCycle < pHdr.cycle) &&
                    ((pgCycle == NVOCTP_MINCYCLE) &&
                     (pHdr.cycle == NVOCTP_MAXCYCLE))))
                {
                    /* First page found is newer, erase the second */
                    NVOCTP_erasePage(pg);

                }
                else
                {
                    /* Second page found is newer, erase the first */
                    NVOCTP_erasePage(activePg);
                    activePg = pg;
                    pgCycle = pHdr.cycle;
                }
            }
        }
        else if(pHdr.state == NVOCTP_PGXFER)
        {
            xferPg = pg;
            if(pgCycle == 0)
            {
                /* Restore interrupted compaction cycle count */
                pgCycle = pHdr.cycle;
            }
        }
        else if(pHdr.state != NVOCTP_PGERASED)
        {
            /* Ensure that interrupted compaction page erase gets finished */
            NVOCTP_erasePage(pg);
        }
    }

    if(activePg == NVOCTP_NULLPAGE)
    {
        if(xferPg == NVOCTP_NULLPAGE)
        {
            /* Both pages are erased. Initial state, select an active page */
            NVOCTP_setPageActive(nvBegPage);
            pgOff = NVOCTP_PGDATAOFS;
            /* Debug msg */
        }
        else
        {
            /* Compacting interrupted in previous power cycle - do it now */
            activePg = xferPg;
            pgOff = NVOCTP_findOffset(xferPg, nvPageSize);
            (void)NVOCTP_compactPage(xferPg, NULL);
        }
    }
    else
    {
        if(xferPg != NVOCTP_NULLPAGE)
        {
            /* Compacting completed except for last step to erase xferPage */
            NVOCTP_erasePage(xferPg);
        }
        /* Find the active page offset for next NV item write */
        pgOff = NVOCTP_findOffset(activePg, nvPageSize);
    }

#if defined (NVOCTP_DIAGNOSTICS)
    {
        uint8_t err;
        NVOCTP_diag_t diags;

        /* Look for a copy of diagnostic info */
        err = NVOCTP_readItemApi(diagId, 0, sizeof(diags), &diags);
        if(err == NVINTF_NOTFOUND)
        {
            /* Assume this is the first time, */
            memset(&diags, 0, sizeof(diags));
            /* Space available for everything else */
            diags.available = (uint16_t)(nvPageSize - (pgOff + NVOCTP_ITEMHDRLEN
                                                       + sizeof(diags)));
        }
        /* Remember this reset */
        diags.resets += 1;
        /* Create/Update the diagnostic NV item */
        NVOCTP_writeItemApi(diagId, sizeof(diags), &diags);
    }
#endif
    return (failW);
}

/*
  Initialize the NV simulation.

  Public function defined in nv_linux.h
 */
void NV_LINUX_init(void)
{
    /* Load the simulation file.. */
    NV_LINUX_load();

    if(nvMutex == 0)
    {
        (void)NVOCTP_disableCache;
        (void)NVOCTP_restoreCache;
        nvMutex = MUTEX_create("nv-mutex");
    }
}

/**
 * @fn      NVOCTP_loadApiPtrs
 *
 * @brief   Global function to return function pointers for NV driver API that
 *          are supported by this module, NULL for functions not supported.
 *
 * @param   pfn - pointer to caller's structure of NV function pointers
 *
 * @return  none
 */
void NVOCTP_loadApiPtrs(NVINTF_nvFuncts_t *pfn)
{
    /* Load caller's structure with pointers to the NV API functions */
    pfn->initNV      = &NVOCTP_initNvApi;
    pfn->compactNV   = &NVOCTP_compactNvApi;
    pfn->createItem  = &NVOCTP_createItemApi;
    pfn->deleteItem  = &NVOCTP_deleteItemApi;
    pfn->readItem    = &NVOCTP_readItemApi;
    pfn->writeItem   = &NVOCTP_writeItemApi;
    pfn->writeItemEx = &NVOCTP_writeItemExApi;
    pfn->getItemLen  = &NVOCTP_getItemLenApi;
}

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
