#include <stdio.h>

#define GPIO1 0x03021000
#define GPIO_SWPORTA_DR 0x000
#define GPIO_SWPORTA_DDR 0x004

void lubancat_led_control(int status);