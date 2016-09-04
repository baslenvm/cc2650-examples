#ifndef SHT11_ARCH_H
#define SHT11_ARCH_H
#include "contiki-conf.h"
#include "ti-lib.h"
// Architecture-specific definitions for the SHT11 sensor on Zolertia Z1
// when connected to the Ziglet port.
// CAUTION: I2C needs to be disabled to use the bitbang protocol of SHT11

#define SHT11_ARCH_IOID_SDA 4 /* IOID_4 */
#define SHT11_ARCH_IOID_SCL 5 /* IOID_5 */
// SHT11_ARCH_PWR is not needed, but until and *-arch abstraction exists, this
// should fix it

#define SDA_0()                                                                \
  (ti_lib_ioc_pin_type_gpio_output(SHT11_ARCH_IOID_SDA),                       \
   ti_lib_gpio_write_dio(SHT11_ARCH_IOID_SDA,0)) /* SDA Output=0 */
#define SDA_1()                                                                \
  (ti_lib_ioc_pin_type_gpio_output(SHT11_ARCH_IOID_SDA),                       \
   ti_lib_gpio_write_dio(SHT11_ARCH_IOID_SDA,1)) /* SDA Output=1 */
#define SDA_IS_1 SDA_IN()                    /* SDA Input */

#define SCL_0()                                                                \
  (ti_lib_ioc_pin_type_gpio_output(SHT11_ARCH_IOID_SCL),                       \
   ti_lib_gpio_write_dio(SHT11_ARCH_IOID_SCL,0)) /* SCL Output=0 */
#define SCL_1()                                                                \
  (ti_lib_ioc_pin_type_gpio_output(SHT11_ARCH_IOID_SCL),                       \
   ti_lib_gpio_write_dio(SHT11_ARCH_IOID_SCL,1))

static unsigned int SDA_IN(void) {
  ti_lib_ioc_pin_type_gpio_input(SHT11_ARCH_IOID_SDA);
  return ti_lib_gpio_read_dio(SHT11_ARCH_IOID_SDA);
}
#define delay_400ns() CPUdelay(1);//__asm("mov r0,r0");
#endif
