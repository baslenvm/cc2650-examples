#ifndef SEG7_H
#define SEG7_H
#define MAX_SEL 8

void seg7_init(char *mode, uint8_t bits, uint8_t sel_pin[], uint8_t pin[]);
void seg7_printf_hex(const char *val, ...);
void seg7_puts(const char *str);
void seg7_pos(uint8_t index);
void seg7_putplaten(uint8_t pattern);
void seg7_date(uint8_t index, uint8_t data);

#endif
