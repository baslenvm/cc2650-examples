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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

uint8_t geti2c[3];
/*---------------------------------------------------------------------------*/
#define SCK IOID_10
#define DATA IOID_24

#define NO_ACK 0
#define ACK 1
//                           adr  command  r/w
#define STATUS_REG_W 0b0000110 // 000   0110    0
#define STATUS_REG_R 0b0000111 // 000   0111    1
#define MEASURE_TEMP 0b0000011 // 000   0011    1
#define MEASURE_HUMI 0b0000101 // 000   0101    1
#define RESET 0b0001111        // 000   1111    0

typedef union {
  unsigned int i;
  float f;
} valueTH;

/*---------------------------------------------------------------------------*/
#define CC2650_LAUNCHPAD_LOOP_INTERVAL (CLOCK_SECOND * 3)
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
static void gpio_write(uint32_t io, uint32_t val) {
  ti_lib_gpio_pin_write((1 << io), val);
}
/*---------------------------------------------------------------------------*/
static void config_gpio(char *restrict mode, uint32_t io) {
  if (strcmp(mode, "IN") == 0) {
    ti_lib_ioc_pin_type_gpio_input(io);
  } else if (strcmp(mode, "OUT") == 0) {
    ti_lib_ioc_pin_type_gpio_output(io);
  } else if (strcmp(mode, "CLEAR") == 0) {
    gpio_write(io, 0);
    ti_lib_ioc_int_clear(io);
  }
}
/*---------------------------------------------------------------------------*/
static uint8_t data_read(void) { return ti_lib_gpio_pin_read((1 << DATA)); }
/*---------------------------------------------------------------------------*/
static void gpio_init(void) {
  config_gpio("OUT", SCK);
  config_gpio("OUT", DATA);
}
/*---------------------------------------------------------------------------*/
void delay(unsigned int n) { clock_delay_usec(n); }
//----------------------------------------------------------------------------------
void s_transstart(void)
//----------------------------------------------------------------------------------
// generates a transmission start
//       _____         ________
// DATA:      |_______|
//           ___     ___
// SCK : ___|   |___|   |______
{
  gpio_write(DATA, 1);
  gpio_write(SCK, 1);
  delay(2);
  gpio_write(DATA, 0);
  delay(3);
  gpio_write(SCK, 0);
  delay(5);
  gpio_write(SCK, 1);
  delay(2);
  gpio_write(DATA, 1);
  delay(3);
  gpio_write(SCK, 0);
  delay(5);
}

//----------------------------------------------------------------------------------
void s_connectionreset(void)
//----------------------------------------------------------------------------------
// communication reset: DATA-line=1 and at least 9 SCK cycles followed by
// transstart
//       _____________________________________________________         ________
// DATA:                                                      |_______|
//          _    _    _    _    _    _    _    _    _        ___     ___
// SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
{
  unsigned char i;

  config_gpio("OUT", DATA);
  // PWR_ON();PWR = 1;

  gpio_write(DATA, 1);
  gpio_write(SCK, 0); // Initial state
  delay(5);
  for (i = 0; i < 9; i++) // 9 SCK cycles
  {
    gpio_write(SCK, 1);
    delay(5);
    gpio_write(SCK, 0);
    delay(5);
  }
}
//----------------------------------------------------------------------------------
bool s_write_byte(unsigned char value2)
//----------------------------------------------------------------------------------
// writes a byte on the Sensibus and checks the acknowledge
{
  unsigned char i;
  bool error;

  error = false;
  config_gpio("OUT", DATA);

  s_transstart(); // transmission start

  for (i = 0x80; i > 0; i >>= 1) // shift bit for masking
  {
    (i & value2)
        ? (gpio_write(DATA, 1))
        : (gpio_write(DATA, 0)); // masking value with i , write to SENSI-BUS
    gpio_write(SCK, 1);          // clk for SENSI-BUS
    delay(5);                    // pulswith approx. 5 us
    gpio_write(SCK, 0);
    delay(5);
  }
  // gpio_write(DATA, 1); // release DATA-line
  // delay(5);
  config_gpio("IN", DATA);
  gpio_write(SCK, 1); // clk #9 for ack
  delay(5);
  if (data_read() == 1) // check ack (DATA will be pulled down by SHT11)
    error = true;
  delay(5);
  gpio_write(SCK, 0);
  for (i = 0; i < 0xff00; i++) {
    if (data_read() == 1) // check ack (DATA will be pulled down by SHT11)
      return error;
  }
  error = true;
  return error; // error=1 in case of no acknowledge
}

