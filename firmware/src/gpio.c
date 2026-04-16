#include "gpio.h"

void gpio_init(void)
{
    RCC_AHB1ENR |= (1UL << 3);
    RCC_AHB1ENR |= (1UL << 3); /* double lecture pour stabiliser */
    GPIOD_MODER &= ~(0xFF000000UL);
    GPIOD_MODER |= (0x55000000UL);
}

void gpio_led_on(uint8_t pin)
{
    GPIOD_ODR |= (1UL << pin);
}

void gpio_led_off(uint8_t pin)
{
    GPIOD_ODR &= ~(1UL << pin);
}

void gpio_led_toggle(uint8_t pin)
{
    GPIOD_ODR ^= (1UL << pin);
}
