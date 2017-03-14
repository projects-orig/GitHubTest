/******************************************************************************
 @file linux_main.c

 @brief TIMAC 2.0 API this is the "main" file for the linux collector app

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

#include "compiler.h"
#include "appsrv.h"
#include "api_mac.h"
#include "api_mac_linux.h"
#include "mt_msg_dbg.h"
#include "ini_file.h"       /* this reads our ini file */
#include "log.h"            /* our logging scheme */
#include "timer.h"
#include "fatal.h"
#include "stream.h"
#include "stream_socket.h"  /* we use a socket in our app */
#include "stream_uart.h"    /* and a uart. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cllc.h"
#include "nvintf.h"
#include "nv_linux.h"
#include "config.h"

int linux_CONFIG_TRANSMIT_POWER = CONFIG_TRANSMIT_POWER_DEFAULT;

int linux_CONFIG_PHY_ID = CONFIG_PHY_ID_DEFAULT;
int linux_CONFIG_CHANNEL_PAGE = CONFIG_CHANNEL_PAGE_DEFAULT;
uint8_t linux_CONFIG_FH_CHANNEL_MASK[APIMAC_154G_CHANNEL_BITMAP_SIZ]= CONFIG_FH_CHANNEL_MASK_DEFAULT;
uint8_t linux_CONFIG_CHANNEL_MASK[APIMAC_154G_CHANNEL_BITMAP_SIZ] = CONFIG_CHANNEL_MASK_DEFAULT;
uint8_t linux_FH_ASYNC_CHANNEL_MASK[APIMAC_154G_CHANNEL_BITMAP_SIZ] = FH_ASYNC_CHANNEL_MASK_DEFAULT;

uint8_t linux_CONFIG_LINKQUALITY   = CONFIG_LINKQUALITY_DEFAULT;
uint8_t linux_CONFIG_PERCENTFILTER = CONFIG_PERCENTFILTER_DEFAULT;
uint8_t linux_CONFIG_SCAN_DURATION = CONFIG_SCAN_DURATION_DEFAULT;
char linux_CONFIG_FH_NETNAME[32] = CONFIG_FH_NETNAME_DEFAULT;
int linux_CONFIG_DWELL_TIME = CONFIG_DWELL_TIME_DEFAULT;
int linux_CONFIG_TRICKLE_MIN_CLK_DURATION = CONFIG_TRICKLE_MIN_CLK_DURATION_DEFAULT;
int linux_CONFIG_TRICKLE_MAX_CLK_DURATION = CONFIG_TRICKLE_MAX_CLK_DURATION_DEFAULT;
int linux_CONFIG_FH_PAN_SIZE = CONFIG_FH_PAN_SIZE_DEFAULT;
bool linux_CONFIG_DOUBLE_TRICKLE_TIMER = CONFIG_DOUBLE_TRICKLE_TIMER_DEFAULT;
bool linux_CONFIG_AUTO_START = CONFIG_AUTO_START_DEFAULT;
bool linux_CONFIG_SECURE = CONFIG_SECURE_DEFAULT;
int  linux_CONFIG_PAN_ID = CONFIG_PAN_ID_DEFAULT;
bool linux_CONFIG_FH_ENABLE = CONFIG_FH_ENABLE_DEFAULT;
int  linux_CONFIG_COORD_SHORT_ADDR = CONFIG_COORD_SHORT_ADDR_DEFAULT;
int  linux_CONFIG_MAC_BEACON_ORDER = CONFIG_MAC_BEACON_ORDER_DEFAULT;
int  linux_CONFIG_MAC_SUPERFRAME_ORDER = CONFIG_MAC_SUPERFRAME_ORDER_DEFAULT;

/*!
 * Called from the linux config file parser as each channel mask is parsed
 * from the configuration file. This allows the user to override/set
 * the channel list from the configuration file.
 */

static int do_mask( struct ini_parser *pINI, uint8_t *var, size_t var_size )
{
    struct ini_numlist nl;
    int n;
    int mask;
    int idx;
    
    /* Clear the existing */
    memset( (void *)(var), 0, var_size );

    if( 0 == strcmp( "all", pINI->item_value ) ){
        memset( (void *)(var), 0x0ff, var_size );
        return 0;
    }

    if( 0 == strcmp( "none", pINI->item_value ) ){
        memset( (void *)(var), 0x00, var_size );
        return 0;
    }
    n = 0;
    
    /* parse numbers... */
    INI_valueAsNumberList_init(&nl, pINI);
    while(INI_valueAsNumberList_next(&nl) != EOF)
    {
        n++;
        idx = nl.value / 8;
        mask = (1 << (nl.value % 8));
        if( idx >= var_size ){
            INI_syntaxError(pINI, "invalid-bitmask-bit: %d\n", nl.value);
            return -1;
        }
        var[ idx ] |= mask;
    }
    if( n == 0 )
    {
        INI_syntaxError( pINI, "no bits?\n");
    }
    if( nl.is_error ){
        return -1;
    }
    // success
    return 0;
}

        

