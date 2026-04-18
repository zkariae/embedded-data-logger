/**
 * @file    iwdg.c
 * @brief   Driver IWDG (Independent Watchdog) — STM32F407
 *
 * Formule du timeout :
 *   timeout (ms) = (prescaler × reload) / LSI_freq
 *   LSI_freq = 32000 Hz
 *
 * Exemple pour timeout = 2000ms :
 *   prescaler = 32  (IWDG_PR_DIV32)
 *   reload    = (2000 × 32000) / (32 × 1000) = 2000
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#include "iwdg.h"

/* Frequence du LSI en Hz */
#define LSI_FREQ_HZ     32000UL

/* Valeur maximale du registre RLR */
#define IWDG_RLR_MAX    0x0FFFUL

/* ------------------------------------------------------------------
 * Fonctions privées
 * ------------------------------------------------------------------ */

/**
 * @brief  Calcule le prescaler et le reload pour un timeout donne.
 *
 * Cherche le prescaler le plus petit qui permet d'obtenir
 * une valeur de reload <= IWDG_RLR_MAX (4095).
 *
 * @param  timeout_ms : timeout souhaite en millisecondes
 * @param  pr         : prescaler trouve (sortie)
 * @param  rlr        : valeur de reload trouvee (sortie)
 */
static void iwdg_compute_params(uint32_t timeout_ms,
                                 uint32_t *pr,
                                 uint32_t *rlr)
{
    /* Valeurs de prescaler : 4, 8, 16, 32, 64, 128, 256 */
    uint32_t prescalers[] = {4, 8, 16, 32, 64, 128, 256};
    uint32_t pr_bits[]    = {
        IWDG_PR_DIV4,  IWDG_PR_DIV8,   IWDG_PR_DIV16,
        IWDG_PR_DIV32, IWDG_PR_DIV64,  IWDG_PR_DIV128,
        IWDG_PR_DIV256
    };

    for (int i = 0; i < 7; i++)
    {
        /* reload = (timeout_ms * LSI_freq) / (prescaler * 1000) */
        uint32_t reload = (timeout_ms * (LSI_FREQ_HZ / 1000)) / prescalers[i];

        if (reload <= IWDG_RLR_MAX)
        {
            *pr  = pr_bits[i];
            *rlr = reload;
            return;
        }
    }

    /* Timeout trop grand : utiliser valeurs maximales */
    *pr  = IWDG_PR_DIV256;
    *rlr = IWDG_RLR_MAX;
}

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

/**
 * @brief  Initialise et demarre le watchdog IWDG.
 *
 * Sequence d'initialisation :
 *   1. Demarrer le watchdog (KR = 0xCCCC)
 *   2. Autoriser l'ecriture des registres PR et RLR (KR = 0x5555)
 *   3. Configurer le prescaler (PR)
 *   4. Configurer la valeur de reload (RLR)
 *   5. Attendre la mise a jour des registres
 *   6. Rafraichir immediatement (KR = 0xAAAA)
 *
 * @param  timeout_ms : timeout en millisecondes
 */
void iwdg_init(uint32_t timeout_ms)
{
    uint32_t pr  = 0;
    uint32_t rlr = 0;

    /* Calcul des parametres */
    iwdg_compute_params(timeout_ms, &pr, &rlr);

    /* 1. Demarrer le watchdog */
    IWDG_KR = IWDG_KEY_START;

    /* 2. Autoriser l'ecriture de PR et RLR */
    IWDG_KR = IWDG_KEY_ACCESS;

    /* 3. Configurer le prescaler */
    IWDG_PR = pr;

    /* 4. Configurer la valeur de reload */
    IWDG_RLR = rlr;

    /* 5. Attendre la mise a jour des registres */
    while (IWDG_SR != 0)
        ;

    /* 6. Rafraichir immediatement */
    IWDG_KR = IWDG_KEY_REFRESH;
}

/**
 * @brief  Rafraichit le compteur du watchdog.
 *
 * Cette fonction doit etre appelee periodiquement dans la boucle
 * principale avant l'expiration du timeout pour eviter un reset.
 */
void iwdg_refresh(void)
{
    IWDG_KR = IWDG_KEY_REFRESH;
}