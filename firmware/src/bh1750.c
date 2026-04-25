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
#include "log.h"

/* ------------------------------------------------------------------
 * Constantes
 * ------------------------------------------------------------------ */

#define I2C_TIMEOUT 10000UL   /* Timeout pour les attentes I2C */

/* ------------------------------------------------------------------
 * Fonctions privees I2C
 * ------------------------------------------------------------------ */

/**
 * @brief  Attend que le bit soit set dans SR1 avec timeout.
 * @param  flag : bit a attendre
 * @return 0 si OK, -1 si timeout
 */
static int i2c_wait_flag(uint32_t flag)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!(I2C1_SR1 & flag))
    {
        if (--timeout == 0)
            return -1;
    }
    return 0;
}

/**
 * @brief  Genere la condition START sur le bus I2C.
 * @return 0 si OK, -1 si timeout
 */
static int i2c_start(void)
{
    I2C1_CR1 |= I2C_CR1_START;
    return i2c_wait_flag(I2C_SR1_SB);
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
 * @param  addr : adresse 7 bits du peripherique
 * @param  rw   : 0 = ecriture, 1 = lecture
 * @return 0 si OK, -1 si timeout
 */
static int i2c_send_addr(uint8_t addr, uint8_t rw)
{
    I2C1_DR = (uint32_t)((addr << 1) | rw);
    if (i2c_wait_flag(I2C_SR1_ADDR) != 0)
        return -1;
    /* Clear ADDR flag en lisant SR1 puis SR2 */
    (void)I2C1_SR1;
    (void)I2C1_SR2;
    return 0;
}

/**
 * @brief  Envoie un octet sur le bus I2C.
 * @param  data : octet a envoyer
 * @return 0 si OK, -1 si timeout
 */
static int i2c_send_byte(uint8_t data)
{
    if (i2c_wait_flag(I2C_SR1_TXE) != 0)
        return -1;
    I2C1_DR = (uint32_t)data;
    return i2c_wait_flag(I2C_SR1_BTF);
}

/**
 * @brief  Lit un octet depuis le bus I2C.
 * @param  ack : 1 = envoyer ACK, 0 = envoyer NACK (dernier octet)
 * @return octet lu, ou 0 si timeout
 */
static uint8_t i2c_read_byte(uint8_t ack)
{
    if (ack)
        I2C1_CR1 |= I2C_CR1_ACK;
    else
        I2C1_CR1 &= ~I2C_CR1_ACK;

    if (i2c_wait_flag(I2C_SR1_RXNE) != 0)
        return 0;

    return (uint8_t)(I2C1_DR & 0xFF);
}

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

/**
 * @brief  Recuperation du bus I2C bloque.
 *         Genere 9 impulsions d'horloge manuelles sur SCL
 *         pour debloquer un peripherique I2C bloque.
 */
static void i2c_bus_recovery(void)
{
    /* Desactiver I2C1 */
    I2C1_CR1 &= ~I2C_CR1_PE;
    delay_ms(5);

    /* Configurer PB6 (SCL) et PB7 (SDA) en GPIO output */
    GPIOB_MODER &= ~((3UL << 12) | (3UL << 14));
    GPIOB_MODER |=  ((1UL << 12) | (1UL << 14));

    /* SDA et SCL HIGH */
    GPIOB_ODR |= (1UL << 6) | (1UL << 7);
    delay_ms(5);

    /* Generer 9 impulsions d'horloge */
    for (int i = 0; i < 9; i++)
    {
        GPIOB_ODR &= ~(1UL << 6);  /* SCL LOW  */
        delay_ms(2);
        GPIOB_ODR |=  (1UL << 6);  /* SCL HIGH */
        delay_ms(2);
    }

    /* Condition STOP manuelle */
    GPIOB_ODR &= ~(1UL << 7);  /* SDA LOW  */
    delay_ms(2);
    GPIOB_ODR |=  (1UL << 6);  /* SCL HIGH */
    delay_ms(2);
    GPIOB_ODR |=  (1UL << 7);  /* SDA HIGH */
    delay_ms(5);

    /* Reconfigurer PB6 et PB7 en AF4 */
    GPIOB_MODER &= ~((3UL << 12) | (3UL << 14));
    GPIOB_MODER |=  ((2UL << 12) | (2UL << 14));
    delay_ms(5);

    /* Reset complet I2C1 */
    I2C1_CR1 |= I2C_CR1_SWRST;
    delay_ms(10);
    I2C1_CR1 &= ~I2C_CR1_SWRST;
    delay_ms(10);

    /* Reconfigurer I2C1 */
    I2C1_CR2   = 16UL;
    I2C1_CCR   = 80UL;
    I2C1_TRISE = 17UL;

    /* Reactiver I2C1 */
    I2C1_CR1 |= I2C_CR1_PE;
    delay_ms(5);
}


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
    /* Recovery du bus I2C si bloque */
    i2c_bus_recovery();

    /* 1. Activer les horloges GPIOB et I2C1 */
    RCC_AHB1ENR |= (1UL << 1);   /* GPIOB clock */
    RCC_APB1ENR |= (1UL << 21);  /* I2C1 clock  */

    /* 2. Configurer PB6 et PB7 en Alternate Function (AF4 = I2C1) */
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

    /* 3. Reset complet I2C1 */
    I2C1_CR1 |= I2C_CR1_SWRST;
    delay_ms(10);
    I2C1_CR1 &= ~I2C_CR1_SWRST;
    delay_ms(10);

    /* 4. Configurer I2C1 en mode standard 100 kHz */
    I2C1_CR2   = 16UL;   /* FREQ = 16 MHz              */
    I2C1_CCR   = 80UL;   /* CCR = 16MHz/(2*100kHz)     */
    I2C1_TRISE = 17UL;   /* TRISE = (1000ns/62.5ns)+1  */

    /* 5. Activer I2C1 */
    I2C1_CR1 |= I2C_CR1_PE;
    delay_ms(10);

    /* 6. Power ON */
    if (i2c_start() != 0)
    { i2c_stop(); LOG_ERROR("BH1750 : i2c_start failed (Power ON)"); return; }

    LOG_INFO("BH1750 : START OK");
    uart_send_int((int32_t)I2C1_SR1);
    uart_send_string(" <- SR1\r\n");
    uart_send_int((int32_t)I2C1_SR2);
    uart_send_string(" <- SR2\r\n");

    if (i2c_send_addr(BH1750_ADDR, 0) != 0)
    { i2c_stop(); LOG_ERROR("BH1750 : i2c_send_addr failed (Power ON)"); return; }
    if (i2c_send_byte(BH1750_POWER_ON) != 0)
    { i2c_stop(); LOG_ERROR("BH1750 : i2c_send_byte failed (Power ON)"); return; }
    i2c_stop();
    LOG_INFO("BH1750 : Power ON OK");
    delay_ms(10);

    /* 7. Mode : One Time High Resolution */
    if (i2c_start() != 0)
    { i2c_stop(); LOG_ERROR("BH1750 : i2c_start failed (Mode)"); return; }
    if (i2c_send_addr(BH1750_ADDR, 0) != 0)
    { i2c_stop(); LOG_ERROR("BH1750 : i2c_send_addr failed (Mode)"); return; }
    if (i2c_send_byte(BH1750_ONE_TIME_H_RES_MODE) != 0)
    { i2c_stop(); LOG_ERROR("BH1750 : i2c_send_byte failed (Mode)"); return; }
    i2c_stop();
    LOG_INFO("BH1750 : Mode OK");
    delay_ms(180);
}

