/*
 * lcd.c:
 *	Text-based LCD driver.
 *	This is designed to drive the parallel interface LCD drivers
 *	based in the Hitachi HD44780U controller and compatables.
 *
 * Copyright (c) 2012 Gordon Henderson.
 ***********************************************************************
 * This file is part of wiringPi:
 *	https://projects.drogon.net/raspberry-pi/wiringpi/
 *
 *    wiringPi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published
 *by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    wiringPi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with wiringPi.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "lib/memb.h"
#include "ti-lib.h"

#include "seg7.h"

const uint8_t SEG7_PATTERN[] = {
    0b11111110, 0b01100000, 0b11011010, 0b11110010, 0b01100110, 0b10110110,
    0b10111110, 0b11100000, 0b11111110, 0b11110110, 0b11101110, 0b00111110,
    0b10011100, 0b01111010, 0b10011110, 0b10001110, 0b00000000, 0b00000001};
const char SEG7_OUTPUT[] = "0123456789abcdef .";
struct seg7_struct {
  char *mode;
  uint8_t bits;
  uint8_t sel_pin[MAX_SEL];
  uint8_t pin[8];
  uint8_t data[MAX_SEL];
};
MEMB(seg7_structs, struct seg7_struct, 1);
struct seg7_struct *seg7;
void seg7_init(char *mode, uint8_t bits, uint8_t sel_pin[], uint8_t pin[]) {
  int8_t i;
  seg7 = memb_alloc(&seg7_structs);
  seg7->mode = mode;
  seg7->bits = bits;
  i = sizeof(sel_pin);
  while (i >= 0) {
    seg7->sel_pin[i] = sel_pin[i];
    i--;
  }
  i = bits;
  while (i >= 0) {
    seg7->pin[i] = pin[i];
    i--;
  }
  i = MAX_SEL;
  while (i >= 0) {
    seg7->data[i] = 0b00000000;
    i--;
  }
}

void seg7_printf_hex(const char *val, ...) {
  va_list argp;
  char buffer[17];

  va_start(argp, val);
  vsnprintf(buffer, 17, val, argp);
  va_end(argp);

  seg7_puts(buffer);
}

void seg7_puts(const char *str) {
  int i = 0, index = 0;
  char *pch;
  if (seg7->bits == 8) {
    while (*str) {
      pch = strchr(SEG7_OUTPUT, *str++);
      if (pch != NULL) {
        index = pch - SEG7_OUTPUT;
        if (index != 17) {
          seg7->data[i++] = SEG7_PATTERN[index];
        } else {
          seg7->data[i - 1] |= SEG7_PATTERN[index];
        }
        if (((!strcmp(seg7->mode, "pin")) &&
             (index == sizeof(seg7->sel_pin))) ||
            ((!strcmp(seg7->mode, "bit")) &&
             (index == pow(2, sizeof(seg7->sel_pin))))) {
          index = 0;
        }
      }
    }
  } else if (seg7->bits == 4) {
    while (*str) {
      seg7->data[i++] = *str++;
    }
  }
}

void seg7_date(uint8_t index, uint8_t data) { seg7->data[index] = data; }

void seg7_putplaten(uint8_t pattern) {
  printf("%u\r\n", seg7->data[0]);
  clock_wait(1);
}
