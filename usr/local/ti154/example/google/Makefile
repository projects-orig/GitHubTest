#############################################################
# @file Makefile
#
# @brief TIMAC 2.0 "google protobuf" makefile
#
# This makefile can download, build and install the google
# protobuf compiler and the "protobuf-c" extension
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

#----------------------------------------
# NOTE:
#  This was tested/documented Thu Mar 31 16:15:04 PDT 2016
#
#  The current GIT commits where:
#     protobuf-c: 006d69bd8481738323c58af5cd4c8477d09eb9f5
#     protobuf:   a25e9964b126a79e8a99bced2207ddca22d6a1a9
#
# Our (TI) expectation is, this will continue to work with
# the current (head) version of the protobuf tools.
# 
# However, if something goes wrong - you can always revert to
# this 'known-good-state' by using the above date/time and/or
# by using last known GIT commit IDs.
#
#----------------------------------------

default:
	@echo "Please use 'make all' to build everything"
	@echo "Or make <something>.protobuf"
	@echo "Or make <something>.proto-c"
	@echo ""
	@echo "Where <something> is one of: checkout autogen config build install"
	@echo ""
	@exit 1

HERE := $(shell pwd)

PREFIX=${HERE}/install

checkout.protobuf:
	git clone https://github.com/google/protobuf.git

autogen.protobuf:
	cd ./protobuf && sh ./autogen.sh

config.protobuf:
	cd ./protobuf && ./configure --prefix=$(PREFIX) --enable-static=yes --enable-shared=no

build.protobuf:
	cd ./protobuf && make -j 2

install.protobuf:
	cd ./protobuf && make -j 2 install

all.protobuf: \
	checkout.protobuf autogen.protobuf config.protobuf build.protobuf install.protobuf


checkout.proto-c:
	git clone https://github.com/protobuf-c/protobuf-c.git

autogen.proto-c:
	cd protobuf-c && ./autogen.sh

configure.proto-c:
	cd protobuf-c && PKG_CONFIG_PATH=${PREFIX}/lib/pkgconfig ./configure --prefix=${PREFIX} --enable-shared=no --enable-static=yes
build.proto-c:
	cd protobuf-c && make -j 2

install.proto-c:
	cd protobuf-c && make -j 2 install


all.proto-c: \
	checkout.proto-c autogen.proto-c configure.proto-c build.proto-c install.proto-c

all: all.protobuf all.proto-c

only.clone:  checkout.protobuf  checkout.proto-c

dist-clean:
	rm -rf install
	rm -rf protobuf
	rm -rf protobuf-c

#  ========================================
#  Texas Instruments Micro Controller Style
#  ========================================
#  Local Variables:
#  mode: makefile-gmake
#  End:
#  vim:set  filetype=make

