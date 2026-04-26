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
 * Constantes privees
 * ------------------------------------------------------------------ */

#define I2C_TIMEOUT     10000UL   /* Nombre d'iterations max avant timeout */
#define I2C_FREQ_MHZ    16UL      /* Frequence APB1 en MHz (HSI = 16MHz)   */
#define I2C_CCR_100KHZ  80UL      /* CCR = FPCLK1/(2*Fscl) = 16M/(2*100k) */
#define I2C_TRISE_100KHZ 17UL     /* TRISE = (1000ns / 62.5ns) + 1         */

/* ------------------------------------------------------------------
 * Fonctions privees — helpers I2C
 * ------------------------------------------------------------------ */

/**
 * @brief  Attend qu'un flag soit set dans I2C1_SR1.
 * @param  flag       : masque du bit a attendre
 * @return 0 si OK, -1 si timeout
 */
static int i2c_wait_sr1(uint32_t flag)
{
    uint32_t t = I2C_TIMEOUT;
    while (!(I2C1_SR1 & flag))
        if (--t == 0) return -1;
    return 0;
}

/**
 * @brief  Genere la condition START et attend SB.
 * @return 0 si OK, -1 si timeout
 */
static int i2c_start(void)
{
    I2C1_CR1 |= I2C_CR1_START;
    return i2c_wait_sr1(I2C_SR1_SB);
}

/**
 * @brief  Genere la condition STOP.
 */
static void i2c_stop(void)
{
    I2C1_CR1 |= I2C_CR1_STOP;
}

/**
 * @brief  Envoie l'adresse 7 bits + bit R/W et attend ADDR.
 * @param  addr : adresse 7 bits
 * @param  rw   : 0 = ecriture, 1 = lecture
 * @return 0 si OK, -1 si timeout ou NACK
 */
static int i2c_send_addr(uint8_t addr, uint8_t rw)
{
    I2C1_DR = (uint32_t)((addr << 1) | rw);
    if (i2c_wait_sr1(I2C_SR1_ADDR) != 0) return -1;
    (void)I2C1_SR1;   /* Clear ADDR : lire SR1 puis SR2 */
    (void)I2C1_SR2;
    return 0;
}

/**
 * @brief  Envoie un octet et attend BTF.
 * @param  data : octet a envoyer
 * @return 0 si OK, -1 si timeout
 */
static int i2c_write_byte(uint8_t data)
{
    if (i2c_wait_sr1(I2C_SR1_TXE) != 0) return -1;
    I2C1_DR = (uint32_t)data;
    return i2c_wait_sr1(I2C_SR1_BTF);
}

/**
 * @brief  Recuperation du bus I2C bloque (9 clock pulses).
 *
 * Si SDA est bloque LOW par un peripherique, genere 9 impulsions
 * manuelles sur SCL pour debloquer le peripherique, puis envoie
 * une condition STOP manuelle.
 * Reconfigure ensuite PB6/PB7 en AF4 et reinitialise I2C1.
 */
static void i2c_bus_recovery(void)
{
    /* Desactiver I2C1 */
    I2C1_CR1 &= ~I2C_CR1_PE;
    delay_ms(5);

    /* PB6/PB7 en sortie GPIO */
    GPIOB_MODER &= ~((3UL << 12) | (3UL << 14));
    GPIOB_MODER |=  ((1UL << 12) | (1UL << 14));

    /* SDA et SCL HIGH */
    GPIOB_ODR |= (1UL << 6) | (1UL << 7);
    delay_ms(5);

    /* 9 impulsions CLK */
    for (int i = 0; i < 9; i++)
    {
        GPIOB_ODR &= ~(1UL << 6);  delay_ms(2);
        GPIOB_ODR |=  (1UL << 6);  delay_ms(2);
    }

    /* STOP manuelle : SDA LOW -> SCL HIGH -> SDA HIGH */
    GPIOB_ODR &= ~(1UL << 7);  delay_ms(2);
    GPIOB_ODR |=  (1UL << 6);  delay_ms(2);
    GPIOB_ODR |=  (1UL << 7);  delay_ms(5);

    /* Reconfigurer PB6/PB7 en AF4 (I2C1) */
    GPIOB_MODER &= ~((3UL << 12) | (3UL << 14));
    GPIOB_MODER |=  ((2UL << 12) | (2UL << 14));
    delay_ms(5);

    /* Reset et reconfiguration complete I2C1 */
    I2C1_CR1 |=  I2C_CR1_SWRST;  delay_ms(10);
    I2C1_CR1 &= ~I2C_CR1_SWRST;  delay_ms(10);

    I2C1_CR2   = I2C_FREQ_MHZ;
    I2C1_CCR   = I2C_CCR_100KHZ;
    I2C1_TRISE = I2C_TRISE_100KHZ;

    I2C1_CR1 |= I2C_CR1_PE;
    delay_ms(5);
}

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

