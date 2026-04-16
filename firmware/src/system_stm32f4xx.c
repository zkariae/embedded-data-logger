
#include <stdint.h>

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)
#define SCB_VTOR  (*(volatile uint32_t *)0xE000ED08UL)

void SystemInit(void)
{
    SCB_CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
    SCB_VTOR = 0x08000000UL;
}
