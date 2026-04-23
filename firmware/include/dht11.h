/**
 * @file    dht11.h
 * @brief   Driver capteur temperature/humidite DHT11
 *
 * Capteur  : DHT11
 * Interface: 1-wire proprietaire (single bus)
 * Pin data : PA1
 * Mesures  : humidite (%), temperature (degC)
 *
 * Protocole DHT11 :
 *   1. MCU envoie signal START : PA1 LOW pendant 18ms, puis HIGH
 *   2. DHT11 repond : LOW 80us, HIGH 80us
 *   3. DHT11 envoie 40 bits de donnees :
 *      - '0' : LOW 50us + HIGH 26-28us
 *      - '1' : LOW 50us + HIGH 70us
 *   4. Structure des 40 bits :
 *      [7:0]  humidity_integer
 *      [15:8] humidity_decimal  (toujours 0 pour DHT11)
 *      [23:16] temperature_integer
 *      [31:24] temperature_decimal (toujours 0 pour DHT11)
 *      [39:32] checksum = somme des 4 octets precedents
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

/*==============================================================
  Pin DATA — PA1
==============================================================*/
#define DHT11_GPIO_BASE     0x40020000UL   /* GPIOA base          */
#define DHT11_PIN           1U             /* PA1                 */

/* Registres GPIOA necessaires */
#define DHT11_GPIOA_MODER   (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x00UL))
#define DHT11_GPIOA_ODR     (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x14UL))
#define DHT11_GPIOA_IDR     (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x10UL))
#define DHT11_GPIOA_PUPDR   (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x0CUL))

/* RCC — horloge GPIOA (deja active par uart.c, mais on la reactive par securite) */
#define DHT11_RCC_BASE      0x40023800UL
#define DHT11_RCC_AHB1ENR   (*(volatile uint32_t *)(DHT11_RCC_BASE + 0x30UL))
#define DHT11_RCC_GPIOAEN   (1UL << 0U)

/*==============================================================
  Timing (us) — base SysTick 16 MHz HSI
==============================================================*/
#define DHT11_START_LOW_MS  18U    /* Signal START : LOW 18 ms          */
#define DHT11_START_HIGH_US 40U    /* Signal START : HIGH 40 us         */
#define DHT11_RESPONSE_US   80U    /* Reponse DHT11 : 80 us LOW + HIGH  */
#define DHT11_TIMEOUT_US    200U   /* Timeout lecture bit (iterations)  */
#define DHT11_BIT_THRESHOLD 40U    /* Seuil '0'/'1' : > 40us = bit '1' */

/* Nombre de bits transmis */
#define DHT11_DATA_BITS     40U

/*==============================================================
  Codes de retour
==============================================================*/
typedef enum
{
    DHT11_OK           = 0,  /**< Succes                          */
    DHT11_ERR_TIMEOUT,       /**< Timeout attente reponse DHT11   */
    DHT11_ERR_CHECKSUM,      /**< Checksum invalide               */
    DHT11_ERR_PARAM          /**< Parametre invalide              */
} DHT11_Status_t;

/*==============================================================
  Structure principale
==============================================================*/
typedef struct
{
    uint8_t humidity;       /**< Humidite en %          (0-100)  */
    uint8_t temperature;    /**< Temperature en degC    (0-50)   */
} DHT11_t;

/*==============================================================
  Interface publique
==============================================================*/

/**
 * @brief  Initialise la pin PA1 pour le DHT11.
 *
 * Configure PA1 en output push-pull par defaut (idle HIGH).
 */
void dht11_init(void);

/**
 * @brief  Lit la temperature et l'humidite du DHT11.
 *
 * Envoie le signal START, lit les 40 bits, verifie le checksum
 * et met a jour dev->humidity et dev->temperature.
 *
 * Duree totale : ~22 ms (bloquant).
 *
 * @param  dev  Pointeur vers la structure DHT11_t
 * @return DHT11_Status_t
 */
DHT11_Status_t dht11_read(DHT11_t *dev);

/**
 * @brief  Retourne la derniere humidite lue (%).
 *
 * @param  dev  Pointeur vers la structure DHT11_t
 * @return Humidite en % (uint8_t)
 */
uint8_t dht11_get_humidity(const DHT11_t *dev);

/**
 * @brief  Retourne la derniere temperature lue (degC).
 *
 * @param  dev  Pointeur vers la structure DHT11_t
 * @return Temperature en degC (uint8_t)
 */
uint8_t dht11_get_temperature(const DHT11_t *dev);

#endif /* DHT11_H */