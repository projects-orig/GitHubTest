/******************************************************************************

 @file sblUart.c

 @brief CC13xx Bootloader Platform Specific UART functions

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

/*********************************************************************
 * INCLUDES
 */
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static int serialPortFd;
static struct termios tioOld;
/*********************************************************************
 * API FUNCTIONS
 */

int SblUart_open(const char *devicePath)
{
    struct termios tio;
    unsigned int rtn;

    /* open the device */
    serialPortFd = open(devicePath, O_RDWR | O_NOCTTY);
    if (serialPortFd < 0)
    {
        perror(devicePath);
        printf("sblUartOpen: %s open failed\n", devicePath);
        return (-1);
    }

    rtn = tcgetattr(serialPortFd, &tioOld);
    if(rtn == -1)
    {
        printf("sblUartOpen: tcgetattr error: %x\n", rtn);
        return (-1);
    }

    /* set board rate */
    rtn = cfsetspeed(&tio, B1500000);
    if(rtn == -1)
    {
        printf("sblUartOpen: cfsetspeed error: %x\n", rtn);
        return (-1);
    }

    /* set raw mode:
        tio->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                        | INLCR | IGNCR | ICRNL | IXON);
        tio->c_oflag &= ~OPOST;
        tio->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        tio->c_cflag &= ~(CSIZE | PARENB);
        tio->c_cflag |= CS8;
    */
    cfmakeraw(&tio);

    /* Make it block for 200ms */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 2;

    tcflush(serialPortFd, TCIFLUSH);
    rtn = tcsetattr(serialPortFd, TCSANOW, (const struct termios *) (&tio));
    if(rtn == -1)
    {
        printf("sblUartOpen: tcsetattr error: %x\n", rtn);
        return (-1);
    }

    return 0;
}

void SblUart_close(void)
{
    unsigned int rtn;

    tcflush(serialPortFd, TCOFLUSH);

    rtn = tcsetattr(serialPortFd, TCSANOW, (const struct termios *) (&tioOld));
    if(rtn == -1)
    {
        printf("SblUart_close: tcsetattr error: %x\n", rtn);
    }

    close(serialPortFd);

    return;
}

void SblUart_write(const unsigned char* buf, size_t len)
{
    write (serialPortFd, buf, len);
    tcflush(serialPortFd, TCOFLUSH);

    return;
}

unsigned char SblUart_read(unsigned char* buf, size_t len)
{
    unsigned char ret = read(serialPortFd, buf, len);

    return (ret);
}
