/**
 * @file    main.c
 * @brief   Point d'entree principal — STM32F407VG-Discovery
 *
 * Gestion d'une machine a etats pour la communication UART avec le PC :
 *
 *   WAIT_SYNC  : attente de la commande de synchronisation '#?#'
 *   IDLE       : connexion etablie, attente du demarrage du stream
 *   STREAMING  : envoi periodique des donnees capteurs via UART DMA
 *
 * Protocole de communication :
 *   PC -> STM32 : '#?#'  -> reponse '#!#4#'          (sync, 4 canaux)
 *   PC -> STM32 : '#A#'  -> debut du stream
 *   PC -> STM32 : '#S#'  -> arret du stream
 *   PC -> STM32 : '#P#'  -> deconnexion
 *   STM32 -> PC : '#D#val1#val2#val3#val4#val5#\n'   (trame de donnees)
 *
 * Indicateurs LED :
 *   WAIT_SYNC  : LED orange allumee
 *   IDLE       : LED bleue allumee
 *   STREAMING  : LED verte allumee
 *   default    : LED rouge allumee
 *
 * Auteur  : z_benakka193
 * Projet  : embedded-data-logger
 */

#include <stdint.h>
#include <string.h>

#include "gpio.h"
#include "systick.h"
#include "uart.h"
#include "log.h"
#include "iwdg.h"
#include "dht11.h"

/* ------------------------------------------------------------------
 * Constantes
 * ------------------------------------------------------------------ */

#define TX_BUFFER_SIZE  64U   /* Taille du buffer de transmission UART */
#define NB_CHANNELS     4U    /* Nombre de canaux de donnees envoyes au PC */


static DHT11_t dht11;


/* ------------------------------------------------------------------
 * Machine a etats
 * ------------------------------------------------------------------ */

/**
 * @brief Etats possibles du systeme de communication.
 */
typedef enum
{
    WAIT_SYNC = 0,   /**< Attente de la commande de synchronisation */
    IDLE      = 1,   /**< Connexion etablie, stream arrete           */
    STREAMING = 2    /**< Envoi periodique des donnees capteurs       */
} SystemState_t;

/* ------------------------------------------------------------------
 * Variables globales
 * ------------------------------------------------------------------ */

static SystemState_t current_state = WAIT_SYNC;
static char          last_cmd      = 'S';
static char          tx_buffer[TX_BUFFER_SIZE];

/* Valeurs fixes des canaux de donnees */
static int val1 = 0;    /* Canal 0 : valeur fixe */
static int val2 = 0;    /* Canal 1 : valeur fixe */
static int val3 = 0;   /* Canal 2 : valeur fixe */
static int val4 = 0;   /* Canal 3 : valeur fixe */
static int val5 = 0;      /* Longueur totale des chiffres ASCII (integrite) */

/* ------------------------------------------------------------------
 * Fonctions utilitaires
 * ------------------------------------------------------------------ */

/**
 * @brief  Compte le nombre de chiffres decimaux d'un entier non signe.
 * @param  n : valeur a analyser
 * @return Nombre de chiffres (minimum 1 pour n=0)
 */
static int count_digits(unsigned long n)
{
    if (n == 0) return 1;
    int count = 0;
    while (n > 0) { count++; n /= 10; }
    return count;
}

/**
 * @brief  Convertit un entier non signe en chaine de caracteres.
 *
 * Ecrit les chiffres decimaux de 'n' dans 'buf' et retourne
 * le nombre de caracteres ecrits (sans le '\0').
 *
 * @param  buf  : buffer de destination
 * @param  n    : valeur a convertir
 * @return Nombre de caracteres ecrits
 */
static int uint_to_str(char *buf, unsigned long n)
{
    char    tmp[12];
    int     i = 0;
    int     j = 0;

    if (n == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }

    /* Remplissage en sens inverse */
    while (n > 0)
    {
        tmp[i++] = '0' + (char)(n % 10);
        n /= 10;
    }

    /* Inversion */
    while (i > 0)
        buf[j++] = tmp[--i];

    buf[j] = '\0';
    return j;
}

/**
 * @brief  Construit la trame de donnees dans tx_buffer sans snprintf.
 *
 * Format : #D#val1#val2#val3#val4#val5#\n
 *
 * @return Longueur de la trame construite, ou 0 si depassement buffer.
 */
static int build_frame(void)
{
    char  *p   = tx_buffer;
    char  *end = tx_buffer + TX_BUFFER_SIZE - 1; /* Reserve '\0' */
    int    len = 0;

    /* Prefixe */
    const char *prefix = "#D#";
    while (*prefix && p < end) { *p++ = *prefix++; }

    /* val1 */
    len = uint_to_str(p, (unsigned long)val1);
    p  += len;
    if (p < end) *p++ = '#';

    /* val2 */
    len = uint_to_str(p, (unsigned long)val2);
    p  += len;
    if (p < end) *p++ = '#';

    /* val3 */
    len = uint_to_str(p, (unsigned long)val3);
    p  += len;
    if (p < end) *p++ = '#';

    /* val4 */
    len = uint_to_str(p, (unsigned long)val4);
    p  += len;
    if (p < end) *p++ = '#';

    /* val5 */
    len = uint_to_str(p, (unsigned long)val5);
    p  += len;
    if (p < end) *p++ = '#';

    /* Suffixe */
    if (p < end) *p++ = '\n';

    *p = '\0';

    /* Verification depassement */
    if (p >= end) return 0;

    return (int)(p - tx_buffer);
}

/* ------------------------------------------------------------------
 * Gestion des LEDs par etat
 * ------------------------------------------------------------------ */

