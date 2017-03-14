#############################################################
# @file front_matter.mak
#
# @brief TIMAC 2.0 Front/Boilerplate Makefile fragment
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

#========================================
#
# ===================
# Theory of Operation
# ===================
#
# This is a *makefile* fragment that works with two other files
# If you are creating a library, you use "library.mak"
# If you are creating an application, you use "app.mak"
#
# In total there are 5 parts to the complete makefile.
#
# Part #1 
#     The local makefile local to the application.
#     Another name for this is the "Primary Makefile"
#     When you invoke "make" the filename is "Makefile"
#     If you invoke "make -f FooBar" the filename is FooBar
#
#     That makefile must define APP_NAME, the name of the app.
#     Likewise LIB_NAME for a library
#
#     Early in that makefile, it does "include front_matter.mak"
#
#     APP Example: ${root}/example/collector/Makefile
#     LIB Example: ${root}/components/common/Makefile
#
# Part #2
#     Is exactly *THIS* makefile, the front or starting common stuff.
#     These are various rules, common macros etc
#
#     For example the macro: OBJDIR is set in front_matter.mak
# 
#     Another example: this file defines how to compile C to OBJ files.
#
# Part #3
#     Is not a seperate makefile, is the *middle* section of the 
#     primary makefile, it is between the two statements:
#          'include front_matter.mak'
#     and: 'include app.mak'
#      or: 'include library.mak'
#  
#     This part lists the various source filenames
#     Possibly other things that are specific to the app or library
#     
#     For example the collector makefile has rules to compile protobuf files.
#
# Part #4
#     Is one of the "app.mak" or "library.mak" file.
#     If you build an app, you include app.mak
#     if you build a library, you include library.mak
#
#     The "app.mak" file defines how to link object files into an appliation.
#     Likewise, the "library.mak" combines multiple object files into a library 
#
# Part #5
#     Is the final portion of the primary Makefile
#     Specifically this is the part after the 'include app.mak' or 'include library.mak'
#
#     In this area, the library or application creates any additional rules
#

#========================================
# if HIDE is not defined then define it.
# to use this:
#     bash$  make  HIDE= -f Makefile
# or  bash$  make  HIDE= -f foo_test.mak
#
HIDE ?= @
#========================================

# Forcably use BASH, do not use /bin/dash
SHELL=/bin/bash
export SHELL

#========================================
# Help recursion, determine main makefilename
#----------------------------------------
# Determine the name of the 'main' makefile
main_MAKEFILE := ${firstword ${MAKEFILE_LIST}}
# If we call make recursively, we use name of the first makefile
recursive_MAKE := ${MAKE} -f ${main_MAKEFILE}

#========================================
# These *MAY* need to be adjusted to fit your SDK install.
# NOTE: The Boot loader also has a macro that may need updating.
# SEE: ${root}/example/cc13xx-sbl/app/linux/Makefile
#----------------------------------------
# What is the Processor SDK Root directory
#----------------------------------------
#bbb_TI_PROC_SDK_DIR=${HOME}/ti-processor-sdk-linux-am335x-evm-03.00.00.01
#bbb_TI_PROC_SDK_DIR=${HOME}/ti-processor-sdk-linux-am335x-evm-02.00.02.11
bbb_TI_PROC_SDK_DIR=${HOME}/ti-processor-sdk-linux-rt-am335x-evm-03.00.00.04

#----------------------------------------
# Hint/Example: Make a symbolic link like this:
#      bash$ cd $HOME
#      bash$ rm -f cur_sdk
#      bash$ ln -s ti-processor-sdk-linux-am335x-evm-02.00.10.01 cur_sdk
# Thus:
#    $HOME/cur_sdk -> $HOME/ti-processor-sdk-linux-am335x-evm-02.00.10.01
#----------------------------------------
# Within the Processor SDK, we find the "bin" directory
# this is where all the GNU tools are located
bbb_BIN_DIR=${bbb_TI_PROC_SDK_DIR}/linux-devkit/sysroots/x86_64-arago-linux/usr/bin
# And we use this prefix to build the target.
#bbb_PREFIX=${bbb_BIN_DIR}/arm-linux-gnueabihf-
bbb_PREFIX=/usr/bin/arm-linux-gnueabihf-

#
# Note you could add others here if required..

#========================================
# Handle various ARCH configurations
#----------------------------------------
ALL_ARCHES        += bbb
bbb_ARCH          := bbb
bbb_GCC_EXE       := ${bbb_PREFIX}gcc 
bbb_ARCH_CFLAGS   += -mcpu=cortex-a8 -march=armv7-a -ffunction-sections
bbb_AR_EXE        := ${bbb_PREFIX}ar
bbb_OBJDUMP_EXE   := ${bbb_PREFIX}objdump
bbb_ARCH_LDFLAGS  += -Wl,--gc-sections

# Sanity check, is the compiler present? Y or N?
bbb_compiler_check:
	@if [ "x${ARCH}x" == "xbbbx" ] ; \
	then \
		if [ ! -x ${bbb_GCC_EXE} ] ; \
		then \
			echo "Cannot find BBB Compiler: ${bbb_GCC_EXE}" ; \
			echo "Please update variable: bbb_TI_PROC_SDK_DIR" ; \
			echo "in the file SDK_ROOT/scripts/front_matter.mak" ; \
			echo "ERROR: Cannot continue" ; \
			exit 1 ; \
		fi ; \
	fi

.PHONY:: bbb_compiler_check

ALL_ARCHES         += host
host_ARCH          := host
host_GCC_EXE       := gcc
host_AR_EXE        := ar
host_OBJDUMP_EXE   := objdump
host_ARCH_LDFLAGS  += -Wl,--gc-sections
host_ARCH_CFLAGS   += -ffunction-sections

