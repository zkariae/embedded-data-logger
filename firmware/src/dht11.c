/**
 * @file    dht11.c
 * @brief   Driver capteur temperature/humidite DHT11
 *
 * Timing base sur TIM2 configure a 1 MHz (1 tick = 1 us).
 * Les interruptions sont desactivees pendant la lecture des bits
 * pour garantir la precision du protocole 1-wire.
 *
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#include "dht11.h"
#include "systick.h"
#include "log.h"

/* ------------------------------------------------------------------
 * TIM2 — init et helpers
 * ------------------------------------------------------------------ */

/**
 * @brief  Configure TIM2 en compteur libre 32 bits a 1 MHz.
 *         Appele uniquement par dht11_init().
 *
 * @note   PCLK1 = 16 MHz (HSI) -> PSC = 15 -> 16/(15+1) = 1 MHz
 *         ARR = 0xFFFFFFFF : debordement toutes les ~4295 secondes
 */
static void tim2_init(void)
{
    /* Activer l'horloge TIM2 sur APB1 */
    DHT11_RCC_APB1ENR |= DHT11_RCC_TIM2EN;

    /* Dummy read : garantit la prise en compte du bit avant ecriture */
    volatile uint32_t dummy = DHT11_RCC_APB1ENR;
    (void)dummy;

    TIM2_CR1 = 0U;                  /* Stopper le compteur             */
    TIM2_PSC = 15U;                 /* PSC = 15 -> 1 tick = 1 us       */
    TIM2_ARR = 0xFFFFFFFFUL;        /* Periode max (32 bits)           */
    TIM2_EGR = TIM2_EGR_UG;        /* Forcer le rechargement PSC/ARR  */
    TIM2_CNT = 0U;                  /* Remettre le compteur a zero     */
    TIM2_CR1 = TIM2_CR1_CEN;       /* Demarrer le compteur            */
}

/**
 * @brief  Attend 'us' microsecondes via TIM2.
 * @param  us  Duree en microsecondes
 *
 * @warning Ne pas appeler si les interruptions sont desactivees
 *          ET si delay_ms() est la seule source de timing —
 *          TIM2 etant materiel, cette fonction reste valide dans
 *          tous les contextes (contrairement a delay_ms).
 */
static void delay_us(uint32_t us)
{
    uint32_t start = TIM2_CNT;
    while ((TIM2_CNT - start) < us) { }
}

/**
 * @brief  Attend que PC0 atteigne le niveau 'level'.
 *
 * @param  level       Niveau attendu : 0 (LOW) ou 1 (HIGH)
 * @param  timeout_us  Delai maximum en microsecondes
 *
 * @return Duree d'attente en microsecondes si le niveau est atteint
 *         DHT11_WAIT_TIMEOUT (0xFFFFFFFF) en cas de timeout
 *
 * [FIX 1] Retour DHT11_WAIT_TIMEOUT au lieu de 0 pour le timeout.
 *         0 est une duree legitime (niveau deja present a l'entree),
 *         et l'ancienne valeur 0 causait des faux positifs de timeout.
 */
static uint32_t wait_level(uint8_t level, uint32_t timeout_us)
{
    uint32_t start = TIM2_CNT;

    while (((DHT11_GPIOC_IDR >> DHT11_PIN) & 1UL) != (uint32_t)level)
    {
        if ((TIM2_CNT - start) >= timeout_us)
            return DHT11_WAIT_TIMEOUT;
    }

    return (TIM2_CNT - start);
}

/* ------------------------------------------------------------------
 * Helpers GPIO
 * ------------------------------------------------------------------ */

/**
 * @brief  Configure PC0 en sortie push-pull.
 */
static void pin_output(void)
{
    DHT11_GPIOC_MODER &= ~(3UL << (DHT11_PIN * 2U));
    DHT11_GPIOC_MODER |=  (1UL << (DHT11_PIN * 2U));  /* 01 = output */
}

/**
 * @brief  Configure PC0 en entree avec pull-up interne.
 *
 * [FIX 4] Le pull-up est indispensable quand il n'y a pas de
 *         resistance externe (4.7k) sur la ligne data. Sans pull-up,
 *         la ligne flotte et les lectures sont aleatoires.
 */
static void pin_input(void)
{
    /* Mode : input (00) */
    DHT11_GPIOC_MODER &= ~(3UL << (DHT11_PIN * 2U));

    /* Pull-up interne : PUPDR = 01 */
    DHT11_GPIOC_PUPDR &= ~(3UL << (DHT11_PIN * 2U));
    DHT11_GPIOC_PUPDR |=  (1UL << (DHT11_PIN * 2U));
}