/**
 * @brief  Met a jour les LEDs selon l'etat courant de la machine.
 *
 * Indicateurs visuels :
 *   WAIT_SYNC  -> LED orange (en attente de connexion)
 *   IDLE       -> LED bleue  (connecte, stream arrete)
 *   STREAMING  -> LED verte  (stream actif)
 *   default    -> LED rouge  (etat inconnu / erreur)
 *
 * @param  state : etat courant du systeme
 */
static void update_leds(SystemState_t state)
{
    gpio_led_off(LED_GREEN);
    gpio_led_off(LED_ORANGE);
    gpio_led_off(LED_RED);
    gpio_led_off(LED_BLUE);

    switch (state)
    {
        case STREAMING:  gpio_led_on(LED_GREEN);  break;
        case IDLE:       gpio_led_on(LED_BLUE);   break;
        case WAIT_SYNC:  gpio_led_on(LED_ORANGE); break;
        default:         gpio_led_on(LED_RED);    break;
    }
}

/* ------------------------------------------------------------------
 * Gestionnaire des commandes UART recues
 * ------------------------------------------------------------------ */

/**
 * @brief  Traite un caractere de commande recu depuis le PC.
 *
 * Transitions de la machine a etats :
 *   WAIT_SYNC + '?' -> IDLE       (sync reussie)
 *   IDLE      + 'A' -> STREAMING  (demarrage stream)
 *   IDLE      + 'P' -> WAIT_SYNC  (deconnexion)
 *   STREAMING + 'S' -> IDLE       (arret stream)
 *   STREAMING + 'P' -> WAIT_SYNC  (deconnexion)
 *
 * @param  cmd : caractere de commande recu ('?', 'A', 'S', 'P')
 */
static void handle_uart_command(char cmd)
{
    switch (current_state)
    {
        case WAIT_SYNC:
            if (cmd == '?')
            {
                uart_send_string("#!#4#\r\n");
                LOG_INFO("Sync OK -> IDLE");
                current_state = IDLE;
            }
            break;

        case IDLE:
            if (cmd == 'A')
            {
                LOG_INFO("Start stream -> STREAMING");
                current_state = STREAMING;
            }
            else if (cmd == 'P')
            {
                uart_send_string("#DISCONNECTED#\r\n");
                LOG_INFO("Disconnect -> WAIT_SYNC");
                current_state = WAIT_SYNC;
            }
            break;

        case STREAMING:
            if (cmd == 'S')
            {
                uart_send_string("#STOP#\r\n");
                LOG_INFO("Stop stream -> IDLE");
                current_state = IDLE;
            }
            else if (cmd == 'P')
            {
                uart_send_string("#DISCONNECTED#\r\n");
                LOG_INFO("Disconnect -> WAIT_SYNC");
                current_state = WAIT_SYNC;
            }
            break;

        default:
            break;
    }
}

/* ------------------------------------------------------------------
 * Envoi d'une trame de donnees
 * ------------------------------------------------------------------ */

/**
 * @brief  Construit et envoie une trame de donnees via UART.
 *
 * Format : #D#val1#val2#val3#val4#val5#\n
 *
 * val5 = somme du nombre de chiffres decimaux de val1..val4
 *        utilisee comme controle d'integrite de la trame.
 */
static void send_data_frame(void)
{
    DHT11_Status_t ret = dht11_read(&dht11);
    switch (ret)
    {
        case DHT11_OK:
            val1 = (int)dht11_get_humidity(&dht11);
            val2 = (int)dht11_get_temperature(&dht11);
            LOG_INFO("DHT11 lecture OK");
            break;

        case DHT11_ERR_TIMEOUT:
            LOG_ERROR("DHT11 : timeout — verifier le cablage PA1");
            break;

        case DHT11_ERR_CHECKSUM:
            LOG_ERROR("DHT11 : checksum invalide — donnees corrompues");
            break;

        case DHT11_ERR_PARAM:
            LOG_ERROR("DHT11 : parametre invalide — pointeur NULL");
            break;

        default:
            LOG_ERROR("DHT11 : erreur inconnue");
            break;
    }

    /* Calcul de val5 : integrite de la trame */
    val5 = count_digits((unsigned long)val1)
         + count_digits((unsigned long)val2)
         + count_digits((unsigned long)val3)
         + count_digits((unsigned long)val4);

    /* Construction et envoi de la trame */
    if (build_frame() == 0)
    {
        uart_send_string("#E#OVF#\n");
        LOG_INFO("Erreur : trame trop longue");
        return;
    }

    uart_send_string(tx_buffer);
    val5 = 0;
}


/* ------------------------------------------------------------------
 * Point d'entree
 * ------------------------------------------------------------------ */

int main(void)
{
    char rx_char = 0;

    /* Initialisation des peripheriques */
    gpio_init();
    systick_init();
    uart_init(UART_BAUD_115200);
    /* Initialisaion de capteur dht11 */
    dht11_init();
    /* Initialisation du watchdog — timeout 2 secondes */
    //iwdg_init(2000);

    LOG_INFO("=== embedded-data-logger ===");
    LOG_INFO("En attente de synchronisation...");

    /* Etat initial : WAIT_SYNC -> LED orange */
    update_leds(current_state);

    while (1)
    {

        /* --- Reception UART --- */
        rx_char = uart_receive_char();

        if (rx_char != 0 || last_cmd == 'A')
        {
            if (rx_char != 0)
                last_cmd = rx_char;

            handle_uart_command(last_cmd);
            update_leds(current_state);
        }

        /* --- Actions selon l'etat courant --- */
        switch (current_state)
        {
            case STREAMING:
                send_data_frame();
                delay_ms(10);
                break;

            case IDLE:
            case WAIT_SYNC:
            default:
                break;
        }
        /* Rafraichir le watchdog — empeche le reset */
        iwdg_refresh();
    }
}