//----------------------------------------------------------------------------------
unsigned char s_read_byte(unsigned char ack)
//----------------------------------------------------------------------------------
// reads a byte form the Sensibus and gives an acknowledge in case of "ack=1"
{
  unsigned char i, val;

  config_gpio("IN", DATA);
  val = 0;
  for (i = 0x80; i > 0; i >>= 1) // shift bit for masking
  {
    gpio_write(SCK, 1); // clk for SENSI-BUS
    delay(5);
    (data_read()) ? (val = (val | i)) : (val = (val | 0)); // read bit
    gpio_write(SCK, 0);
    delay(5);
  }
  config_gpio("OUT", DATA);
  gpio_write(DATA, !ack); // in case of "ack==1" pull down DATA-Line
  gpio_write(SCK, 1);     // clk #9 for ack
  delay(5);               // pulswith approx. 5 us
  gpio_write(SCK, 0);
  // gpio_write(DATA, 1); // release DATA-line
  delay(5);
  return val;
}
//----------------------------------------------------------------------------------
char s_read_statusreg(unsigned char *p_value, unsigned char *p_checksum)
//----------------------------------------------------------------------------------
// reads the status register with checksum (8-bit)
{
  unsigned char error = 0;
  s_transstart();                     // transmission start
  error = s_write_byte(STATUS_REG_R); // send command to sensor
  *p_value = s_read_byte(ACK);        // read status register (8-bit)
  *p_checksum = s_read_byte(NO_ACK);  // read checksum (8-bit)
  return error; // error=1 in case of no response form the sensor
}

//----------------------------------------------------------------------------------
char s_write_statusreg(unsigned char *p_value)
//----------------------------------------------------------------------------------
// writes the status register with checksum (8-bit)
{
  unsigned char error = 0;
  s_transstart();                      // transmission start
  error += s_write_byte(STATUS_REG_W); // send command to sensor
  error += s_write_byte(*p_value);     // send value of status register
  return error; // error>=1 in case of no response form the sensor
}

//----------------------------------------------------------------------------------
bool s_measure(unsigned char *p_value, unsigned char *p_checksum,
               char *restrict mode)
