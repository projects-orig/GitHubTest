/******************************************************************************

 @file config.h

 @brief configuration parameters for TIMAC 2.0

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
#ifndef CONFIG_H
#define CONFIG_H

/******************************************************************************
 Includes
 *****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 Constants and definitions
 *****************************************************************************/
/* config parameters */

/*
   NOTE ABOUT CONFIGURATION PARAMTERS
   ----------------------------------
   In the embedded device, these are hard coded configuration items
   In the Linux impimentation the are configurable in 2 ways.
   Method #1 via hard coding with the _DEFAULT value.
   Method #2 via the "appsrv.cfg" configuration file.
   This "extern bool" hidden via the macro exists to facilitate
   the linux configuration scheme.
 */

/*! Should the newtwork auto start or not? */
extern bool linux_CONFIG_AUTO_START;
#define CONFIG_AUTO_START   linux_CONFIG_AUTO_START
#define CONFIG_AUTO_START_DEFAULT true

/*! Security Enable - set to true to turn on security */
extern bool linux_CONFIG_SECURE;
#define CONFIG_SECURE                linux_CONFIG_SECURE
#define CONFIG_SECURE_DEFAULT        true
/*! PAN ID */
extern int linux_CONFIG_PAN_ID;
#define CONFIG_PAN_ID                ((uint16_t)(linux_CONFIG_PAN_ID))
#define CONFIG_PAN_ID_DEFAULT        0xffff

/*! Coordinator short address */
extern int linux_CONFIG_COORD_SHORT_ADDR;
#define CONFIG_COORD_SHORT_ADDR      ((uint16_t)(linux_CONFIG_COORD_SHORT_ADDR))
#define CONFIG_COORD_SHORT_ADDR_DEFAULT 0xAABB
/*! FH disabled as default */

extern bool linux_CONFIG_FH_ENABLE;
#define CONFIG_FH_ENABLE             linux_CONFIG_FH_ENABLE
#define CONFIG_FH_ENABLE_DEFAULT     false

/*! maximum beacons possibly received */
#define CONFIG_MAX_BEACONS_RECD      200

/*! link quality */
extern uint8_t linux_CONFIG_LINKQUALITY;
#define CONFIG_LINKQUALITY           linux_CONFIG_LINKQUALITY
#define CONFIG_LINKQUALITY_DEFAULT  1

/*! percent filter */
extern uint8_t linux_CONFIG_PERCENTFILTER;
#define CONFIG_PERCENTFILTER         linux_CONFIG_PERCENTFILTER
#define CONFIG_PERCENTFILTER_DEFAULT 0xFF

/*! scan duration */
extern uint8_t linux_CONFIG_SCAN_DURATION;
#define CONFIG_SCAN_DURATION         linux_CONFIG_SCAN_DURATION
#define CONFIG_SCAN_DURATION_DEFAULT 5

/*! maximum devices in association table */
#define CONFIG_MAX_DEVICES           50

/*!
 Setting beacon order to 15 will disable the beacon, 8 is a good value for
 beacon mode
 */
extern int linux_CONFIG_MAC_BEACON_ORDER;
#define CONFIG_MAC_BEACON_ORDER      linux_CONFIG_MAC_BEACON_ORDER
#define CONFIG_MAC_BEACON_ORDER_DEFAULT 15

/*!
 Setting superframe order to 15 will disable the superframe, 6 is a good value
 for beacon mode
 */
extern int linux_CONFIG_MAC_SUPERFRAME_ORDER;
#define CONFIG_MAC_SUPERFRAME_ORDER  linux_CONFIG_MAC_SUPERFRAME_ORDER
#define CONFIG_MAC_SUPERFRAME_ORDER_DEFAULT 15

/*! Setting for channel page */
extern int linux_CONFIG_CHANNEL_PAGE ;
#define CONFIG_CHANNEL_PAGE_DEFAULT          (APIMAC_CHANNEL_PAGE_9)
#define CONFIG_CHANNEL_PAGE                  linux_CONFIG_CHANNEL_PAGE

/*! Setting for Phy ID */
extern int linux_CONFIG_PHY_ID;
#define CONFIG_PHY_ID (linux_CONFIG_PHY_ID)
#define CONFIG_PHY_ID_DEFAULT                (APIMAC_STD_US_915_PHY_1)

