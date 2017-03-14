/******************************************************************************

 @file cc13xxdnld.h

 @brief API for downloading to the CC13/26xx Flash using the ROM
        Bootloader

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

/*
//!
//! \defgroup CcDnld
*/

/*
//!
//! \ingroup CcDnld
//@{
*/

//*****************************************************************************
//  @file cc13xxdnld.h
//
//  @brief          API for downloading to the CC13/26xx Flash using the ROM
//                          Bootloader
//
// # Overview #
// The CcDnld API should be used in application code. The CcDnld API is
// intended to ease the integration of the CC13/26xx bootloader functionality
// in a host processor connected to the CC13/26xx UART[Use Cases]
// (@ref USE_CASES).
//
// # General Behavior #
// Before using the CcDnld API:
//   - A binary image should be prepared that is to be loaded into the cc13/26xx
//       Flash.
//  .
// To load data to the CC13/26xx Flash one should:
//   - Set the UART send/recv data callbacks with CcDnld_init().
//   - Connect to the CC13xx/26xx ROM bootloader with CcDnld_connect().
//   - Optionally Erase Flash with CcDnld_flashEraseRange().
//   - Optionally Program Flash with CcDnld_startDownload() and CcDnld_sendData().
//   - Optionally Erase Flash with CcDnld_flashEraseRange().
//   - Optionally Verify Flash with CcDnld_verify().
//
// # Error handling #
//      The CcDnld API will return CcDnld_Status containing success or error
//      code. The CcDnld_Status codes are:
//      CcDnld_Status_Success
//      CcDnld_Status_Cmd_Error
//      CcDnld_Status_State_Error
//      CcDnld_Status_Param_Error
//   .
//
//
// # Supported Functions #
// | Generic API function          | Description                                       |
// |-------------------------------|---------------------------------------------------|
// | CcDnld_init()                 | Registers the UART read/write function points     |
// | CcDnld_connect()              | Connects to the CC13xx/26xx ROM bootloader        |
// | CcDnld_flashEraseRange()      | Erases the spicified flash range                  |
// | CcDnld_startDownload()        | Sends the download command to the CC13xx/26xx ROM |
// |                               | bootloader                                        |
// | CcDnld_sendData()             | Sends program data to be program data to be       |
// |                               | programmed in to flash                            |
// | CcDnld_verify()               | Verify a flash range with data in a buffer        |
//
//  # Not Supported Functionality #
//
//
//*****************************************************************************
#ifndef CcDnld__include
#define CcDnld__include

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

/// \brief defines the API version
#define CCDNLD_API_VERSION                      "ccDnld-v1.00.00"

/// \brief defines CC13/26xx Flash Page Size
#define CCDNLD_CC13XX_PAGE_SIZE                 4096

/// \brief defines number CC13/26xx Flash Pages
#define CCDNLD_CC13XX_NUM_PAGES                 32

/// \brief defines Max number of bytes that can be transfered in 1 command
#define CCDNLD_MAX_BYTES_PER_TRANSFER           252

/// \brief defines start address
#define CCDNLD_FLASH_START_ADDRESS              0x00000000

/// \brief CcDnld Status and error codes
typedef enum
{
        CcDnld_Status_Success               = 0, ///Success
        CcDnld_Status_Cmd_Error             = 1, ///Command Error
        CcDnld_Status_State_Error           = 2, ///Device State Error
        CcDnld_Status_Param_Error           = 3, ///Parameter Error
        CcDnld_Status_Crc_Error             = 4, ///CRC Error
} CcDnld_Status;

/// \brief UART read write function pointers. These functions are provided by the
///              OS / Platform specific layer to access the CC13xx/26xx UART
typedef struct CcDnld_UartFxns_t {
        uint8_t (*sbl_UartReadByte)(void);
        void (*sbl_UartWriteByte)(uint8_t);
        void (*sbl_UartWrite)(uint8_t*, uint32_t);

} CcDnld_UartFxns_t;

//*****************************************************************************
//
//! \brief Initialises UART function pointer table.
//!
//! This function initializes the UART functions for sending SBL commands to the
//! CC13/26xx
//!
//! \param uartFxns - The UART function pioiter table.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
void CcDnld_init(CcDnld_UartFxns_t* uartFxns);

//*****************************************************************************
//
//! \brief Establishes a connnection with the ROM Bootloader.
//!
//! This function establishes a connnection with the ROM Bootloader by setting
//! the autoboard rate. By sending 0x55 the CC163/26xx ROM bootloader uses the
//! alternating 0's/1's to detect the board rate and sends an Ack/0xCC in
//! response
//!
//! \param None.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_connect(void);

//*****************************************************************************
//
//! \brief Gets the number of pages needing to be erased Erases the specified page.
//!
//! This function gets the number of pages needing to be erased based on Flash start address
//! and the Bytes to be written
//!
//! \param startAddress - The Flash start address.
//! \param byteCount        - Number of bytes to be erased.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_flashEraseRange(unsigned int startAddress,unsigned int byteCount);

//*****************************************************************************
//
//! \brief Starts the download to the CC13/26xx ROM bootloader.
//!
//! This function starts the download to the CC13/26xx ROM Bootloader
//!
//! \param startAddress - The Flash start address.
//! \param byteCount        - Total Number of bytes to be programmed.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_startDownload(uint32_t startAddress, uint32_t byteCount);

//*****************************************************************************
//
//! \brief Loads a chunk of data into the CC13/26xx Flash.
//!
//! This function Loads a chunk of data into the CC13/26xx Flash thorugh the
//! ROM bootloader
//!
//! \param startAddress - The Flash start address.
//! \param pData                        - Pointer to a buffer containing the data.
//! \param byteCount        - Number of bytes to be programmed.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_sendData(uint8_t *pData, uint32_t byteCount);

//*****************************************************************************
//
//! \brief Loads a chunk of data into the CC13/26xx Flash.
//!
//! This function Loads a chunk of data into the CC13/26xx Flash thorugh the
//! ROM bootloader
//!
//! \param dataAddress  - start address of data in Flash to be verified.
//! \param pData                - Pointer to a buffer containing the data to be
//!                                             verified.
//! \param byteCount        - Number of bytes to be programmed.
//!
//! \return CcDnld_Status
//
//*****************************************************************************
CcDnld_Status CcDnld_verifyData(uint32_t dataAddress, uint8_t *pData,
                                                            uint32_t byteCount);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* CcDnld__include */

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//
//*****************************************************************************

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