//----------------------------------------------------------------------------------
// makes a measurement (humidity/temperature) with checksum
{

  bool error;
  unsigned int i;

  // send command to sensor
  error = false;
  s_connectionreset();
  if (strcmp(mode, "TEMP")) {
    error |= s_write_byte(MEASURE_TEMP);
  } else if (strcmp(mode, "HUMI")) {
    error |= s_write_byte(MEASURE_HUMI);
  }
  if (error)
    return error;
  for (i = 0; i < 0xff00; i++) {
    delay(5);
    if (data_read() == 0)
      break; // wait until sensor has finished the measurement
  }

  if (data_read() == 1)
    error |= true;                   // or timeout (~2 sec.) is reached
  *(p_value + 1) = s_read_byte(ACK); // read the first byte (MSB)
  *(p_value) = s_read_byte(ACK);     // read the second byte (LSB)
  *p_checksum = s_read_byte(NO_ACK); // read checksum
  return error;
}
//----------------------------------------------------------------------------------------
void calc_sth1x(float *p_humidity, float *p_temperature)
//----------------------------------------------------------------------------------------
// calculates temperature and humidity [%RH]
// input :  humi [Ticks] (12 bit)
//          temp [Ticks] (14 bit)
// output:  humi [%RH]
//          temp
{
  const float C1 = -2.0468;       // for 12 Bit
  const float C2 = +0.0367;       // for 12 Bit
  const float C3 = -0.0000015955; // for 12 Bit
  const float T1 = +0.01;         // for 14 Bit @ 5V
  const float T2 = +0.00008;      // for 14 Bit @ 5V
  const float D1 = -39.66;        // for 14 Bit @ 5V
  const float D2 = 0.01;          // for 14 Bit DEGF

  float rh = *p_humidity;   // rh:      Humidity [Ticks] 12 Bit
  float t = *p_temperature; // t:       Temperature [Ticks] 14 Bit
  float rh_lin;             // rh_lin:  Humidity linear
  float rh_true;            // rh_true: Temperature compensated humidity
  float t_C;                // t_C   :  Temperature

  t_C = (t * D2) + D1; // calc. temperature from ticks to [°C]
  rh_lin =
      C1 + (C2 * rh) + (C3 * rh * rh); // calc. humidity from ticks to [%RH]
  rh_true = (t_C - 25) * (T1 + T2 * rh) +
            rh_lin; // calc. temperature compensated humidity [%RH]
  if (rh_true > 100)
    rh_true = 100; // cut if the value is outside of
  if (rh_true < 0.1)
    rh_true = 0.1; // the physical possible range

  *p_temperature = t_C;  // return temperature [°C]
  *p_humidity = rh_true; // return humidity[%RH]
}
/*---------------------------------------------------------------------------*/
static void temp_humi(void) {
  bool error;
  valueTH humi_val, temp_val;
  unsigned char checksum;
  float temp_cal = 0, humi_cal = 0;
  uint16_t humi_int = 0, temp_int = 0, temp_i_l = 0, temp_i_h = 0, humi_i_l = 0,
           humi_i_h = 0;

  error = false;
  error |= s_measure((unsigned char *)&temp_val.i, (unsigned char *)&checksum,
                     "TEMP");
  error |= s_measure((unsigned char *)&humi_val.i, (unsigned char *)&checksum,
                     "HUMI");

  if (!error) {
    humi_val.f = (float)humi_val.i;       // converts integer to float
    temp_val.f = (float)temp_val.i;       // converts integer to float
    calc_sth1x(&humi_val.f, &temp_val.f); // calculate humidity,temperature

    temp_cal = temp_val.f * 100;
    temp_int = ((int)temp_val.f) * 100;
    temp_i_l = (int)(temp_cal - temp_int);
    temp_i_h = (int)temp_val.f;

    humi_cal = humi_val.f * 100;
    humi_int = ((int)humi_val.f) * 100;
    humi_i_l = (int)(humi_cal - humi_int);
    humi_i_h = (int)humi_val.f;

    printf("T=%d.%d C\t", temp_i_h, temp_i_l);
    printf("H=%d.%d RH\r\n", humi_i_h, humi_i_l);
  }
}
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

  PROCESS_BEGIN();
  printf("\r\n");
  printf("CC26XX demo\r\n");

  SENSORS_ACTIVATE(batmon_sensor);

  /* Init the BLE advertisement daemon */
  rf_ble_beacond_config(0, BOARD_STRING);
  rf_ble_beacond_start();

  etimer_set(&et, CC2650_LAUNCHPAD_LOOP_INTERVAL);
  get_sync_sensor_readings();
  gpio_init();
  while (1) {

    PROCESS_YIELD();

    if (ev == PROCESS_EVENT_TIMER) {
      if (data == &et) {
        leds_toggle(CC2650_LAUNCHPAD_LEDS_PERIODIC);
        get_sync_sensor_readings();

        // delay(50);
        temp_humi();
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
