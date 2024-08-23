#include "lubancat_led_test.h"

void lubancat_led_control(int status)
{   
    /* 设置为输出*/
	*(uint32_t*)(GPIO1 | GPIO_SWPORTA_DDR) = 1 << 11;

    /* 读取当前寄存器值 */
    uint32_t current_value = *(uint32_t*)(GPIO1 | GPIO_SWPORTA_DR);

    if (status) {
        /* 设置第11位为1 */
        *(uint32_t*)(GPIO1 | GPIO_SWPORTA_DR) = current_value | (1 << 11);
        printf("led light off\n");
    } else {
        /* 清除第11位 */
        *(uint32_t*)(GPIO1 | GPIO_SWPORTA_DR) = current_value & ~(1 << 11);
        printf("led light on\n");
    }
}

