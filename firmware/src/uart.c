#include "uart.h"

/*==============================================================
  Initialisation USART2 sur PA2(TX) / PA3(RX)
==============================================================*/
void uart_init(uint32_t baud)
{
    /* 1. Activer clocks GPIOA et USART2 */
    RCC_AHB1ENR |= (1UL << 0);  /* GPIOA clock */
    RCC_APB1ENR |= (1UL << 17); /* USART2 clock */

    /* 2. Configurer PA2 et PA3 en Alternate Function (AF7) */
    /* MODER : bits 4-5 = PA2 AF, bits 6-7 = PA3 AF */
    GPIOA_MODER &= ~((3UL << 4) | (3UL << 6));
    GPIOA_MODER |= ((2UL << 4) | (2UL << 6));

    /* AFRL : AF7 (USART2) pour PA2 et PA3 */
    GPIOA_AFRL &= ~((0xFUL << 8) | (0xFUL << 12));
    GPIOA_AFRL |= ((7UL << 8) | (7UL << 12));

    /* 3. Configurer USART2 */
    USART2_CR1 = 0;             /* Reset          */
    USART2_BRR = baud;          /* Baud rate      */
    USART2_CR1 = USART_CR1_TE | /* TX enable      */
                 USART_CR1_RE | /* RX enable      */
                 USART_CR1_UE;  /* UART enable    */
}

/*==============================================================
  Envoyer un caractère
==============================================================*/
void uart_send_char(char c)
{
    while (!(USART2_SR & USART_SR_TXE))
        ;
    USART2_DR = (uint32_t)c;
}

/*==============================================================
  Envoyer une chaîne de caractères
==============================================================*/
void uart_send_string(const char *str)
{
    while (*str)
    {
        uart_send_char(*str++);
    }
}

/*==============================================================
  Envoyer un entier en décimal
==============================================================*/
void uart_send_int(int32_t value)
{
    char buf[12];
    int  i = 0;

    if (value < 0)
    {
        uart_send_char('-');
        value = -value;
    }

    if (value == 0)
    {
        uart_send_char('0');
        return;
    }

    while (value > 0)
    {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0)
    {
        uart_send_char(buf[--i]);
    }
}

/*==============================================================
  Recevoir un caractere (non bloquant)
  Retourne 0 si aucun caractère disponible
==============================================================*/
char uart_receive_char(void)
{
    if (!(USART2_SR & USART_SR_RXNE))
        return 0;  /*  Aucun caractere disponible */
    return (char)(USART2_DR & 0xFF);
}