/*
 * These are application specific log flags.
 * See "log.h" for more details.
 *
 * These log flags and others (see log_flag_names below)
 * make up the log flags that can be controled via the cfg file.
 */
const struct ini_flag_name app_log_flags[] = {

    { .name = "appsrv-connections", .value = LOG_APPSRV_CONNECTIONS },
    { .name = "appsrv-broadcasts",  .value = LOG_APPSRV_BROADCAST   },
    { .name = "appsrv-msg-content", .value = LOG_APPSRV_MSG_CONTENT },
    {.name = NULL }
};

/*
 * Debug log flags.
 */
const struct ini_flag_name * const log_flag_names[] = {
    /* See log.h */
    log_builtin_flag_names,
    /* see mt_msg.h */
    mt_msg_log_flags,
    /* see above */
    app_log_flags,
    /* see nv_linux.h */
    nv_log_flags,
    /* See api_mac_linux.h */
    api_mac_log_flags,
    /* terminate */
    NULL
};

/*!
 * @brief Handle any config file settings for the uart.
 *
 * @param pINI - ini file parse info
 * @param handled - set to true if the item was handled
 * @return 0 success, -1 error
 */
static int my_UART_INI_settings(struct ini_parser *pINI, bool *handled)
{
    int r;

    r = 0;
    if(INI_itemMatches(pINI, "uart-cfg", NULL))
    {
        r = UART_INI_settingsOne(pINI, handled, &uart_cfg);
    }
    return r;
}

/*
 * @brief Handle socket settings for our two sockets.
 *
 * @param pINI - ini file parse info
 * @param handled - set to true if the item was handled
 * @return 0 success, -1 error
 */
static int my_SOCKET_INI_settings(struct ini_parser *pINI, bool *handled)
{
    int r;

    r = 0;
    if(INI_itemMatches(pINI, "npi-socket-cfg", NULL))
    {
        r = SOCKET_INI_settingsOne(pINI, handled, &npi_socket_cfg);
    }
    if(INI_itemMatches(pINI, "appClient-socket-cfg", NULL))
    {
        r = SOCKET_INI_settingsOne(pINI, handled, &appClient_socket_cfg);
    }
    return r;
}

/*
 * @brief Handle msg interface settings for our interfaces.
 *
 * @param pINI - ini file parse info
 * @param handled - set to true if the item was handled
 * @return 0 success, -1 error
 */
static int my_MT_MSG_INI_settings(struct ini_parser *pINI, bool *handled)
{
    int r;

    r = 0;
    if(INI_itemMatches(pINI, "appClient-socket-interface", NULL))
    {
        r = MT_MSG_INI_settings(pINI, handled, &appClient_mt_interface_template);
    }

    if(INI_itemMatches(pINI, "npi-socket-interface", NULL))
    {
        r = MT_MSG_INI_settings(pINI, handled, &npi_mt_interface);
    }
    if(INI_itemMatches(pINI, "uart-interface", NULL))
    {
        r = MT_MSG_INI_settings(pINI, handled, &uart_mt_interface);
    }
    return r;
}

/*
 * @brief Handle application level settings.
 *
 * @param pINI - ini file parse info
 * @param handled - set to true if the item was handled
 * @return 0 success, -1 error
 */
