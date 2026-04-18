/**
 * @file    iwdg.h
 * @brief   Driver IWDG (Independent Watchdog) — STM32F407
 *
 * Le watchdog independant utilise l'horloge LSI (32 kHz) et
 * redémarre automatiquement le microcontroleur si la fonction
 * iwdg_refresh() n'est pas appelee avant le timeout.
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#ifndef IWDG_H
#define IWDG_H

#include <stdint.h>

/* ------------------------------------------------------------------
 * Adresses des registres IWDG
 * ------------------------------------------------------------------ */
#define IWDG_BASE   0x40003000UL

#define IWDG_KR     (*(volatile uint32_t *)(IWDG_BASE + 0x00UL))  /* Key register        */
#define IWDG_PR     (*(volatile uint32_t *)(IWDG_BASE + 0x04UL))  /* Prescaler register  */
#define IWDG_RLR    (*(volatile uint32_t *)(IWDG_BASE + 0x08UL))  /* Reload register     */
#define IWDG_SR     (*(volatile uint32_t *)(IWDG_BASE + 0x0CUL))  /* Status register     */

/* ------------------------------------------------------------------
 * Cles IWDG
 * ------------------------------------------------------------------ */
#define IWDG_KEY_REFRESH    0xAAAAUL   /* Rafraichir le compteur     */
#define IWDG_KEY_ACCESS     0x5555UL   /* Autoriser ecriture PR/RLR  */
#define IWDG_KEY_START      0xCCCCUL   /* Demarrer le watchdog       */

/* ------------------------------------------------------------------
 * Prescaler IWDG (LSI = 32 kHz)
 * ------------------------------------------------------------------ */
#define IWDG_PR_DIV4    0x00UL   /* Timeout max :  ~0.5s  */
#define IWDG_PR_DIV8    0x01UL   /* Timeout max :  ~1s    */
#define IWDG_PR_DIV16   0x02UL   /* Timeout max :  ~2s    */
#define IWDG_PR_DIV32   0x03UL   /* Timeout max :  ~4s    */
#define IWDG_PR_DIV64   0x04UL   /* Timeout max :  ~8s    */
#define IWDG_PR_DIV128  0x05UL   /* Timeout max :  ~16s   */
#define IWDG_PR_DIV256  0x06UL   /* Timeout max :  ~26s   */

/* ------------------------------------------------------------------
 * Interface publique
 * ------------------------------------------------------------------ */

/**
 * @brief  Initialise et demarre le watchdog avec un timeout en ms.
 * @param  timeout_ms : timeout en millisecondes (max ~26000ms)
 */
void iwdg_init(uint32_t timeout_ms);

/**
 * @brief  Rafraichit le watchdog (empeche le reset).
 *         Doit etre appele periodiquement avant le timeout.
 */
void iwdg_refresh(void);

#endif /* IWDG_H */