/**
 * @file    dht11.c
 * @brief   Driver capteur temperature/humidite DHT11
 *
 * Protocole 1-wire proprietaire DHT11 implemente en polling
 * avec comptage de cycles CPU pour mesurer les durees des bits.
 *
 * Timing base sur HSI 16 MHz — pas de timer hardware requis.
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#include "dht11.h"
#include "systick.h"
#include "log.h"

/* ------------------------------------------------------------------
 * Helpers GPIO inline
 * ------------------------------------------------------------------ */

/**
 * @brief  Configure PA1 en sortie output push-pull.
 */
static void pin_output(void)
{
    DHT11_GPIOA_MODER &= ~(3UL << (DHT11_PIN * 2U));
    DHT11_GPIOA_MODER |=  (1UL << (DHT11_PIN * 2U));
}

/**
 * @brief  Configure PA1 en entree (input floating).
 */
static void pin_input(void)
{
    DHT11_GPIOA_MODER &= ~(3UL << (DHT11_PIN * 2U));
}

/**
 * @brief  Met PA1 a l'etat HIGH.
 */
static void pin_high(void)
{
    DHT11_GPIOA_ODR |= (1UL << DHT11_PIN);
}

/**
 * @brief  Met PA1 a l'etat LOW.
 */
static void pin_low(void)
{
    DHT11_GPIOA_ODR &= ~(1UL << DHT11_PIN);
}

/**
 * @brief  Lit l'etat de PA1.
 * @return 1 si HIGH, 0 si LOW
 */
static uint8_t pin_read(void)
{
    return (uint8_t)((DHT11_GPIOA_IDR >> DHT11_PIN) & 1UL);
}

/* ------------------------------------------------------------------
 * Delai microsecondes par comptage CPU
 * A 16 MHz HSI : 1 iteration ~ 4 cycles -> ~0.25 us
 * Pour N us : N * 4 iterations
 * ------------------------------------------------------------------ */

static void delay_us(uint32_t us)
{
    volatile uint32_t count = us * 4UL;
    while (count--) { __asm__("nop"); }
}

/* ------------------------------------------------------------------
 * Attente avec timeout
 * Retourne le nombre d'iterations ecoulees, ou 0 si timeout
 * ------------------------------------------------------------------ */

/**
 * @brief  Attend que PA1 passe a l'etat attendu, avec timeout.
 *
 * @param  level    Etat attendu : 0 = LOW, 1 = HIGH
 * @param  timeout  Nombre max d'iterations
 * @return Iterations ecoulees (> 0) ou 0 si timeout
 */
static uint32_t wait_level(uint8_t level, uint32_t timeout)
{
    uint32_t count = 0U;

    while (pin_read() != level)
    {
        if (++count >= timeout)
            return 0U;
        __asm__("nop");
    }

    return count;
}

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

void dht11_init(void)
{
    /* Activer horloge GPIOA (deja active par uart, securite) */
    DHT11_RCC_AHB1ENR |= DHT11_RCC_GPIOAEN;

    volatile uint32_t dummy = DHT11_RCC_AHB1ENR;
    (void)dummy;

    /* PA1 en output push-pull, idle HIGH */
    pin_output();
    pin_high();

    /* Attente stabilisation capteur apres power-on : 1 seconde */
    delay_ms(1000U);

    LOG_INFO("DHT11 : initialise sur PA1");
}