/*! Setting Default Key*/
#define KEY_TABLE_DEFAULT_KEY \
    {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,                    \
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
/*!
 Channel mask - Each bit indicates if the corresponding channel is to be
 scanned First byte represents channel 0 to 7 and the last byte represents
 channel 128 to 135
 In FH Mode: Represents the list of channels excluded from hopping
 It is a bit string with LSB representing Ch0
 e.g., 0x01 0x10 represents Ch0 and Ch12 are excluded
 Currently same mask is used for Unicast and Broadcast hopping sequence

 NOTE: this is the default channel mask,

    In the linux impliementation the INI file parser callback
    uses a a function to *clear/zero* the mask, and another
    function to set various bits within the channel mask.
 */
extern uint8_t linux_CONFIG_CHANNEL_MASK[APIMAC_154G_CHANNEL_BITMAP_SIZ];
#define CONFIG_CHANNEL_MASK_DEFAULT   { 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00, 0x00, 0x00 }
/*!
 Channel mask used when CONFIG_FH_ENABLE is true.
 Represents the list of channels on which the device can hop.
 The actual sequence used shall be based on DH1CF function.
 It is represented as a bit string with LSB representing Ch0.
 e.g., 0x01 0x10 represents Ch0 and Ch12 are included.
 */
extern uint8_t linux_CONFIG_FH_CHANNEL_MASK[APIMAC_154G_CHANNEL_BITMAP_SIZ]; 
#define CONFIG_FH_CHANNEL_MASK_DEFAULT { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

/*!
 List of channels to target the Async frames
 It is represented as a bit string with LSB representing Ch0
 e.g., 0x01 0x10 represents Ch0 and Ch12 are included
 It should cover all channels that could be used by a target device in its
 hopping sequence. Channels marked beyond number of channels supported by
 PHY Config will be excluded by stack. To avoid interference on a channel,
 it should be removed from Async Mask and added to exclude channels
 (CONFIG_CHANNEL_MASK).
 */
extern uint8_t linux_FH_ASYNC_CHANNEL_MASK[APIMAC_154G_CHANNEL_BITMAP_SIZ];
#define FH_ASYNC_CHANNEL_MASK_DEFAULT                                   \
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,                               \
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,                         \
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

/* FH related config variables */
/*! Dwell time */
extern int linux_CONFIG_DWELL_TIME;
#define CONFIG_DWELL_TIME            linux_CONFIG_DWELL_TIME
#define CONFIG_DWELL_TIME_DEFAULT    250

/* The mimum trickle timer window for PAN Advertisement ,
* and PAN Configuration frame transmissions
* Default is 0.5 minute. Recommended to set this to
* half of PAS/PCS MIN Timer */
extern int linux_CONFIG_TRICKLE_MIN_CLK_DURATION;
#define CONFIG_TRICKLE_MIN_CLK_DURATION    linux_CONFIG_TRICKLE_MIN_CLK_DURATION
#define CONFIG_TRICKLE_MIN_CLK_DURATION_DEFAULT 30000

/* The maximum trickle timer window for PAN Advertisement ,
 * and PAN Configuration frame transmissions
 * Default is 16 minutes */
extern int linux_CONFIG_TRICKLE_MAX_CLK_DURATION;
#define CONFIG_TRICKLE_MAX_CLK_DURATION    linux_CONFIG_TRICKLE_MAX_CLK_DURATION
#define CONFIG_TRICKLE_MAX_CLK_DURATION_DEFAULT 960000

/* default value for PAN Size PIB */
extern int linux_CONFIG_FH_PAN_SIZE;
#define CONFIG_FH_PAN_SIZE             linux_CONFIG_FH_PAN_SIZE
#define CONFIG_FH_PAN_SIZE_DEFAULT     0x0032

/* To enable Doubling of PA/PC trickle time,
 * useful when network has non sleepy nodes and
 * thre is a requirement to use PA/PC to convey updated
 * PAN information */
extern bool linux_CONFIG_DOUBLE_TRICKLE_TIMER;
#define CONFIG_DOUBLE_TRICKLE_TIMER    linux_CONFIG_DOUBLE_TRICKLE_TIMER
#define CONFIG_DOUBLE_TRICKLE_TIMER_DEFAULT false

/*! value for ApiMac_FHAttribute_netName */
extern char linux_CONFIG_FH_NETNAME[32];
#define CONFIG_FH_NETNAME            linux_CONFIG_FH_NETNAME
#define CONFIG_FH_NETNAME_DEFAULT    {"FHTest"}

/*!
 Value for Transmit Power in dBm
 Default value is 14, allowed values are any value
 between 0 dBm and 14 dBm in 1 dB increments, and -10 dBm
 When the nodes in the network are close to each other
 lowering this value will help reduce saturation */
extern int linux_CONFIG_TRANSMIT_POWER;
#define CONFIG_TRANSMIT_POWER   linux_CONFIG_TRANSMIT_POWER
#define CONFIG_TRANSMIT_POWER_DEFAULT        14


#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */

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

