#ifndef __ONEWIRE_GPIO_H__
#define __ONEWIRE_GPIO_H__

void Delay_us(unsigned int T);
void Delay_ns(unsigned int T);
unsigned char ow_reset(void);
unsigned char read_bit(void);
void write_bit(char bitval);
unsigned char read_byte(void);
void write_byte(char val);
void Software_Reset(void);
int onewire_gpio_get_status(void);
bool onewire_gpio_is_ready(void);

#endif