static int my_APP_settings(struct ini_parser *pINI, bool *handled)
{
    /* we only deal with application level settings. */
    if(pINI->item_name == NULL)
    {
        /* we don't care about the section line. */
        return 0;
    }
    if(!INI_itemMatches(pINI, "application", NULL))
    {
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "api-mac-areq-timeout"))
    {
        ApiMacLinux_areq_timeout_mSecs = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "interface"))
    {
        if(0 == strcmp("socket", pINI->item_value))
        {
            API_MAC_msg_interface = &npi_mt_interface;
            *handled = true;
            return 0;
        }
        if(0 == strcmp("uart", pINI->item_value))
        {
            API_MAC_msg_interface = &uart_mt_interface;
            *handled = true;
            return 0;
        }
    }

    if(INI_itemMatches(pINI, NULL, "load-nv-sim"))
    {
        linux_CONFIG_NV_RESTORE = INI_valueAsBool(pINI);
        *handled = true;
        return 0;
    }

    if( INI_itemMatches(pINI,NULL, "config-tx-power" ) ){
        linux_CONFIG_TRANSMIT_POWER = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-phy-id"))
    {
        linux_CONFIG_PHY_ID = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-linkquality"))
    {
        linux_CONFIG_LINKQUALITY = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-scan-duration"))
    {
        linux_CONFIG_SCAN_DURATION = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-percentfilter"))
    {
        linux_CONFIG_PERCENTFILTER = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-channel-page"))
    {
        linux_CONFIG_CHANNEL_PAGE = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-fh-netname"))
    {
        INI_dequote(pINI);
        strcpy( linux_CONFIG_FH_NETNAME, pINI->item_value );
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-dwell-time"))
    {
        linux_CONFIG_DWELL_TIME = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI,NULL, "config-trickle-min-clk-duration"))
    {
        linux_CONFIG_TRICKLE_MIN_CLK_DURATION = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if( INI_itemMatches(pINI,NULL, "config-trickle-max-clk-duration") ){
        linux_CONFIG_TRICKLE_MAX_CLK_DURATION = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if( INI_itemMatches(pINI, NULL, "config-double-trickle-timer" ) ){
        linux_CONFIG_DOUBLE_TRICKLE_TIMER = INI_valueAsBool(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-secure"))
    {
        linux_CONFIG_SECURE = INI_valueAsBool(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-fh-enable"))
    {
        linux_CONFIG_FH_ENABLE = INI_valueAsBool(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-pan-id"))
    {
        linux_CONFIG_PAN_ID = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-fh-pan-size"))
    {
        linux_CONFIG_FH_PAN_SIZE = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-coord-short-addr"))
    {
        linux_CONFIG_COORD_SHORT_ADDR = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if( INI_itemMatches( pINI, NULL, "config-mac-beacon-order"))
    {
        linux_CONFIG_MAC_BEACON_ORDER = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if( INI_itemMatches( pINI, NULL, "config-mac-superframe-order" ))
    {
        linux_CONFIG_MAC_SUPERFRAME_ORDER = INI_valueAsInt(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI, NULL, "config-channel-mask"))
    {
        *handled = true;
        return do_mask( pINI, 
                        (&linux_CONFIG_CHANNEL_MASK[0]),
                        sizeof(linux_CONFIG_CHANNEL_MASK));
    }


    if(INI_itemMatches(pINI, NULL, "config-fh-async-channel-mask"))
    {
        *handled = true;
        return do_mask( pINI, 
                        (&linux_FH_ASYNC_CHANNEL_MASK[0]),
                        sizeof(linux_FH_ASYNC_CHANNEL_MASK));
    }

    if(INI_itemMatches(pINI, NULL, "config-fh-channel-mask"))
    {
        *handled = true;
        return do_mask( pINI, 
                        (&linux_CONFIG_FH_CHANNEL_MASK[0]),
                        sizeof(linux_CONFIG_FH_CHANNEL_MASK));
    }
    

    if(INI_itemMatches(pINI, NULL, "config-auto-start"))
    {
        linux_CONFIG_AUTO_START = INI_valueAsBool(pINI);
        *handled = true;
        return 0;
    }

    if(INI_itemMatches(pINI,NULL,"msg-dbg-data"))
    {
        struct mt_msg_dbg **ppDbg;

        /* append at end of list */
        ppDbg = &(ALL_MT_MSG_DBG);
        while( *ppDbg ){
            ppDbg = &((*ppDbg)->m_pNext);
        }
        INI_dequote(pINI);
        *ppDbg = MT_MSG_dbg_load(pINI->item_value);
        *handled = true;
        return 0;
    }

    return 0;
}

/* Callback for parsing the INI file. */
static int cfg_callback(struct ini_parser *pINI, bool *handled)
{
    int x;
    int r;

    static ini_rd_callback * const ini_cb_table[] = {
        LOG_INI_settings,
        my_UART_INI_settings,
        my_SOCKET_INI_settings,
        my_MT_MSG_INI_settings,
        my_APP_settings,
        /* Terminate list */
        NULL
    };

    for(x = 0 ; ini_cb_table[x] ; x++)
    {
        r = (*(ini_cb_table[x]))(pINI, handled);
        if(*handled)
        {
            return r;
        }
    }
    /* let the system handle it */
    return 0;
}

/* Our main... */
int main(int argc, char **argv)
{
    int r;
    int x;
    char *cfg_filenames[3];

    if( argc == 1 )
    {
        /* use default config filename */
        cfg_filenames[0] = argv[0];
        cfg_filenames[1] = "collector.cfg";
        cfg_filenames[2] = NULL;

        argc = 2;
        argv = cfg_filenames;
    }

    /* Basic initialization */
    SOCKET_init();
    STREAM_init();
    TIMER_init();
    LOG_init("/dev/stderr");
    /* we want these logs to begin with */
    log_cfg.log_flags = LOG_FATAL | LOG_WARN | LOG_ERROR;

    APP_defaults();

    /* Read all configuration files */
    for( x = 1 ; x < argc ; x++ ){
        r = INI_read(argv[x], cfg_callback, 0);
        if(r != 0)
        {
            FATAL_printf("Failed to read cfg file\n");
        }
    }

    /* And away we go! */
    APP_main();

    exit(0);
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
