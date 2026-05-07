/**
 * @file    mock_hw.c
 * @brief   Implémentation de la simulation des registres STM32 pour tests unitaires
 */

#include "mock_hw.h"
#include <stdio.h>

/* Initialisation des registres simulés */
volatile uint32_t mock_RCC_AHB1ENR = 0;
volatile uint32_t mock_RCC_APB1ENR = 0;
volatile uint32_t mock_GPIOB_MODER = 0;
volatile uint32_t mock_GPIOB_OTYPER = 0;
volatile uint32_t mock_GPIOB_OSPEEDR = 0;
volatile uint32_t mock_GPIOB_PUPDR = 0;
volatile uint32_t mock_GPIOB_AFRL = 0;
volatile uint32_t mock_GPIOB_ODR = 0;
volatile uint32_t mock_I2C1_CR1 = 0;
volatile uint32_t mock_I2C1_CR2 = 0;
volatile uint32_t mock_I2C1_DR = 0;
volatile uint32_t mock_I2C1_SR1 = 0;
volatile uint32_t mock_I2C1_SR2 = 0;
volatile uint32_t mock_I2C1_CCR = 0;
volatile uint32_t mock_I2C1_TRISE = 0;

/* Variables pour simuler le comportement I2C */
static uint8_t i2c_rx_msb = 0;
static uint8_t i2c_rx_lsb = 0;
static int i2c_byte_count = 0;

void mock_i2c_reset(void)
{
    mock_I2C1_CR1 = 0;
    mock_I2C1_CR2 = 0;
    mock_I2C1_DR = 0;
    mock_I2C1_SR1 = 0;
    mock_I2C1_SR2 = 0;
    mock_I2C1_CCR = 0;
    mock_I2C1_TRISE = 0;
    i2c_byte_count = 0;
}

void mock_i2c_set_lux_data(uint8_t msb, uint8_t lsb)
{
    i2c_rx_msb = msb;
    i2c_rx_lsb = lsb;
}

void mock_i2c_simulate_ack(void)
{
    /* ACK est le comportement normal, rien à faire */
}

void mock_i2c_simulate_nack(void)
{
    /* Pour simuler NACK, on ne set pas ADDR ou on génère une erreur */
    /* On va simplement ne pas mettre à jour SR1 */
}

/* Implémentation des fonctions mockées pour I2C */

int mock_i2c_start(void)
{
    /* Simuler START : mettre SB (Start Bit) */
    mock_I2C1_SR1 |= (1UL << 0);  /* SB flag */
    return 0;  /* OK */
}

void mock_i2c_stop(void)
{
    /* Simuler STOP : clear START et mettre STOP */
    mock_I2C1_CR1 &= ~(1UL << 8);  /* Clear START */
    mock_I2C1_CR1 |= (1UL << 9);   /* STOP */
    /* En vrai, STOP devrait se clear tout seul après exécution */
}

int mock_i2c_send_addr(uint8_t addr, uint8_t rw)
{
    (void)addr;
    (void)rw;
    
    /* Simuler envoi adresse avec ACK */
    mock_I2C1_SR1 |= (1UL << 1);  /* ADDR flag */
    /* Simuler lecture SR2 pour clear ADDR (le driver le fera) */
    return 0;  /* OK */
}

int mock_i2c_write_byte(uint8_t data)
{
    (void)data;
    
    /* Simuler écriture de données avec ACK */
    mock_I2C1_SR1 |= (1UL << 7);  /* TXE flag */
    mock_I2C1_SR1 |= (1UL << 2);  /* BTF flag (Byte Transfer Finished) */
    return 0;  /* OK */
}

/* Fonction pour simuler la lecture (appelée par le test) */
void mock_i2c_prepare_read(void)
{
    if (i2c_byte_count == 0) {
        mock_I2C1_DR = i2c_rx_msb;
        i2c_byte_count = 1;
    } else {
        mock_I2C1_DR = i2c_rx_lsb;
        i2c_byte_count = 0;
    }
    mock_I2C1_SR1 |= (1UL << 6);  /* RXNE flag */
}
