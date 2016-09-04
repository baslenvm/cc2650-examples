#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "batmon-sensor.h"
#include "board-peripherals.h"
#include "button-sensor.h"
#include "contiki.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "lib/sensors.h"
#include "random.h"
#include "rf-core/rf-ble.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "ti-lib.h"

#include "lcd.h"
/*---------------------------------------------------------------------------*/
#define CC2650_LAUNCHPAD_LOOP_INTERVAL (CLOCK_SECOND * 5)
#define CC2650_LAUNCHPAD_LEDS_PERIODIC LEDS_YELLOW
#define CC2650_LAUNCHPAD_LEDS_BUTTON LEDS_RED
#define CC2650_LAUNCHPAD_LEDS_REBOOT LEDS_ALL
/*---------------------------------------------------------------------------*/
#define CC2650_LAUNCHPAD_SENSOR_1 &button_left_sensor
#define CC2650_LAUNCHPAD_SENSOR_2 &button_right_sensor
/*---------------------------------------------------------------------------*/
static struct etimer et;
/*---------------------------------------------------------------------------*/
PROCESS(cc26xx_demo_process, "cc26xx demo process");
AUTOSTART_PROCESSES(&cc26xx_demo_process);
/*---------------------------------------------------------------------------*/
static void get_sync_sensor_readings(void) {
  int value;

  printf("-----------------------------------------\r\n");

  value = batmon_sensor.value(BATMON_SENSOR_TYPE_TEMP);
  printf("Bat: Temp=%d C\r\n", value);

  value = batmon_sensor.value(BATMON_SENSOR_TYPE_VOLT);
  printf("Bat: Volt=%d mV\r\n", (value * 125) >> 5);

  return;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cc26xx_demo_process, ev, data) {
int fd;
  PROCESS_BEGIN();
  printf("\r\n");
  printf("CC26XX demo\r\n");
  SENSORS_ACTIVATE(batmon_sensor);

  /* Init the BLE advertisement daemon */
  rf_ble_beacond_config(0, BOARD_STRING);
  rf_ble_beacond_start();

  etimer_set(&et, CC2650_LAUNCHPAD_LOOP_INTERVAL);
  get_sync_sensor_readings();

  //int  lcdInit (int rows, int cols, int bits, int rs, int strb,
  //      int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7) ;
  fd = lcdInit (2, 16, 4,  11,12 , 25,26,27,28,0,0,0,0) ;
  while (1) {
    PROCESS_YIELD();
    if (ev == PROCESS_EVENT_TIMER) {
      if (data == &et) {
        leds_toggle(CC2650_LAUNCHPAD_LEDS_PERIODIC);
        get_sync_sensor_readings();
        lcdPrintf(fd,"hello, world!");

        etimer_set(&et, CC2650_LAUNCHPAD_LOOP_INTERVAL);
      }
    } else if (ev == sensors_event) {
      if (data == CC2650_LAUNCHPAD_SENSOR_1) {
        printf(
            "Left: Pin %d, press duration %d clock ticks\n",
            (CC2650_LAUNCHPAD_SENSOR_1)->value(BUTTON_SENSOR_VALUE_STATE),
            (CC2650_LAUNCHPAD_SENSOR_1)->value(BUTTON_SENSOR_VALUE_DURATION));

        if ((CC2650_LAUNCHPAD_SENSOR_1)->value(BUTTON_SENSOR_VALUE_DURATION) >
            CLOCK_SECOND) {
          printf("Long button press!\n");
        }
        leds_toggle(CC2650_LAUNCHPAD_LEDS_BUTTON);
      } else if (data == CC2650_LAUNCHPAD_SENSOR_2) {
        leds_on(CC2650_LAUNCHPAD_LEDS_REBOOT);
        watchdog_reboot();
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
