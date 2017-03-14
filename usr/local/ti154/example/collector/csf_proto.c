/******************************************************************************
 @file csf_proto.c

 @brief TIMAC 2.0 API Manage csf.h structures to protobuf conversion

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
#include "csf.h"
#include "api_mac_proto.h"
#include "csf_proto.h"

#include "malloc.h"

/*
 * Free a device list item
 * Public function defined in csf_proto.h
 */
void free_CsfDeviceInformation(CsfDeviceInformation *pThis)
{
    if(pThis)
    {
        free_ApiMacDeviceDescriptor(pThis->devinfo);
        pThis->devinfo = NULL;
        free_ApiMacCapabilityInfo(pThis->capinfo);
        pThis->capinfo = NULL;
        free((void *)(pThis));
    }
}

/*
 * Convert a device list item
 * Public function defined in llc_proto.h
 */
CsfDeviceInformation *copy_Csf_deviceInformation(const Csf_deviceInformation_t *pThis)
{
    CsfDeviceInformation *p;

    p = calloc(1, sizeof(*p));
    if(p == NULL)
    {
        LOG_printf(LOG_ERROR, "No memory for: CsfDeviceInformation\n");
    }
    else
    {
        csf_device_information__init(p);
        p->devinfo = copy_ApiMac_deviceDescriptor(&(pThis->devInfo));
        p->capinfo = copy_ApiMac_capabilityInfo(&(pThis->capInfo));
        if((p->devinfo == NULL) || (p->capinfo == NULL))
        {
            free_CsfDeviceInformation(p);
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

