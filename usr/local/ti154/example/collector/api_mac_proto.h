/******************************************************************************
 @file api_mac_proto.h

 @brief TIMAC 2.0 API Convert the items in the api_mac.h file

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

#if !defined(API_MAC_PROTO_H)
#define API_MAC_PROTO_H

#include "api_mac.pb-c.h"

/*!
 * @brief free the Device descriptor
 */
void free_ApiMacDeviceDescriptor(ApiMacDeviceDescriptor *pThis);

/*!
 * @brief free the device capability info message
 */
void free_ApiMacCapabilityInfo(ApiMacCapabilityInfo *pThis);

/*!
 * @brief free the Saddr message
 */
void free_ApiMacSAddr(ApiMacSAddr *pThis);

/*!
 * @brief convert the Saddr message.
 */
ApiMacSAddr *copy_ApiMac_sAddr(const ApiMac_sAddr_t *pThis);

/*!
 * @brief Convert the device descriptor
 */
ApiMacDeviceDescriptor *copy_ApiMac_deviceDescriptor(
    const ApiMac_deviceDescriptor_t *pThis);

/*!
 * @brief convert the capability info
 */
ApiMacCapabilityInfo   *copy_ApiMac_capabilityInfo(
    const ApiMac_capabilityInfo_t *pThis);

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