/**
 * @brief  Met PC0 a l'etat haut (ODR).
 */
static void pin_high(void)
{
    DHT11_GPIOC_ODR |= (1UL << DHT11_PIN);
}

/**
 * @brief  Met PC0 a l'etat bas (ODR).
 */
static void pin_low(void)
{
    DHT11_GPIOC_ODR &= ~(1UL << DHT11_PIN);
}

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

/**
 * @brief  Initialise le driver DHT11.
 *
 * Sequence :
 *   1. Active l'horloge GPIOC
 *   2. Initialise TIM2 a 1 MHz
 *   3. Configure PC0 en sortie, HIGH (etat de repos du bus)
 *   4. Active le pull-up interne sur PC0
 *   5. Attend 1s pour laisser le DHT11 se stabiliser
 *
 * [FIX 5] tim2_init() est appelee ici uniquement.
 *         Ne pas l'appeler dans main() pour eviter un double appel
 *         qui remettrait TIM2_CNT a zero apres le demarrage.
 */
void dht11_init(void)
{
    /* Activer horloge GPIOC */
    DHT11_RCC_AHB1ENR |= DHT11_RCC_GPIOCEN;
    volatile uint32_t dummy = DHT11_RCC_AHB1ENR;
    (void)dummy;

    /* Timer 1 MHz */
    tim2_init();

    /* PC0 en sortie, etat HIGH (repos du bus 1-wire) */
    pin_output();
    pin_high();

    /* Pull-up interne — actif en input, configure des maintenant
     * pour etre pret lors du premier passage en mode input */
    DHT11_GPIOC_PUPDR &= ~(3UL << (DHT11_PIN * 2U));
    DHT11_GPIOC_PUPDR |=  (1UL << (DHT11_PIN * 2U));

    /* Attente stabilisation capteur (spec DHT11 : 1s apres power-on) */
    delay_ms(1000U);

    LOG_INFO("DHT11 : initialise sur PC0 (TIM2 1MHz)");
}

/**
 * @brief  Lit temperature et humidite depuis le DHT11.
 *
 * Protocole :
 *   1. START  : MCU force LOW 18ms puis HIGH 40us puis passe en input
 *   2. REPONSE: DHT11 tire LOW ~80us puis HIGH ~80us
 *   3. DONNEES: 40 bits, chaque bit = LOW 50us + HIGH (26us='0', 70us='1')
 *   4. CHECKSUM: data[4] == (data[0]+data[1]+data[2]+data[3]) & 0xFF
 *
 * @param  dev  Pointeur vers DHT11_t (non NULL)
 * @return DHT11_OK, DHT11_ERR_TIMEOUT, DHT11_ERR_CHECKSUM, DHT11_ERR_PARAM
 *
 * @note   Appeler au minimum toutes les 2 secondes (spec DHT11).
 */
