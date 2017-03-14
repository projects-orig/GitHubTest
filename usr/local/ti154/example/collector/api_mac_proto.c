/******************************************************************************
 @file api_mac_proto.c

 @brief TIMAC 2.0 API Convert the items in the api_mac.h file (implimentation)

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

#include "log.h"

#include "api_mac.h"
#include "api_mac.pb-c.h"
#include "api_mac_proto.h"
#include "malloc.h"

/* common function */
static uint64_t copy_ApiMac_sAddrExt(const ApiMac_sAddrExt_t pThis)
{
    uint64_t v,a;
    int x;

    v = 0;

    for(x = 0 ; x < APIMAC_SADDR_EXT_LEN ; x++)
    {
        a = pThis[x];
        a = a << (x * 8);
        v = v | a;
    }
    return v;
}

/* Free the device descriptor.
 * Public function defined in api_mac_proto.h
 */
void free_ApiMacDeviceDescriptor(ApiMacDeviceDescriptor *pThis)
{
    if(pThis)
    {
        free((void*)(pThis));
    }
}

/* Free the capability info
 * Public function defined in api_mac_proto.h
 */
void free_ApiMacCapabilityInfo(ApiMacCapabilityInfo *pThis)
{
    if(pThis)
    {
        free((void*)(pThis));
    }
}

/* convert the device descriptor
 * Public function defined in api_mac_proto.h
 */
ApiMacDeviceDescriptor *copy_ApiMac_deviceDescriptor(
    const ApiMac_deviceDescriptor_t *pThis)
{
    ApiMacDeviceDescriptor *pResult;

    pResult = (ApiMacDeviceDescriptor *)calloc(1,sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: ApiMacDeviceDescriptor\n");
    }
    else
    {
        api_mac_device_descriptor__init(pResult);
        pResult->panid = pThis->panID;
        pResult->shortaddress = pThis->shortAddress;
        pResult->extaddress   = copy_ApiMac_sAddrExt(pThis->extAddress);
    }
    return pResult;
}

/* convert the capabiity info
 * Public function defined in api_mac_proto.h
 */
ApiMacCapabilityInfo   *copy_ApiMac_capabilityInfo(
    const ApiMac_capabilityInfo_t *pThis)
{
    ApiMacCapabilityInfo   *pResult;

    pResult = (ApiMacCapabilityInfo *)calloc(1,sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: ApiMacCapabilityInfo\n");
    }
    else
    {
        api_mac_capability_info__init(pResult);
        pResult->pancoord = pThis->panCoord;
        pResult->ffd = pThis->ffd;
        pResult->mainspower = pThis->mainsPower;
        pResult->rxonwhenidle = pThis->rxOnWhenIdle;
        pResult->security = pThis->security;
        pResult->allocaddr = pThis->allocAddr;
    }
    return pResult;
}

/* release memory for the Saddr
 * Public function defined in api_mac_proto.h
 */
void free_ApiMacSAddr(ApiMacSAddr *pThis)
{
    if(pThis)
    {
        free((void *)(pThis));
    }
}

/* Copy a Saddr
 * Public function defined in api_mac_proto.h
 */
ApiMacSAddr *copy_ApiMac_sAddr(const ApiMac_sAddr_t *pThis)
{
    ApiMacSAddr *pResult;

    pResult = calloc(1,sizeof(*pResult));

    if(pResult == NULL)
    {
        LOG_printf(LOG_ERROR,"No memory for: ApiMacSAddr\n");
        return pResult;
    }

    api_mac_s_addr__init(pResult);

    switch(pThis->addrMode)
    {
    case ApiMac_addrType_none:
        pResult->addrmode = API_MAC_ADDR_TYPE__ApiMac_addrType_none;
        break;
    case ApiMac_addrType_short:
        pResult->addrmode = API_MAC_ADDR_TYPE__ApiMac_addrType_short;
        pResult->shortaddr = pThis->addr.shortAddr;
        break;
    case ApiMac_addrType_extended:
        pResult->addrmode = API_MAC_ADDR_TYPE__ApiMac_addrType_extended;
        pResult->extaddr = copy_ApiMac_sAddrExt(pThis->addr.extAddr);
        break;
    }

    return pResult;

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

