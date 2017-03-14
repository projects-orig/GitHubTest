#!/bin/bash
#############################################################
# @file build_all.sh
#
# @brief TIMAC 2.0 Build All script
#
# This shell script will "build the world" everything in order.
# It accepts a few parameters, examples:
#
#    bash$ sh ./build_all.sh              <- defaults to host build
#    bash$ sh ./build_all.sh host         <- host only
#    bash$ sh ./build_all.sh bbb          <- bbb target
#    bash$ sh ./build_all.sh clean        <- erases everything
#    bash$ sh ./build_all.sh remake       <- 'clean' and then 'all'
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


function usage()
{
	echo "usage $0 TARGET"
	echo ""
	echo "Where TARGET is one of:  host, bbb"
	echo ""
	echo "The BBB options require appropriate tools"
	echo "be made in `pwd`/scripts/front_matter.mak"
	echo "And in `pwd`/example/cc13xx-sbl/app/linux/Makefile"
	echo ""
}

case $# in
0)
	target=host
	;;
1)
	target=$1
	;;
*)
	echo "Wrong number of parameters, $#"
	usage
	exit 1
	;;
esac

case $target in
clean)
	# ok
	;;
host)
	# ok
	;;
bbb)
	# ok
	;;
klockwork)
	# ok
	;;
remake)
	# ok
	;;
*)
	echo "Unknown target: $target"
	usage
	exit 1
esac

# this is a simple script...
# We die/exit if there was a problem
set -e

script -e -f -c "cd components/common && make $target"  $target.common.log

script -e -f -c "cd components/nv     && make $target"  $target.nv.log

script -e -f -c "cd components/api    && make $target"  $target.api.log

script -e -f -c "cd example/npi_server2 && make $target" $target.npi_server2.log

script -e -f -c "cd example/collector && make $target" $target.collector.log

script -e -f -c "cd example/cc13xx-sbl/app/linux && make $target" $target.bootloader.log

#  ========================================
#  Texas Instruments Micro Controller Style
#  ========================================
#  Local Variables:
#  mode: sh
#  End:
#  vim:set  filetype=sh

