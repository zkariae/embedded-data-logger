/**
 * @file    bh1750.c
 * @brief   Driver BH1750FVI — Capteur de luminosite I2C
 *
 * Connexion :
 *   SDA -> PB7 (I2C1_SDA, AF4)
 *   SCL -> PB6 (I2C1_SCL, AF4)
 *   ADD -> GND (adresse 0x23)
 *   VCC -> 3.3V
 *   GND -> GND
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#include "bh1750.h"
#include "systick.h"

/* ------------------------------------------------------------------
 * Fonctions privees I2C
 * ------------------------------------------------------------------ */

/**
 * @brief  Attend que le bit soit set dans le registre SR1.
 * @param  flag : bit a attendre
 */
static void i2c_wait_flag(uint32_t flag)
{
    while (!(I2C1_SR1 & flag))
        ;
}

/**
 * @brief  Genere la condition START sur le bus I2C.
 */
static void i2c_start(void)
{
    I2C1_CR1 |= I2C_CR1_START;
    i2c_wait_flag(I2C_SR1_SB);
}

/**
 * @brief  Genere la condition STOP sur le bus I2C.
 */
static void i2c_stop(void)
{
    I2C1_CR1 |= I2C_CR1_STOP;
}

/**
 * @brief  Envoie l'adresse du peripherique sur le bus I2C.
 * @param  addr    : adresse 7 bits du peripherique
 * @param  rw      : 0 = ecriture, 1 = lecture
 */
static void i2c_send_addr(uint8_t addr, uint8_t rw)
{
    I2C1_DR = (uint32_t)((addr << 1) | rw);
    i2c_wait_flag(I2C_SR1_ADDR);
    /* Clear ADDR flag en lisant SR1 puis SR2 */
    (void)I2C1_SR1;
    (void)I2C1_SR2;
}

/**
 * @brief  Envoie un octet sur le bus I2C.
 * @param  data : octet a envoyer
 */
static void i2c_send_byte(uint8_t data)
{
    i2c_wait_flag(I2C_SR1_TXE);
    I2C1_DR = (uint32_t)data;
    i2c_wait_flag(I2C_SR1_BTF);
}

/**
 * @brief  Lit un octet depuis le bus I2C.
 * @param  ack : 1 = envoyer ACK, 0 = envoyer NACK (dernier octet)
 * @return octet lu
 */
static uint8_t i2c_read_byte(uint8_t ack)
{
    if (ack)
        I2C1_CR1 |= I2C_CR1_ACK;
    else
        I2C1_CR1 &= ~I2C_CR1_ACK;

    i2c_wait_flag(I2C_SR1_RXNE);
    return (uint8_t)(I2C1_DR & 0xFF);
}

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

/**
 * @brief  Initialise I2C1 et le capteur BH1750.
 *
 * Configuration I2C :
 *   - Frequence APB1 : 16 MHz (HSI)
 *   - Mode standard  : 100 kHz
 *   - PB6 = SCL (AF4, open-drain)
 *   - PB7 = SDA (AF4, open-drain)
 */
void bh1750_init(void)
{
    /* 1. Activer les horloges GPIOB et I2C1 */
    RCC_AHB1ENR |= (1UL << 1);   /* GPIOB clock */
    RCC_APB1ENR |= (1UL << 21);  /* I2C1 clock  */

    /* 2. Configurer PB6 et PB7 en Alternate Function (AF4 = I2C1) */
    /* MODER : AF mode (10) pour PB6 et PB7 */
    GPIOB_MODER &= ~((3UL << 12) | (3UL << 14));
    GPIOB_MODER |=  ((2UL << 12) | (2UL << 14));

    /* OTYPER : open-drain pour PB6 et PB7 */
    GPIOB_OTYPER |= (1UL << 6) | (1UL << 7);

    /* OSPEEDR : high speed pour PB6 et PB7 */
    GPIOB_OSPEEDR |= ((3UL << 12) | (3UL << 14));

    /* PUPDR : pull-up pour PB6 et PB7 */
    GPIOB_PUPDR &= ~((3UL << 12) | (3UL << 14));
    GPIOB_PUPDR |=  ((1UL << 12) | (1UL << 14));

    /* AFRL : AF4 pour PB6 (bits 24-27) et PB7 (bits 28-31) */
    GPIOB_AFRL &= ~((0xFUL << 24) | (0xFUL << 28));
    GPIOB_AFRL |=  ((4UL  << 24) | (4UL  << 28));

    /* 3. Reset I2C1 */
    I2C1_CR1 |= I2C_CR1_SWRST;
    I2C1_CR1 &= ~I2C_CR1_SWRST;

    /* 4. Configurer I2C1 en mode standard 100 kHz */
    /* APB1 = 16 MHz */
    I2C1_CR2  = 16UL;              /* FREQ = 16 MHz          */
    I2C1_CCR  = 80UL;              /* CCR = 16MHz/(2*100kHz) */
    I2C1_TRISE = 17UL;             /* TRISE = (1000ns/62.5ns)+1 */

    /* 5. Activer I2C1 */
    I2C1_CR1 |= I2C_CR1_PE;

    /* 6. Initialiser le BH1750 */
    delay_ms(10);

    /* Power ON */
    i2c_start();
    i2c_send_addr(BH1750_ADDR, 0);
    i2c_send_byte(BH1750_POWER_ON);
    i2c_stop();
    delay_ms(10);

    /* Mode : Continuous High Resolution */
    i2c_start();
    i2c_send_addr(BH1750_ADDR, 0);
    i2c_send_byte(BH1750_CONT_H_RES_MODE);
    i2c_stop();
    delay_ms(180);  /* Temps de mesure : 120-180ms */
}

/**
 * @brief  Lit la luminosite en lux depuis le BH1750.
 *
 * Le BH1750 renvoie 2 octets (MSB + LSB).
 * Conversion : lux = (MSB << 8 | LSB) / 1.2
 *
 * @return Valeur en lux (0 - 65535)
 */
uint16_t bh1750_read_lux(void)
{
    uint8_t  msb = 0;
    uint8_t  lsb = 0;
    uint16_t raw = 0;

    i2c_start();
    i2c_send_addr(BH1750_ADDR, 1);  /* Lecture */

    msb = i2c_read_byte(1);   /* MSB — ACK  */
    lsb = i2c_read_byte(0);   /* LSB — NACK */

    i2c_stop();

    raw = (uint16_t)((msb << 8) | lsb);

    /* Conversion : lux = raw / 1.2 */
    return (uint16_t)(raw * 10 / 12);
}