DHT11_Status_t dht11_read(DHT11_t *dev)
{
    uint8_t  data[5] = {0U, 0U, 0U, 0U, 0U};
    uint8_t  i;
    uint8_t  checksum;
    uint32_t t_start;
    uint32_t duration;

    if (dev == 0) return DHT11_ERR_PARAM;

    /* ----------------------------------------------------------
     * 1. Signal START : LOW 18ms puis HIGH 40us
     * ---------------------------------------------------------- */
    pin_output();
    pin_low();
    delay_ms(DHT11_START_LOW_MS);   /* LOW 18ms (SysTick OK ici) */

    pin_high();
    delay_us(DHT11_START_HIGH_US);  /* HIGH 40us (TIM2) */

    /* Passer en entree (pull-up interne actif) */
    pin_input();

    /* [FIX 2] Ne pas verifier immediatement : la ligne est encore HIGH
     * (pull-up). Le DHT11 a besoin de 20-40us pour tirer LOW.
     * On laisse wait_level() gerer l'attente avec timeout. */

    /* Desactiver les interruptions : critique pour les timings us */
    __asm__ volatile ("cpsid i" ::: "memory");

    /* ----------------------------------------------------------
     * 2. Reponse DHT11
     *    [FIX 3] Sequence corrigee :
     *    - Attendre que DHT11 tire la ligne LOW (~80us)
     *    - Puis attendre le front montant (fin du LOW ~80us)
     *    - Puis attendre la fin du HIGH de reponse (~80us)
     * ---------------------------------------------------------- */

    /* Attendre front descendant (DHT11 tire LOW) */
    duration = wait_level(0U, DHT11_TIMEOUT_US);
    if (duration == DHT11_WAIT_TIMEOUT)
    {
        __asm__ volatile ("cpsie i" ::: "memory");
        LOG_ERROR("DHT11 : timeout — pas de reponse LOW (verifier cablage PC0)");
        return DHT11_ERR_TIMEOUT;
    }

    /* Attendre front montant (fin du LOW ~80us) */
    duration = wait_level(1U, DHT11_TIMEOUT_US);
    if (duration == DHT11_WAIT_TIMEOUT)
    {
        __asm__ volatile ("cpsie i" ::: "memory");
        LOG_ERROR("DHT11 : timeout reponse HIGH");
        return DHT11_ERR_TIMEOUT;
    }

    /* Attendre fin du HIGH de reponse (~80us) */
    duration = wait_level(0U, DHT11_TIMEOUT_US);
    if (duration == DHT11_WAIT_TIMEOUT)
    {
        __asm__ volatile ("cpsie i" ::: "memory");
        LOG_ERROR("DHT11 : timeout fin reponse HIGH");
        return DHT11_ERR_TIMEOUT;
    }

    /* ----------------------------------------------------------
     * 3. Lecture des 40 bits
     *
     * Chaque bit :
     *   - LOW  ~50us  : separateur inter-bit
     *   - HIGH ~26us  : bit '0'
     *   - HIGH ~70us  : bit '1'
     *   Seuil de decision : DHT11_BIT_THRESHOLD = 50us
     * ---------------------------------------------------------- */
    for (i = 0U; i < DHT11_DATA_BITS; i++)
    {
        /* Attendre la fin du LOW (~50us) -> debut du HIGH */
        duration = wait_level(1U, DHT11_TIMEOUT_US);
        if (duration == DHT11_WAIT_TIMEOUT)
        {
            __asm__ volatile ("cpsie i" ::: "memory");
            LOG_ERROR("DHT11 : timeout debut bit HIGH");
            return DHT11_ERR_TIMEOUT;
        }

        /* Mesurer la duree du HIGH pour distinguer '0' de '1' */
        t_start  = TIM2_CNT;
        duration = wait_level(0U, DHT11_TIMEOUT_US);
        if (duration == DHT11_WAIT_TIMEOUT)
        {
            __asm__ volatile ("cpsie i" ::: "memory");
            LOG_ERROR("DHT11 : timeout fin bit HIGH");
            return DHT11_ERR_TIMEOUT;
        }
        duration = TIM2_CNT - t_start;

        /* Decaler et inserer le bit (MSB en premier) */
        data[i / 8U] <<= 1U;
        if (duration > DHT11_BIT_THRESHOLD)
            data[i / 8U] |= 1U;  /* bit '1' */
        /* else : bit '0', pas d'action (bit deja a 0 apres shift) */
    }

    __asm__ volatile ("cpsie i" ::: "memory");

    /* ----------------------------------------------------------
     * 4. Verification checksum
     *    checksum = (data[0]+data[1]+data[2]+data[3]) & 0xFF
     * ---------------------------------------------------------- */
    checksum = (uint8_t)((data[0] + data[1] + data[2] + data[3]) & 0xFFU);

    if (checksum != data[4])
    {
        LOG_ERROR("DHT11 : checksum invalide");
        LOG_DEBUG_INT("  calcule = ", (int32_t)checksum);
        LOG_DEBUG_INT("  recu    = ", (int32_t)data[4]);
        return DHT11_ERR_CHECKSUM;
    }

    /* ----------------------------------------------------------
     * 5. Extraction des donnees
     *    data[0] = humidite integer
     *    data[1] = humidite decimal  (toujours 0 sur DHT11)
     *    data[2] = temperature integer
     *    data[3] = temperature decimal (toujours 0 sur DHT11)
     * ---------------------------------------------------------- */
    dev->humidity    = data[0];
    dev->temperature = data[2];

    LOG_DEBUG_INT("DHT11 humidite    = ", (int32_t)dev->humidity);
    LOG_DEBUG_INT("DHT11 temperature = ", (int32_t)dev->temperature);

    return DHT11_OK;
}

/**
 * @brief  Retourne la derniere humidite lue (%).
 */
uint8_t dht11_get_humidity(const DHT11_t *dev)
{
    if (dev == 0) return 0U;
    return dev->humidity;
}

/**
 * @brief  Retourne la derniere temperature lue (degC).
 */
uint8_t dht11_get_temperature(const DHT11_t *dev)
{
    if (dev == 0) return 0U;
    return dev->temperature;
}
