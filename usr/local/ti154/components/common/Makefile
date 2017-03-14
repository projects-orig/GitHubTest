#############################################################
# @file Makefile
#
# @brief TIMAC 2.0 Common Library Makefile
#
# Group: WCS LPC
# $Target Devices: Linux: AM335x, Embedded Devices: CC1310, CC1350$
#
#############################################################
# $License: BSD3 2016 $
#  
#   Copyright (c) 2015, Texas Instruments Incorporated
#   All rights reserved.
#  
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#  
#   *  Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#  
#   *  Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#  
#   *  Neither the name of Texas Instruments Incorporated nor the names of
#      its contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#  
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#############################################################
# $Release Name: TI-15.4Stack Linux x64 SDK$
# $Release Date: July 14, 2016 (2.00.00.30)$
#############################################################

# By default, we make the library file
_default: _libfile

# basic boiler plate makefile stuff
include ../../scripts/front_matter.mak

# What is the name of our library?
LIB_NAME=common


C_SOURCES_linux   += linux/linux_specific.c
C_SOURCES_linux   += linux/linux_uart.c

C_SOURCES_generic += src/debug_helpers.c
C_SOURCES_generic += src/fatal.c
C_SOURCES_generic += src/fifo.c
C_SOURCES_generic += src/hexline.c
C_SOURCES_generic += src/ini_file.c
C_SOURCES_generic += src/log.c
C_SOURCES_generic += src/log_ini.c
C_SOURCES_generic += src/mutex.c
C_SOURCES_generic += src/rand_data.c
C_SOURCES_generic += src/stream_common.c
C_SOURCES_generic += src/stream_file.c
C_SOURCES_generic += src/stream_mem.c
C_SOURCES_generic += src/stream_socket_client.c
C_SOURCES_generic += src/stream_socket_ini.c
C_SOURCES_generic += src/stream_socket_private.c
C_SOURCES_generic += src/stream_socket_server.c
C_SOURCES_generic += src/stream_uart_ini.c
C_SOURCES_generic += src/threads.c
C_SOURCES_generic += src/timer.c
C_SOURCES_generic += src/timer_cb.c
C_SOURCES_generic += src/ti_semaphore.c
C_SOURCES_generic += src/unix_fdrw.c

C_SOURCES += ${C_SOURCES_linux}
C_SOURCES += ${C_SOURCES_generic}


# And include the final library portion
include ../../scripts/library.mak

#  ========================================
#  Texas Instruments Micro Controller Style
#  ========================================
#  Local Variables:
#  mode: makefile-gmake
#  End:
#  vim:set  filetype=make

