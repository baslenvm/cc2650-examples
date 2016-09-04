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

//#include "dht22.h"
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
  int16_t temperature, humidity;
  uint8_t i= 0,j = 255;
  PROCESS_BEGIN();
  printf("\r\n");
  printf("CC26XX demo\r\n");
  SENSORS_ACTIVATE(batmon_sensor);
  //SENSORS_ACTIVATE(dht22);
  /* Init the BLE advertisement daemon */
  rf_ble_beacond_config(0, BOARD_STRING);
  rf_ble_beacond_start();

  etimer_set(&et, CC2650_LAUNCHPAD_LOOP_INTERVAL);
  get_sync_sensor_readings();
  ti_lib_gpio_set_output_enable_dio(11,1);
  while (1) {
    while (1) {
          watchdog_periodic();//330ns
      ti_lib_gpio_toggle_dio(11);
    }
    PROCESS_YIELD();
    if (ev == PROCESS_EVENT_TIMER) {
      if (data == &et) {
        leds_toggle(CC2650_LAUNCHPAD_LEDS_PERIODIC);
        get_sync_sensor_readings();

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