/**
 * @brief  Initialise I2C1 et le capteur BH1750.
 *
 * Sequence :
 *   1. Recovery du bus si bloque
 *   2. Configuration GPIO PB6/PB7 en AF4 open-drain pull-up
 *   3. Configuration I2C1 : 100 kHz, APB1 = 16 MHz
 *   4. Envoi commande POWER_ON
 *   5. Envoi commande ONE_TIME_H_RES_MODE
 */
void bh1750_init(void)
{
    /* 1. Recovery preventive du bus */
    i2c_bus_recovery();

    /* 2. Activer horloges GPIOB et I2C1 */
    RCC_AHB1ENR |= (1UL << 1);   /* GPIOB */
    RCC_APB1ENR |= (1UL << 21);  /* I2C1  */

    /* 3. PB6 = SCL, PB7 = SDA : AF4, open-drain, pull-up, high speed */
    GPIOB_MODER   &= ~((3UL << 12) | (3UL << 14));
    GPIOB_MODER   |=  ((2UL << 12) | (2UL << 14));
    GPIOB_OTYPER  |=  (1UL << 6)  | (1UL << 7);
    GPIOB_OSPEEDR |=  (3UL << 12) | (3UL << 14);
    GPIOB_PUPDR   &= ~((3UL << 12) | (3UL << 14));
    GPIOB_PUPDR   |=  (1UL << 12) | (1UL << 14);
    GPIOB_AFRL    &= ~((0xFUL << 24) | (0xFUL << 28));
    GPIOB_AFRL    |=  (4UL << 24)   | (4UL << 28);

    /* 4. Configurer I2C1 : 100 kHz */
    I2C1_CR1  |=  I2C_CR1_SWRST;  delay_ms(10);
    I2C1_CR1  &= ~I2C_CR1_SWRST;  delay_ms(10);
    I2C1_CR2   =  I2C_FREQ_MHZ;
    I2C1_CCR   =  I2C_CCR_100KHZ;
    I2C1_TRISE =  I2C_TRISE_100KHZ;
    I2C1_CR1  |=  I2C_CR1_PE;
    delay_ms(10);

    /* 5. Power ON */
    if (i2c_start() != 0)                        { i2c_stop(); return; }
    if (i2c_send_addr(BH1750_ADDR, 0) != 0)      { i2c_stop(); return; }
    if (i2c_write_byte(BH1750_POWER_ON) != 0)    { i2c_stop(); return; }
    i2c_stop();
    LOG_INFO("BH1750 : init OK");
    delay_ms(10);
}

/**
 * @brief  Declenche une mesure et lit la luminosite en lux.
 *
 * Sequence :
 *   1. Envoyer ONE_TIME_H_RES_MODE -> declenche une mesure unique
 *   2. Attendre 180ms (temps de mesure haute resolution)
 *   3. Lire 2 octets MSB + LSB
 *   4. Calculer lux = raw / 1.2
 *
 * @return Valeur en lux (0 - 65535), 0 en cas d'erreur I2C
 */
uint16_t bh1750_read_lux(void)
{
    uint8_t  msb = 0;
    uint8_t  lsb = 0;

    /* Declencher la mesure */
    if (i2c_start() != 0)                              { i2c_stop(); return 0; }
    if (i2c_send_addr(BH1750_ADDR, 0) != 0)            { i2c_stop(); return 0; }
    if (i2c_write_byte(BH1750_ONE_TIME_H_RES_MODE) != 0){ i2c_stop(); return 0; }
    i2c_stop();
    delay_ms(180);   /* Temps de mesure haute resolution */

    /* Lire le resultat (2 octets) */
    if (i2c_start() != 0)                   { i2c_stop(); return 0; }
    if (i2c_send_addr(BH1750_ADDR, 1) != 0) { i2c_stop(); return 0; }

    /* Sequence STM32F4 pour reception 2 octets :
     * - Activer ACK avant clear ADDR
     * - Lire MSB
     * - Desactiver ACK + programmer STOP avant lecture LSB */
    I2C1_CR1 |= I2C_CR1_ACK;

    if (i2c_wait_sr1(I2C_SR1_RXNE) != 0) { i2c_stop(); return 0; }
    msb = (uint8_t)(I2C1_DR & 0xFF);

    I2C1_CR1 &= ~I2C_CR1_ACK;
    I2C1_CR1 |=  I2C_CR1_STOP;

    if (i2c_wait_sr1(I2C_SR1_RXNE) != 0) return 0;
    lsb = (uint8_t)(I2C1_DR & 0xFF);

    /* Conversion : lux = raw / 1.2 = raw * 10 / 12 */
    return (uint16_t)(((uint16_t)((msb << 8) | lsb)) * 10U / 12U);
}