DHT11_Status_t dht11_read(DHT11_t *dev)
{
    uint8_t  data[5] = {0U, 0U, 0U, 0U, 0U};
    uint8_t  i;
    uint8_t  bit;
    uint32_t count;
    uint32_t high_count;

    if (dev == 0) return DHT11_ERR_PARAM;

    /* ----------------------------------------------------------
     * 1. Signal START : MCU tire DATA LOW pendant 18 ms
     *    puis relache HIGH
     * ---------------------------------------------------------- */
    pin_output();
    pin_low();
    delay_ms(DHT11_START_LOW_MS);

    pin_high();
    delay_us(DHT11_START_HIGH_US);

    /* Passer en input pour ecouter la reponse du DHT11 */
    pin_input();

    /* ----------------------------------------------------------
     * 2. Reponse DHT11 : LOW ~80 us puis HIGH ~80 us
     * ---------------------------------------------------------- */

    /* Attendre le front descendant (DHT11 tire LOW) */
    count = wait_level(0U, DHT11_TIMEOUT_US * 4UL);
    if (count == 0U)
    {
        LOG_ERROR("DHT11 : timeout reponse LOW");
        return DHT11_ERR_TIMEOUT;
    }

    /* Attendre le front montant (DHT11 relache HIGH) */
    count = wait_level(1U, DHT11_TIMEOUT_US * 4UL);
    if (count == 0U)
    {
        LOG_ERROR("DHT11 : timeout reponse HIGH");
        return DHT11_ERR_TIMEOUT;
    }

    /* Attendre fin du HIGH de reponse */
    count = wait_level(0U, DHT11_TIMEOUT_US * 4UL);
    if (count == 0U)
    {
        LOG_ERROR("DHT11 : timeout fin reponse");
        return DHT11_ERR_TIMEOUT;
    }

    /* ----------------------------------------------------------
     * 3. Lecture des 40 bits
     *    Chaque bit : LOW ~50 us + HIGH (26us='0', 70us='1')
     * ---------------------------------------------------------- */
    for (i = 0U; i < DHT11_DATA_BITS; i++)
    {
        /* Attendre fin du LOW (~50 us) */
        count = wait_level(1U, DHT11_TIMEOUT_US * 4UL);
        if (count == 0U)
        {
            LOG_ERROR("DHT11 : timeout bit LOW");
            return DHT11_ERR_TIMEOUT;
        }

        /* Mesurer la duree du HIGH pour distinguer '0' et '1' */
        high_count = 0U;
        while (pin_read() == 1U)
        {
            high_count++;
            __asm__("nop");
            if (high_count >= DHT11_TIMEOUT_US * 4UL)
            {
                LOG_ERROR("DHT11 : timeout bit HIGH");
                return DHT11_ERR_TIMEOUT;
            }
        }

        /* Seuil : si high_count > DHT11_BIT_THRESHOLD*4 -> bit '1' */
        bit = (high_count > (DHT11_BIT_THRESHOLD * 4UL)) ? 1U : 0U;

        /* Stocker le bit dans le bon octet (MSB en premier) */
        data[i / 8U] <<= 1U;
        data[i / 8U] |= bit;
    }

    /* ----------------------------------------------------------
     * 4. Verification du checksum
     *    checksum = data[0] + data[1] + data[2] + data[3]
     * ---------------------------------------------------------- */
    uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);

    if (checksum != data[4])
    {
        LOG_ERROR("DHT11 : checksum invalide");
        LOG_DEBUG_INT("  calcule  = ", (int32_t)checksum);
        LOG_DEBUG_INT("  recu     = ", (int32_t)data[4]);
        return DHT11_ERR_CHECKSUM;
    }

    /* ----------------------------------------------------------
     * 5. Extraction des donnees
     *    data[0] = humidite entier
     *    data[2] = temperature entier
     * ---------------------------------------------------------- */
    dev->humidity    = data[0];
    dev->temperature = data[2];

    LOG_DEBUG_INT("DHT11 humidite    = ", (int32_t)dev->humidity);
    LOG_DEBUG_INT("DHT11 temperature = ", (int32_t)dev->temperature);

    return DHT11_OK;
}

uint8_t dht11_get_humidity(const DHT11_t *dev)
{
    if (dev == 0) return 0U;
    return dev->humidity;
}

uint8_t dht11_get_temperature(const DHT11_t *dev)
{
    if (dev == 0) return 0U;
    return dev->temperature;
}