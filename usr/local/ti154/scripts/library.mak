#############################################################
# @file library.mak
#
# @brief TIMAC 2.0 Library Makefile fragment (Front portion)
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
# A detailed description is in "front_matter.mak"
#


#========================================
# Error checking
#----------------------------------------
# Make sure the libname was specified
ifeq (x${LIB_NAME}x,xx)
$(error LIB_NAME is not specified)
endif


# convert *.c in varous forms into objs/host/*.o
#             or objs/bbb/*.o
# 
# we also need to change "linux/*.c" to "*.c"
# and "src/*.c" to just "*.c"
# We do this in 2 steps
_1_C_SOURCES=${C_SOURCES:linux/%.c=%.c}
_2_C_SOURCES=${_1_C_SOURCES:src/%.c=%.c}
# Now convert
OBJFILES=${_2_C_SOURCES:%.c=${OBJDIR}/%.o}

#========================================
# What is the name of our lib file we are creating
# ie:  foo ->  objs/host/libfoo.a
# or           objs/bbb/libfoo.a
LIBFILE=${OBJDIR}/lib${LIB_NAME}.a

ifeq (${ARCH},all_arches)
# Are we building all
_libfile.all: ${ALL_ARCHES:%=_libfile.%}

# do this recursivly
_libfile.%:
	${MAKE} ARCH=$* _libfile
else
# the lib file generically
_libfile: ${LIBFILE}

# the actual file
${LIBFILE}: bbb_compiler_check generated_files ${OBJFILES} force 
	${HIDE}rm -f $@
	${HIDE}echo "Creating Lib: ${LIBFILE}"
	${HIDE}${AR_EXE} qc $@ ${OBJFILES}

clean::
	${HIDE}rm -f ${LIBFILE}

endif

#========================================
# We might have some uint test app
# build test apps, generically
_TEST_MAKEFILES=${wildcard *_test.mak}

testapps: ${_TEST_MAKEFILES:%=testapp_%}

testapp_%:
	${MAKE} -f $* ${MAKEFLAGS}



#  ========================================
#  Texas Instruments Micro Controller Style
#  ========================================
#  Local Variables:
#  mode: makefile-gmake
#  End:
#  vim:set  filetype=make

