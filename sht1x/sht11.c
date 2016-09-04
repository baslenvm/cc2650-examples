/*
 * Copyright (c) 2007, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Device driver for the Sensirion SHT1x/SHT7x family of humidity and
 * temperature sensors.
 */

#include "contiki.h"
#include "dev/sht11/sht11.h"
#include "sht11-arch.h"
#include <stdio.h>

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* adr   command  r/w */
#define STATUS_REG_W 0x06 /* 000    0011    0 */
#define STATUS_REG_R 0x07 /* 000    0011    1 */
#define MEASURE_TEMP 0x03 /* 000    0001    1 */
#define MEASURE_HUMI 0x05 /* 000    0010    1 */
#define RESET 0x1e        /* 000    1111    0 */

/*---------------------------------------------------------------------------*/
static void sstart(void) {
  SDA_1();
  SCL_0();
  delay_400ns();
  SCL_1();
  delay_400ns();
  SDA_0();
  delay_400ns();
  SCL_0();
  delay_400ns();
  SCL_1();
  delay_400ns();
  SDA_1();
  delay_400ns();
  SCL_0();
}
/*---------------------------------------------------------------------------*/
static void sreset(void) {
  int i;
  SDA_1();
  SCL_0();
  for (i = 0; i < 9; i++) {
    SCL_1();
    delay_400ns();
    SCL_0();
    delay_400ns();
  }
  sstart(); /* Start transmission, why??? */
}
/*---------------------------------------------------------------------------*/
/*
 * Return true if we received an ACK.
 */
static int swrite(unsigned _c) {
  unsigned char c = _c;
  int i;
  int ret;

  for (i = 0; i < 8; i++, c <<= 1) {
    if (c & 0x80) {
      SDA_1();
    } else {
      SDA_0();
    }
    SCL_1();
    delay_400ns();
    SCL_0();
    delay_400ns();
  }

  SDA_1();
  SCL_1();
  delay_400ns();
  ret = !SDA_IS_1;

  SCL_0();

  return ret;
}
/*---------------------------------------------------------------------------*/
static unsigned sread(int send_ack) {
  int i;
  unsigned char c = 0x00;

  SDA_1();
  for (i = 0; i < 8; i++) {
    c <<= 1;
    SCL_1();
    delay_400ns();
    if (SDA_IS_1) {
      c |= 0x1;
    }
    SCL_0();
    delay_400ns();
  }

  if (send_ack) {
    SDA_0();
  }
  SCL_1();
  delay_400ns();
  SCL_0();

  SDA_1(); /* Release SDA */

  return c;
}
/*---------------------------------------------------------------------------*/
/*
 * Power up the device. The device can be used after an additional
 * 11ms waiting time.
 */
void sht11_init(void) {
/*
 * SCL Output={0,1}
 * SDA 0: Output=0
 *     1: Input and pull-up (Output=0)
 */
#ifdef SHT11_INIT
  SHT11_INIT();
#else
/* As this driver is bit-bang based, disable the I2C first
   This assumes the SDA/SCL pins passed in the -arch.h file are
   actually the same used for I2C operation, else comment out the following
*/
/*SHT11_PxSEL &= ~(BV(SHT11_ARCH_SDA) | BV(SHT11_ARCH_SCL));
#if defined(__MSP430_HAS_MSP430X_CPU__) || defined(__MSP430_HAS_MSP430XV2_CPU__)
  SHT11_PxREN &= ~(BV(SHT11_ARCH_SDA) | BV(SHT11_ARCH_SCL));
#endif*/

/* Configure SDA/SCL as GPIOs */
/*SHT11_PxOUT |= BV(SHT11_ARCH_PWR);
SHT11_PxOUT &= ~(BV(SHT11_ARCH_SDA) | BV(SHT11_ARCH_SCL));
SHT11_PxDIR |= BV(SHT11_ARCH_PWR) | BV(SHT11_ARCH_SCL);*/
#endif
}
/*---------------------------------------------------------------------------*/
/*
 * Power of device.
 */
void sht11_off(void) {
#ifdef SHT11_OFF
  SHT11_OFF();
#else
/*SHT11_PxOUT &= ~BV(SHT11_ARCH_PWR);
SHT11_PxOUT &= ~(BV(SHT11_ARCH_SDA) | BV(SHT11_ARCH_SCL));
SHT11_PxDIR |= BV(SHT11_ARCH_PWR) | BV(SHT11_ARCH_SCL);*/
#endif
}
/*---------------------------------------------------------------------------*/
/*
 * Only commands MEASURE_HUMI or MEASURE_TEMP!
 */
static unsigned int scmd(unsigned cmd) {
  unsigned int n;

  if (cmd != MEASURE_HUMI && cmd != MEASURE_TEMP) {
    PRINTF("Illegal command: %d\n", cmd);
    return -1;
  }

  //sstart(); /* Start transmission */
  sreset();
  if (!swrite(cmd)) {
    PRINTF("SHT11: scmd - swrite failed\n");
    goto fail;
  }

  for (n = 0; n < 4000000; n++) {
    if (!SDA_IS_1) {
      unsigned t0, t1, rcrc;
      t0 = sread(1);
      t1 = sread(1);
      rcrc = sread(0);
      PRINTF("SHT11: scmd - read %d, %d\n", t0, t1);
      return (t0 << 8) | t1;
    }
    /* short wait before next loop */
    clock_wait(2);
  }
fail:
  sreset();
  return -1;
}
/*---------------------------------------------------------------------------*/
/*
 * Call may take up to 210ms.
 */
unsigned int sht11_temp(void) { return scmd(MEASURE_TEMP); }
/*---------------------------------------------------------------------------*/
/*
 * Call may take up to 210ms.
 */
unsigned int sht11_humidity(void) { return scmd(MEASURE_HUMI); }
/*---------------------------------------------------------------------------*/
#if 1 /* But ok! */
unsigned sht11_sreg(void) {
  unsigned sreg, rcrc;

  sstart(); /* Start transmission */
  if (!swrite(STATUS_REG_R)) {
    goto fail;
  }

  sreg = sread(1);
  rcrc = sread(0);

  return sreg;

fail:
  sreset();
  return -1;
}
#endif
/*---------------------------------------------------------------------------*/
#if 0
int
sht11_set_sreg(unsigned sreg)
{
  sstart();			/* Start transmission */
  if(!swrite(STATUS_REG_W)) {
    goto fail;
  }
  if(!swrite(sreg)) {
    goto fail;
  }

  return 0;

 fail:
  sreset();
  return -1;
}
#endif
/*---------------------------------------------------------------------------*/
#if 0
int
sht11_reset(void)
{
  sstart();			/* Start transmission */
  if(!swrite(RESET)) {
    goto fail;
  }

  return 0;

 fail:
  sreset();
  return -1;
}
#endif
/*---------------------------------------------------------------------------*/
