#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#define RCC_BASE   0x40023800UL
#define GPIOD_BASE 0x40020C00UL

#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define GPIOD_MODER (*(volatile uint32_t *)(GPIOD_BASE + 0x00UL))
#define GPIOD_ODR   (*(volatile uint32_t *)(GPIOD_BASE + 0x14UL))

#define LED_GREEN  12U
#define LED_ORANGE 13U
#define LED_RED    14U
#define LED_BLUE   15U

void gpio_init(void);
void gpio_led_on(uint8_t pin);
void gpio_led_off(uint8_t pin);
void gpio_led_toggle(uint8_t pin);

#endif
