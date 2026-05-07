/**
 * @file    mock_hw.c
 * @brief   Implémentation de la simulation des registres STM32 pour tests unitaires
 */

#include "mock_hw.h"

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
static uint8_t expected_addr = 0;
static uint8_t expected_rw = 0;
static uint8_t i2c_rx_msb = 0;
static uint8_t i2c_rx_lsb = 0;
static int i2c_nack_enabled = 0;

void mock_i2c_reset(void)
{
    mock_I2C1_CR1 = 0;
    mock_I2C1_CR2 = 0;
    mock_I2C1_DR = 0;
    mock_I2C1_SR1 = 0;
    mock_I2C1_SR2 = 0;
    mock_I2C1_CCR = 0;
    mock_I2C1_TRISE = 0;
    i2c_nack_enabled = 0;
}

void mock_i2c_set_lux_data(uint8_t msb, uint8_t lsb)
{
    i2c_rx_msb = msb;
    i2c_rx_lsb = lsb;
}

void mock_i2c_simulate_ack(void)
{
    i2c_nack_enabled = 0;
}

void mock_i2c_simulate_nack(void)
{
    i2c_nack_enabled = 1;
}

/* Fonction simulant le comportement I2C lors des accès */
void mock_i2c_process(void)
{
    /* Simuler le comportement basique du bus I2C */
    if (mock_I2C1_CR1 & (1UL << 8)) {  /* START */
        mock_I2C1_SR1 |= (1UL << 0);   /* SB flag */
        mock_I2C1_CR1 &= ~(1UL << 8);  /* Clear START */
    }
    
    if (mock_I2C1_SR1 & (1UL << 0)) {  /* SB set, address phase */
        if (mock_I2C1_DR & 0x01) {     /* Read */
            mock_I2C1_SR1 |= (1UL << 1);   /* ADDR */
            mock_I2C1_SR2 = 0;             /* Clear ADDR by reading SR2 */
        } else {                         /* Write */
            mock_I2C1_SR1 |= (1UL << 1);   /* ADDR */
            mock_I2C1_SR2 = 0;
        }
    }
}