ALL_ARCHES         += klockwork
klockwork_ARCH     := klockwork
KWRAP_OUTPUT       ?= `pwd`/objs/klockwork/kwrap.output


klockwork_GCC_EXE       := kwwrap -o ${KWRAP_OUTPUT} gcc
klockwork_AR_EXE        := kwwrap -o ${KWRAP_OUTPUT} ar
klockwork_OBJDUMP_EXE   := objdump
klockwork_ARCH_LDFLAGS  += -Wl,--gc-sections
klockwork_ARCH_CFLAGS   += -ffunction-sections

# Add others here if required

#========================================
# Command line goals, the main make target
#---------------------------------------
# What is the first target word on the cmdline?
_word_zero  := $(firstword $(MAKECMDGOALS))
# What is the rest of the command line goals?
_word_rest  := $(wordlist 2,999,$(MAKECMDGOALS))

#========================================
# Do we build the default ARCH?
# If so... the target is the host version only!
#----------------------------------------
ifeq (${_word_zero},)
_word_zero := host
endif

ARCH      := ${${_word_zero}_ARCH}

#----------------------------------------
# if ARCH is blank.. define to all arches
ifeq (x${ARCH}x,xx)
ARCH      = all_arches
endif

#----------------------------------------
# Select GCC for the all arch situation
# We use /bin/false because these will
# simulate an error
all_arches_GCC_EXE  	:= /bin/false
all_arches_AR_EXE       := /bin/false
all_arches_ARCH_CFLAGS  :=


#========================================
# Where do we put things? & Select final names.
#----------------------------------------
OBJDIR       := objs/${ARCH}
GCC_EXE      := ${${ARCH}_GCC_EXE}
AR_EXE       := ${${ARCH}_AR_EXE}
OBJDUMP_EXE  := ${${ARCH}_OBJDUMP_EXE}
ARCH_CFLAGS  := ${${ARCH}_ARCH_CFLAGS}
ARCH_LDFLAGS := ${${ARCH}_ARCH_LDFLAGS}

#========================================
# Default targets for bbb and host targets

host:
	${recursive_MAKE} ARCH=host ${_word_rest}

bbb:
	${recursive_MAKE} ARCH=bbb ${_word_rest}

klockwork:
	${recursive_MAKE} ARCH=klockwork ${_word_rest}

# Clean and rebuild.
remake:
	${recursive_MAKE} ARCH=${ARCH} clean
	${recursive_MAKE} 

${OBJDIR}:
	mkdir -p $@

#========================================
# Compliation rules
#----------------------------------------
# FUTURE: Add C++ rules, and ASM rules

CFLAGS_STRICT= -Wshadow -Wpointer-arith -Wcast-qual 
CFLAGS +=-Wall -Wmissing-prototypes -Wstrict-prototypes -Iinc 

# Compile
#  NOTE: "-MMD" and "-MF" create compiler dependancy files
#        Which we load below with an include statement.
${OBJDIR}/%.o: %.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -c ${CFLAGS} ${ARCH_CFLAGS} -MP -MMD -MF ${OBJDIR}/${*F}.d -o ${OBJDIR}/${@F} $<

${OBJDIR}/%.o: linux/%.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -c ${CFLAGS} ${ARCH_CFLAGS} -MP -MMD -MF ${OBJDIR}/${*F}.d -o ${OBJDIR}/${@F} $<


${OBJDIR}/%.o: src/%.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -c ${CFLAGS} ${ARCH_CFLAGS} -MP -MMD -MF ${OBJDIR}/${*F}.d -o ${OBJDIR}/${@F} $<

# Preprocess only
${OBJDIR}/%.i: %.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -E ${CFLAGS} ${ARCH_CFLAGS} -o ${@D} $<

# Preprocess only
${OBJDIR}/%.i: linux/%.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -E ${CFLAGS} ${ARCH_CFLAGS} -o ${@D} $<

# Preprocess only
${OBJDIR}/%.i: src/%.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -E ${CFLAGS} ${ARCH_CFLAGS} -o ${@D} $<


# Generate assembler
${OBJDIR}/%.s: %.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -S ${CFLAGS} ${ARCH_CFLAGS} -o ${@D} $<

${OBJDIR}/%.s: linux/%.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -S ${CFLAGS} ${ARCH_CFLAGS} -o ${@D} $<

${OBJDIR}/%.s: src/%.c
	${HIDE}mkdir -p ${OBJDIR}
	${HIDE}echo "Compling: $<"
	${HIDE}${GCC_EXE} -g -S ${CFLAGS} ${ARCH_CFLAGS} -o ${@D} $<


#========================================
# Load any generated dependancies
-include ${OBJDIR}/*.d

generated_files::

.PHONY:: generated_files

#========================================
# Debug, determine the value of macro like this:
# 
#  bash$  make  whatis_OBJDIR
#  bash$  make  whatis_GCC_EXE

whatis_%:
	@echo ""
	@echo "Var:  $*  => ${${*}}"
	@echo ""

#========================================
# Simple target to force somethings to happen
force:

.PHONY:: force

#========================================
# How do we cleanup or mess?
#----------------------------------------
ifeq (${ARCH},all_arches)
clean:: ${ALL_ARCHES:%=clean.%}
	${HIDE}rm -f *.bak
	${HIDE}rm -f *~
	${HIDE}echo "Clean done"

clean.%:
	${recursive_MAKE} ARCH=$* clean
else
clean::
	${HIDE}echo "Cleaning: ${OBJDIR} ..."
	${HIDE}rm -rf ${OBJDIR}

endif

#  ========================================
#  Texas Instruments Micro Controller Style
#  ========================================
#  Local Variables:
#  mode: makefile-gmake
#  End:
#  vim:set  filetype=make

