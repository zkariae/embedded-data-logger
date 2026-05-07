/**
 * @file    mock_hw.h
 * @brief   Simulation des registres STM32 pour tests unitaires sur PC
 */

#ifndef MOCK_HW_H
#define MOCK_HW_H

#include <stdint.h>
#include <stdlib.h>

/* Simulation des registres via des variables globales */
extern volatile uint32_t mock_RCC_AHB1ENR;
extern volatile uint32_t mock_RCC_APB1ENR;
extern volatile uint32_t mock_GPIOB_MODER;
extern volatile uint32_t mock_GPIOB_OTYPER;
extern volatile uint32_t mock_GPIOB_OSPEEDR;
extern volatile uint32_t mock_GPIOB_PUPDR;
extern volatile uint32_t mock_GPIOB_AFRL;
extern volatile uint32_t mock_GPIOB_ODR;
extern volatile uint32_t mock_I2C1_CR1;
extern volatile uint32_t mock_I2C1_CR2;
extern volatile uint32_t mock_I2C1_DR;
extern volatile uint32_t mock_I2C1_SR1;
extern volatile uint32_t mock_I2C1_SR2;
extern volatile uint32_t mock_I2C1_CCR;
extern volatile uint32_t mock_I2C1_TRISE;

/* Définitions pour le code source (redirection vers les mocks) */
#ifdef TEST_HOST
#define RCC_AHB1ENR     mock_RCC_AHB1ENR
#define RCC_APB1ENR     mock_RCC_APB1ENR
#define GPIOB_MODER     mock_GPIOB_MODER
#define GPIOB_OTYPER    mock_GPIOB_OTYPER
#define GPIOB_OSPEEDR   mock_GPIOB_OSPEEDR
#define GPIOB_PUPDR     mock_GPIOB_PUPDR
#define GPIOB_AFRL      mock_GPIOB_AFRL
#define GPIOB_ODR       mock_GPIOB_ODR
#define I2C1_CR1        mock_I2C1_CR1
#define I2C1_CR2        mock_I2C1_CR2
#define I2C1_DR         mock_I2C1_DR
#define I2C1_SR1        mock_I2C1_SR1
#define I2C1_SR2        mock_I2C1_SR2
#define I2C1_CCR        mock_I2C1_CCR
#define I2C1_TRISE      mock_I2C1_TRISE
#endif

/* Fonctions de simulation I2C pour les tests */
void mock_i2c_reset(void);
void mock_i2c_set_lux_data(uint8_t msb, uint8_t lsb);
void mock_i2c_simulate_ack(void);
void mock_i2c_simulate_nack(void);

#endif /* MOCK_HW_H */
