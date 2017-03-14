/******************************************************************************
 @file llc_proto.c

 @brief TIMAC 2.0 API Convert items in the llc.h files

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

#include "llc.h"
#include "llc.pb-c.h"
#include "llc_proto.h"
#include "api_mac_proto.h"

#include "malloc.h"

/*
 * Release memory for an LlcNetInfo
 * Public functiond defined in llc_proto.h
 */
void free_LlcNetInfo(LlcNetInfo *pThis)
{
    if(pThis)
    {
        free_ApiMacDeviceDescriptor(pThis->devinfo);
        pThis->devinfo = NULL;

        free(pThis);
    }

}

/*
 * Convert a Llc_netInfo structure to protobuf form
 * Public function defined in llc_proto.h
 */
LlcNetInfo *copy_Llc_netInfo(const Llc_netInfo_t *pThis)
{
    LlcNetInfo *pResult;

    pResult = calloc(1, sizeof(*pResult));

    if(pResult == NULL)
    {
        LOG_printf(LOG_ERROR, "No Memory For: LlcNetInfo\n");
    }
    else
    {
        llc_net_info__init(pResult);
        pResult->fh = pThis->fh;
        pResult->channel = pThis->channel;

        pResult->devinfo = copy_ApiMac_deviceDescriptor(&(pThis->devInfo));

    }
    return pResult;
}

/*
 * Free a device list item
 * Public function defined in llc_proto.h
 */
void free_LlcDeviceListItem(LlcDeviceListItem *pThis)
{
    if(pThis)
    {
        free_ApiMacDeviceDescriptor(pThis->devinfo);
        free_ApiMacCapabilityInfo(pThis->capinfo);
        free( (void *)(pThis) );
    }
}

/*
 * Convert a device list item
 * Public function defined in llc_proto.h
 */
LlcDeviceListItem *copy_Llc_deviceListItem(const Llc_deviceListItem_t *pThis)
{
    LlcDeviceListItem *p;

    p = calloc(1, sizeof(*p));
    if(p == NULL)
    {
        LOG_printf(LOG_ERROR, "No memory for: LlcDeviceListItem\n");
    }
    else
    {
        llc_device_list_item__init(p);
        p->devinfo = copy_ApiMac_deviceDescriptor(&(pThis->devInfo));
        p->capinfo = copy_ApiMac_capabilityInfo(&(pThis->capInfo));
        p->rxframecounter = pThis->rxFrameCounter;
        p->reporting = pThis->reporting;
        p->polling = pThis->polling;
        
        if((p->devinfo == NULL) || (p->capinfo == NULL))
        {
            free_LlcDeviceListItem(p);
            p = NULL;
        }
    }
    return p;
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

