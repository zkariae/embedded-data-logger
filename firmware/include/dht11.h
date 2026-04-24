/**
 * @file    dht11.h
 * @brief   Driver capteur temperature/humidite DHT11
 *
 * Capteur  : DHT11
 * Interface: 1-wire proprietaire (single bus)
 * Pin data : PC0
 * Mesures  : humidite (%), temperature (degC)
 *
 * Timing base sur TIM2 (timer 32 bits) configure a 1 MHz
 * -> 1 tick = 1 us, precision garantie independamment de
 * l'optimisation du compilateur.
 *
 * Protocole DHT11 :
 *   1. MCU envoie signal START : PC0 LOW pendant 18ms, puis HIGH
 *   2. DHT11 repond : LOW 80us, HIGH 80us
 *   3. DHT11 envoie 40 bits :
 *      - '0' : LOW 50us + HIGH ~26us
 *      - '1' : LOW 50us + HIGH ~70us
 *   4. Structure des 40 bits :
 *      [7:0]   humidity_integer
 *      [15:8]  humidity_decimal  (toujours 0 pour DHT11)
 *      [23:16] temperature_integer
 *      [31:24] temperature_decimal
 *      [39:32] checksum
 *
 * CORRECTIONS appliquees :
 *   - Pull-up interne PC0 active en mode input (evite bus flottant)
 *   - Valeur sentinelle UINT32_MAX dans wait_level (evite faux timeout)
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

/*==============================================================
  Pin DATA — PC0
==============================================================*/
#define DHT11_GPIO_BASE     0x40020800UL   /* GPIOC base */
#define DHT11_PIN           0U             /* PC0        */

/* Registres GPIOC */
#define DHT11_GPIOC_MODER   (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x00UL))
#define DHT11_GPIOC_ODR     (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x14UL))
#define DHT11_GPIOC_IDR     (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x10UL))
#define DHT11_GPIOC_PUPDR   (*(volatile uint32_t *)(DHT11_GPIO_BASE + 0x0CUL))

/*==============================================================
  RCC
==============================================================*/
#define DHT11_RCC_BASE      0x40023800UL
#define DHT11_RCC_AHB1ENR   (*(volatile uint32_t *)(DHT11_RCC_BASE + 0x30UL))
#define DHT11_RCC_APB1ENR   (*(volatile uint32_t *)(DHT11_RCC_BASE + 0x40UL))
#define DHT11_RCC_GPIOCEN   (1UL << 2U)   /* GPIOC = bit 2 */
#define DHT11_RCC_TIM2EN    (1UL << 0U)   /* TIM2  = bit 0 */

/*==============================================================
  TIM2 — Timer 32 bits, 1 MHz (1 tick = 1 us)
  PCLK1 = 16 MHz HSI -> PSC = 15 -> 16/(15+1) = 1 MHz
==============================================================*/
#define TIM2_BASE           0x40000000UL
#define TIM2_CR1            (*(volatile uint32_t *)(TIM2_BASE + 0x00UL))
#define TIM2_PSC            (*(volatile uint32_t *)(TIM2_BASE + 0x28UL))
#define TIM2_ARR            (*(volatile uint32_t *)(TIM2_BASE + 0x2CUL))
#define TIM2_CNT            (*(volatile uint32_t *)(TIM2_BASE + 0x24UL))
#define TIM2_EGR            (*(volatile uint32_t *)(TIM2_BASE + 0x14UL))

/* Bits TIM2_CR1 */
#define TIM2_CR1_CEN        (1UL << 0U)   /* Counter enable    */

/* Bits TIM2_EGR */
#define TIM2_EGR_UG         (1UL << 0U)   /* Update generation */

/*==============================================================
  Timing DHT11 (en microsecondes)
==============================================================*/
#define DHT11_START_LOW_MS  18U    /* START : LOW 18 ms          */
#define DHT11_START_HIGH_US 40U    /* START : HIGH 40 us         */
#define DHT11_TIMEOUT_US    200U   /* Timeout signal : 200 us    */
                                   /* (80us reponse + marge)     */
#define DHT11_BIT_THRESHOLD 50U    /* Seuil '0'/'1' : 50 us      */
#define DHT11_DATA_BITS     40U    /* Nombre de bits a lire       */

/*==============================================================
  Valeur sentinelle wait_level
  UINT32_MAX ne peut pas etre une duree legitime -> indique timeout
==============================================================*/
#define DHT11_WAIT_TIMEOUT  0xFFFFFFFFUL

/*==============================================================
  Codes de retour
==============================================================*/
typedef enum
{
    DHT11_OK           = 0,  /**< Succes                        */
    DHT11_ERR_TIMEOUT,       /**< Timeout attente reponse       */
    DHT11_ERR_CHECKSUM,      /**< Checksum invalide             */
    DHT11_ERR_PARAM          /**< Parametre invalide            */
} DHT11_Status_t;

/*==============================================================
  Structure principale
==============================================================*/
typedef struct
{
    uint8_t humidity;       /**< Humidite en %       (0-100) */
    uint8_t temperature;    /**< Temperature en degC (0-50)  */
} DHT11_t;

/*==============================================================
  Interface publique
==============================================================*/

/**
 * @brief  Initialise TIM2 a 1 MHz et PC0 pour le DHT11.
 *         - Active l'horloge GPIOC et TIM2
 *         - Configure PC0 en output push-pull, HIGH (repos)
 *         - Active le pull-up interne sur PC0
 *         - Attend 1s pour laisser le capteur se stabiliser
 *
 * @note   A appeler UNE SEULE FOIS au demarrage.
 *         Ne pas appeler tim2_init() separement dans main().
 */
void dht11_init(void);

/**
 * @brief  Lit temperature et humidite du DHT11.
 *
 * @param  dev  Pointeur vers DHT11_t (non NULL)
 * @return DHT11_OK           : lecture reussie
 *         DHT11_ERR_TIMEOUT  : pas de reponse du capteur
 *         DHT11_ERR_CHECKSUM : donnees corrompues
 *         DHT11_ERR_PARAM    : dev == NULL
 *
 * @note   Appeler au minimum toutes les 2 secondes.
 *         Les interruptions sont desactivees pendant la
 *         lecture des bits (~5ms max).
 */
DHT11_Status_t dht11_read(DHT11_t *dev);

/**
 * @brief  Retourne la derniere humidite lue (%).
 * @param  dev  Pointeur vers DHT11_t (non NULL)
 * @return humidite en % ou 0 si dev == NULL
 */
uint8_t dht11_get_humidity(const DHT11_t *dev);

/**
 * @brief  Retourne la derniere temperature lue (degC).
 * @param  dev  Pointeur vers DHT11_t (non NULL)
 * @return temperature en degC ou 0 si dev == NULL
 */
uint8_t dht11_get_temperature(const DHT11_t *dev);

#endif /* DHT11_H */