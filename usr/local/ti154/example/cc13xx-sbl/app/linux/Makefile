_default: host
#/******************************************************************************
#
# @file Makefile
#
# @brief CC13xx Bootloader Example Application
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

SBU_REV= "0.1"

INCLUDE = -I$(PROJ_DIR)../../cc13xxdnld -I$(PROJ_DIR)../../platform/linux 

klockwork:
	rm -f ./kw.output
	${MAKE} KLOCKWORK=true remake host

ifdef KLOCKWORK
HERE := $(shell pwd)
CC_HOST= kwwrap -o ${HERE}/kw.output gcc
else
CC_HOST= gcc
endif

#----------------------------------------
# Location of the BBB compiler, the Processor SDK
#----------------------------------------
#BBB_SDK=${HOME}/ti-processor-sdk-linux-am335x-evm-03.00.00.01
#BBB_SDK=${HOME}/ti-processor-sdk-linux-am335x-evm-02.00.02.11
BBB_SDK=${HOME}/ti-processor-sdk-linux-rt-am335x-evm-03.00.00.04
#----------------------------------------
# Hint/Example: Make a symbolic link like this:
#      bash$ cd $HOME
#      bash$ rm -f cur_sdk
#      bash$ ln -s ti-processor-sdk-linux-am335x-evm-02.00.10.01 cur_sdk
# Thus:
#    $HOME/cur_sdk -> $HOME/ti-processor-sdk-linux-am335x-evm-02.00.10.01
#----------------------------------------
# Within the Processor SDK, we find the compiler
#BBB_PREFIX=${BBB_SDK}/linux-devkit/sysroots/x86_64-arago-linux/usr/bin/arm-linux-gnueabihf-
BBB_PREFIX=/usr/bin/arm-linux-gnueabihf-
CC_BBB = ${BBB_PREFIX}gcc


# Sanity check, is the compiler present? Y or N?
bbb_compiler_check:
	@if [ ! -x ${CC_BBB} ] ; \
	then \
		echo "ERROR: Cannot find BBB Compiler: ${CC_BBB}" ; \
		echo "ERROR: Please update variable: BBB_PREFIX"  ; \
		echo "in the file SDK_ROOT/cc13xx-sbl/app/linux/Makefile"; \
		echo "ERROR: Cannot continue" ; \
		exit 1 ; \
	fi

.PHONY:: bbb_compiler_check 

CFLAGS= -c -Wall -g -std=gnu99
PROJ_DIR=

OBJ_HOST=objs/host
OBJ_BBB =objs/bbb

bbb: bbb_cc13xx-sbl
host: host_cc13xx-sbl

.PHONY:: bbb host

all: bbb_cc13xx-sbl host_cc13xx-sbl

.PHONY:: all

.PHONY:: bbb_cc13xx-sbl host_cc13xx-sbl

${OBJ_HOST}:
	mkdir -p $@
${OBJ_BBB}:
	mkdir -p $@

host_cc13xx-sbl: ${OBJ_HOST}/main.o ${OBJ_HOST}/cc13xxdnld.o ${OBJ_HOST}/sblUart.o
	$(CC_HOST) -o host_cc13xx-sbl ${OBJ_HOST}/main.o ${OBJ_HOST}/cc13xxdnld.o ${OBJ_HOST}/sblUart.o

# rule for file "cc13xxdnld.o".
${OBJ_HOST}/main.o: main.c ${OBJ_HOST}
	$(CC_HOST) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)main.c -o $@

# rule for file "cc13xxdnld.o".
${OBJ_HOST}/cc13xxdnld.o: ../../cc13xxdnld/cc13xxdnld.c  ${OBJ_HOST}
	$(CC_HOST) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)../../cc13xxdnld/cc13xxdnld.c  -o $@

# rule for file "rpcTransport.o".
${OBJ_HOST}/sblUart.o: sblUart.h sblUart.c  ${OBJ_HOST}
	$(CC_HOST) $(CFLAGS) $(INCLUDE) $(DEFS) sblUart.c -o $@

bbb_cc13xx-sbl: bbb_compiler_check ${OBJ_BBB}/main.o ${OBJ_BBB}/cc13xxdnld.o ${OBJ_BBB}/sblUart.o 
	$(CC_BBB) -o bbb_cc13xx-sbl ${OBJ_BBB}/main.o ${OBJ_BBB}/cc13xxdnld.o ${OBJ_BBB}/sblUart.o

# rule for file "cc13xxdnld.o".
${OBJ_BBB}/main.o: main.c ${OBJ_BBB}
	$(CC_BBB) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)main.c  -o $@

# rule for file "cc13xxdnld.o".
${OBJ_BBB}/cc13xxdnld.o: ../../cc13xxdnld/cc13xxdnld.c ${OBJ_BBB}
	$(CC_BBB) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)../../cc13xxdnld/cc13xxdnld.c  -o $@

# rule for file "rpcTransport.o".
${OBJ_BBB}/sblUart.o: sblUart.h sblUart.c ${OBJ_BBB}
	$(CC_BBB) $(CFLAGS) $(INCLUDE) $(DEFS) sblUart.c  -o $@

# rule for cleaning files generated during compilations.
clean:
	/bin/rm -f host_* bbb_* 
	/bin/rm -rf objs

remake: clean _default

