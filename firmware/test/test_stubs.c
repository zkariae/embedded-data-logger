/**
 * @file    test_stubs.c
 * @brief   Stubs pour les dépendances (systick, log, uart) lors des tests
 */

#include <stdint.h>

/* Stub pour delay_ms */
void delay_ms(uint32_t ms)
{
    /* Pas d'implémentation pour les tests unitaires */
    (void)ms;
}

/* Stubs pour uart (utilisé par log.h) */
void uart_send_string(const char *str)
{
    (void)str;
}

void uart_send_int(int val)
{
    (void)val;
}

/* Stub pour systick_init */
void systick_init(void)
{
}
