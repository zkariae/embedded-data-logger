/**
 * @file    bh1750.h
 * @brief   Driver BH1750FVI — Capteur de luminosite I2C
 *
 * Connexion :
 *   SDA -> PB7 (I2C1_SDA)
 *   SCL -> PB6 (I2C1_SCL)
 *   ADD -> GND (adresse 0x23)
 *   VCC -> 3.3V
 *   GND -> GND
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>

/* ------------------------------------------------------------------
 * Adresses des registres I2C1
 * ------------------------------------------------------------------ */
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/* GPIOB */
#define GPIOB_BASE      0x40020400UL
#define GPIOB_MODER     (*(volatile uint32_t *)(GPIOB_BASE + 0x00UL))
#define GPIOB_OTYPER    (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL))
#define GPIOB_OSPEEDR   (*(volatile uint32_t *)(GPIOB_BASE + 0x08UL))
#define GPIOB_PUPDR     (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL))
#define GPIOB_AFRL      (*(volatile uint32_t *)(GPIOB_BASE + 0x20UL))

/* I2C1 */
#define I2C1_BASE       0x40005400UL
#define I2C1_CR1        (*(volatile uint32_t *)(I2C1_BASE + 0x00UL))
#define I2C1_CR2        (*(volatile uint32_t *)(I2C1_BASE + 0x04UL))
#define I2C1_OAR1       (*(volatile uint32_t *)(I2C1_BASE + 0x08UL))
#define I2C1_DR         (*(volatile uint32_t *)(I2C1_BASE + 0x10UL))
#define I2C1_SR1        (*(volatile uint32_t *)(I2C1_BASE + 0x14UL))
#define I2C1_SR2        (*(volatile uint32_t *)(I2C1_BASE + 0x18UL))
#define I2C1_CCR        (*(volatile uint32_t *)(I2C1_BASE + 0x1CUL))
#define I2C1_TRISE      (*(volatile uint32_t *)(I2C1_BASE + 0x20UL))

/* Bits I2C_CR1 */
#define I2C_CR1_PE      (1UL << 0)   /* Peripheral enable     */
#define I2C_CR1_START   (1UL << 8)   /* Start generation      */
#define I2C_CR1_STOP    (1UL << 9)   /* Stop generation       */
#define I2C_CR1_ACK     (1UL << 10)  /* Acknowledge enable    */
#define I2C_CR1_SWRST   (1UL << 15)  /* Software reset        */

/* Bits I2C_SR1 */
#define I2C_SR1_SB      (1UL << 0)   /* Start bit             */
#define I2C_SR1_ADDR    (1UL << 1)   /* Address sent          */
#define I2C_SR1_BTF     (1UL << 2)   /* Byte transfer finished*/
#define I2C_SR1_RXNE    (1UL << 6)   /* Data register not empty */
#define I2C_SR1_TXE     (1UL << 7)   /* Data register empty   */

/* ------------------------------------------------------------------
 * BH1750 — Adresse et commandes
 * ------------------------------------------------------------------ */
#define BH1750_ADDR             0x23UL  /* ADD = GND             */
#define BH1750_POWER_ON         0x01UL  /* Power on              */
#define BH1750_RESET            0x07UL  /* Reset data register   */
#define BH1750_CONT_H_RES_MODE  0x10UL  /* Continuous high res   */

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

/**
 * @brief  Initialise I2C1 et le capteur BH1750.
 */
void bh1750_init(void);

/**
 * @brief  Lit la luminosite en lux.
 * @return Valeur en lux (0 - 65535)
 */
uint16_t bh1750_read_lux(void);

#endif /* BH1750_H */