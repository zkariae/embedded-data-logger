#ifndef UART_H
#define UART_H

#include <stdint.h>

/*==============================================================
  Adresses des registres USART2 (utilise les mocks si TEST_HOST)
  PA2 = TX, PA3 = RX
==============================================================*/
#ifdef TEST_HOST
#include "mock_hw.h"
#else
#define RCC_BASE    0x40023800UL
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

#define GPIOA_BASE  0x40020000UL
#define GPIOA_MODER (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRL  (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

#define USART2_BASE 0x40004400UL
#define USART2_SR   (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_DR   (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR  (*(volatile uint32_t *)(USART2_BASE + 0x08UL))
#define USART2_CR1  (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))
#endif /* TEST_HOST */

/* Bits USART_SR */
#define USART_SR_TXE  (1UL << 7) /* TX buffer empty    */
#define USART_SR_TC   (1UL << 6) /* Transmission complete */
#define USART_SR_RXNE (1UL << 5) /* RX not empty       */

/* Bits USART_CR1 */
#define USART_CR1_UE (1UL << 13) /* UART enable        */
#define USART_CR1_TE (1UL << 3)  /* Transmit enable    */
#define USART_CR1_RE (1UL << 2)  /* Receive enable     */

/* Baud rate @ 16MHz HSI */
#define UART_BAUD_9600   0x0683UL /* 16MHz / 9600       */
#define UART_BAUD_115200 0x008BUL /* 16MHz / 115200     */

/*==============================================================
  Interface publique
==============================================================*/
void uart_init(uint32_t baud);
void uart_send_char(char c);
void uart_send_string(const char *str);
void uart_send_int(int32_t value);
char uart_receive_char(void);

#endif /* UART_H */
