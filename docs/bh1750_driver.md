# Driver BH1750FVI — Capteur de luminosité I2C

## Description

Le **BH1750FVI** est un capteur de luminosité numérique communiquant via le protocole **I2C**.
Il mesure l'intensité lumineuse ambiante en **lux** avec une résolution de 1 lx sur une plage de 0 à 65535 lx.

---

## Caractéristiques

| Paramètre | Valeur |
|-----------|--------|
| Modèle | BH1750FVI |
| Mesure | Intensité lumineuse |
| Unité | Lux (lx) |
| Plage | 0 — 65535 lx |
| Résolution | 1 lx (mode haute résolution) |
| Protocole | I2C |
| Tension | 3.3V / 5V |
| Adresse I2C | `0x23` (ADD=GND) ou `0x5C` (ADD=VCC) |
| Temps de mesure | 120 — 180 ms (mode haute résolution) |

---

## Connexion matérielle

| BH1750 | STM32F407VG | Description |
|--------|-------------|-------------|
| VCC | 3.3V | Alimentation |
| GND | GND | Masse |
| SDA | PB7 (I2C1_SDA, AF4) | Données |
| SCL | PB6 (I2C1_SCL, AF4) | Horloge |
| ADD | GND | Adresse I2C = 0x23 |

### Résistances pull-up

Le bus I2C nécessite des résistances pull-up externes :
3.3V ──┬── 4.7kΩ ──── SDA (PB7)
└── 4.7kΩ ──── SCL (PB6)

> Sans pull-up, le bus I2C ne fonctionne pas !

---

## Configuration I2C1

| Paramètre | Valeur | Calcul |
|-----------|--------|--------|
| APB1 clock | 16 MHz (HSI) | - |
| Mode | Standard | 100 kHz |
| CCR | 80 | 16MHz / (2 × 100kHz) |
| TRISE | 17 | (1000ns / 62.5ns) + 1 |
| GPIO | PB6/PB7 | AF4, open-drain, pull-up |

---

## Architecture du driver
bh1750.h / bh1750.c
│
├── Fonctions privees I2C
│   ├── i2c_wait_sr1()     — attente flag SR1 avec timeout
│   ├── i2c_start()        — condition START
│   ├── i2c_stop()         — condition STOP
│   ├── i2c_send_addr()    — envoi adresse + R/W
│   ├── i2c_write_byte()   — envoi octet
│   └── i2c_bus_recovery() — recuperation bus bloque
│
└── Interface publique
├── bh1750_init()      — initialisation I2C + capteur
└── bh1750_read_lux()  — lecture luminosite en lux

---

## Interface publique

### `bh1750_init()`

```c
void bh1750_init(void);
```

Initialise I2C1 et le capteur BH1750.

**Séquence :**
1. Recovery préventive du bus (9 clock pulses)
2. Configuration GPIO PB6/PB7 en AF4 open-drain pull-up
3. Configuration I2C1 : 100 kHz, APB1 = 16 MHz
4. Envoi commande `POWER_ON (0x01)`

---

### `bh1750_read_lux()`

```c
uint16_t bh1750_read_lux(void);
```

Déclenche une mesure et retourne la luminosité en lux.

**Séquence :**
1. Envoi commande `ONE_TIME_H_RES_MODE (0x20)`
2. Attente 180 ms (temps de mesure haute résolution)
3. Lecture 2 octets MSB + LSB
4. Conversion : `lux = raw / 1.2`

**Retour :** Valeur en lux (0 — 65535), `0` en cas d'erreur I2C

---

## Protocole I2C — Séquences

### Envoi commande (écriture)
START → ADDR(0x23, W) → ACK → CMD → ACK → STOP

### Lecture résultat
START → ADDR(0x23, R) → ACK → MSB → ACK → LSB → NACK → STOP

### Séquence STM32F4 pour 2 octets

Activer ACK
Clear ADDR (lire SR1 + SR2)
Attendre RXNE → lire MSB
Désactiver ACK
Programmer STOP
Attendre RXNE → lire LSB


---

## Commandes BH1750

| Commande | Code | Description |
|----------|------|-------------|
| `POWER_ON` | `0x01` | Mise sous tension |
| `RESET` | `0x07` | Reset registre données |
| `CONT_H_RES_MODE` | `0x10` | Mesure continue haute résolution |
| `ONE_TIME_H_RES_MODE` | `0x20` | Mesure unique haute résolution |

> Le driver utilise `ONE_TIME_H_RES_MODE` — une mesure est déclenchée à chaque appel de `bh1750_read_lux()`.

---

## Conversion lux
raw = (MSB << 8) | LSB
lux = raw / 1.2 = raw × 10 / 12

**Exemple :**
raw = 105
lux = 105 × 10 / 12 = 87 lux

---

## Gestion des erreurs

### Bus I2C bloqué

Si le bus est bloqué (SDA LOW), la fonction `i2c_bus_recovery()` :
1. Désactive I2C1
2. Configure PB6/PB7 en GPIO output
3. Génère 9 impulsions manuelles sur SCL
4. Envoie une condition STOP manuelle
5. Reconfigure PB6/PB7 en AF4
6. Réinitialise I2C1 complètement

### Timeout I2C

Toutes les attentes I2C ont un timeout de `10000` itérations.
En cas de timeout, la fonction retourne `0`.

---

## Utilisation dans main.c

```c
#include "bh1750.h"

int main(void)
{
    bh1750_init();

    while (1)
    {
        uint16_t lux = bh1750_read_lux();
        /* utiliser lux */
        delay_ms(200);
    }
}
```

---

## Résultats de test

| Condition | Valeur mesurée |
|-----------|----------------|
| Bureau éclairé | ~87 — 100 lux |
| Lumière directe | > 1000 lux |
| Obscurité | ~0 lux |

---

## Fichiers

| Fichier | Description |
|---------|-------------|
| `firmware/include/bh1750.h` | Déclarations, registres, interface publique |
| `firmware/src/bh1750.c` | Implémentation complète |
| `docs/bh1750_driver.md` | Documentation (ce fichier) |