/**
 * @brief  Lit la luminosite en lux depuis le BH1750.
 *
 * Le BH1750 renvoie 2 octets (MSB + LSB).
 * Conversion : lux = (MSB << 8 | LSB) / 1.2
 *
 * @return Valeur en lux (0 - 65535), 0 en cas d'erreur
 */
uint16_t bh1750_read_lux(void)
{
    uint8_t  msb     = 0;
    uint8_t  lsb     = 0;
    uint16_t raw     = 0;
    uint32_t timeout = 0;

    /* Envoyer commande One Time avant chaque lecture */
    if (i2c_start() != 0)
    { i2c_stop(); LOG_ERROR("BH1750 read: START1 failed"); return 0; }
    if (i2c_send_addr(BH1750_ADDR, 0) != 0)
    { i2c_stop(); LOG_ERROR("BH1750 read: ADDR1 failed"); return 0; }
    if (i2c_send_byte(BH1750_ONE_TIME_H_RES_MODE) != 0)
    { i2c_stop(); LOG_ERROR("BH1750 read: CMD failed"); return 0; }
    i2c_stop();
    delay_ms(180);   /* Attendre fin de mesure */

    /* Lire le resultat */
    if (i2c_start() != 0)
    { i2c_stop(); LOG_ERROR("BH1750 read: START2 failed"); return 0; }
    if (i2c_send_addr(BH1750_ADDR, 1) != 0)
    { i2c_stop(); LOG_ERROR("BH1750 read: ADDR2 failed"); return 0; }

    /* Activer ACK */
    I2C1_CR1 |= I2C_CR1_ACK;

    /* Clear ADDR flag */
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /* Attendre MSB */
    timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_RXNE))
        if (--timeout == 0) { LOG_ERROR("BH1750 read: MSB failed"); return 0; }
    msb = (uint8_t)(I2C1_DR & 0xFF);

    /* Desactiver ACK avant de lire LSB */
    I2C1_CR1 &= ~I2C_CR1_ACK;

    /* Programmer STOP */
    I2C1_CR1 |= I2C_CR1_STOP;

    /* Attendre LSB */
    timeout = 10000;
    while (!(I2C1_SR1 & I2C_SR1_RXNE))
        if (--timeout == 0) { LOG_ERROR("BH1750 read: LSB failed"); return 0; }
    lsb = (uint8_t)(I2C1_DR & 0xFF);

    raw = (uint16_t)((msb << 8) | lsb);
    uart_send_int((int32_t)raw);
    uart_send_string(" <- raw lux\r\n");

    return (uint16_t)(raw * 10 / 12);
}