#include "gpio.h"
#include "systick.h"
#include "uart.h"
#include "log.h"



int main(void)
{

    gpio_init();
    systick_init();
    uart_init(UART_BAUD_115200);

    LOG_INFO("Hello from main(void)");

    while (1)
    {

        gpio_led_on(LED_GREEN);
        LOG_INFO("GREEN ON");
        delay_ms(500);
        gpio_led_off(LED_GREEN);
        delay_ms(500);

        gpio_led_on(LED_ORANGE);
        LOG_INFO("ORANGE ON");
        delay_ms(500);
        gpio_led_off(LED_ORANGE);
        delay_ms(500);

        gpio_led_on(LED_RED);
        LOG_INFO("RED ON");
        delay_ms(500);
        gpio_led_off(LED_RED);
        delay_ms(500);

        gpio_led_on(LED_BLUE);
        LOG_INFO("BLUE ON");
        delay_ms(500);
        gpio_led_off(LED_BLUE);
        delay_ms(500);
    }
}

