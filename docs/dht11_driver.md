# Driver DHT11 — Capteur de température et humidité

## Description

Le **DHT11** est un capteur numérique de température et d'humidité
communiquant via un protocole **1-wire** propriétaire.
Il mesure la température de **0 à 50°C** et l'humidité de **20 à 90% RH**.

---

## Caractéristiques
```
| Paramètre | Valeur |
|-----------|--------|
| Modèle | DHT11 |
| Mesure | Température + Humidité |
| Plage température | 0 — 50 °C |
| Précision température | ±2 °C |
| Plage humidité | 20 — 90 % RH |
| Précision humidité | ±5 % RH |
| Protocole | 1-wire propriétaire |
| Tension | 3.3V / 5V |
| Intervalle lecture | minimum 2 secondes |
```
---

## Connexion matérielle
```
| DHT11 | STM32F407VG | Description |
|-------|-------------|-------------|
| VCC | 3.3V | Alimentation |
| GND | GND | Masse |
| DATA | PC0 | Données 1-wire |
```
### Résistance pull-up
3.3V ──── 4.7kΩ ──── DATA (PC0)

> Le driver utilise le pull-up interne du STM32 si pas de résistance externe.

---

## Configuration GPIO
```
| Paramètre | Valeur |
|-----------|--------|
| Broche | PC0 |
| Mode output | Push-pull |
| Mode input | Pull-up interne |
| Timer | TIM2 (1 MHz — 1 tick = 1 µs) |
```
```
---

## Architecture du driver
dht11.h / dht11.c
│
├── Fonctions privees
│   ├── tim2_init()    — TIM2 1MHz pour timings µs
│   ├── delay_us()     — attente en microsecondes
│   ├── wait_level()   — attente niveau logique avec timeout
│   ├── pin_output()   — PC0 en sortie push-pull
│   ├── pin_input()    — PC0 en entree pull-up
│   ├── pin_high()     — PC0 = HIGH
│   └── pin_low()      — PC0 = LOW
│
└── Interface publique
├── dht11_init()         — initialisation GPIO + TIM2
├── dht11_read()         — lecture temperature + humidite
├── dht11_get_humidity() — retourne derniere humidite lue
└── dht11_get_temperature() — retourne derniere temperature lue

---
```

## Interface publique

### `dht11_init()`

```c
void dht11_init(void);
```

Initialise le driver DHT11.

**Séquence :**
1. Activation horloge GPIOC
2. Initialisation TIM2 à 1 MHz
3. Configuration PC0 en sortie HIGH (repos du bus)
4. Activation pull-up interne PC0
5. Attente 1 seconde (stabilisation capteur)

---

### `dht11_read()`

```c
DHT11_Status_t dht11_read(DHT11_t *dev);
```

Lit la température et l'humidité depuis le DHT11.

**Séquence :**
1. Signal START : LOW 18ms → HIGH 40µs → mode input
2. Réponse DHT11 : LOW ~80µs → HIGH ~80µs
3. Lecture 40 bits : LOW 50µs + HIGH (26µs='0', 70µs='1')
4. Vérification checksum

**Retour :**
```
| Code | Description |
|------|-------------|
| `DHT11_OK` | Lecture réussie |
| `DHT11_ERR_TIMEOUT` | Timeout — vérifier câblage |
| `DHT11_ERR_CHECKSUM` | Données corrompues |
| `DHT11_ERR_PARAM` | Pointeur NULL |
```
---

### `dht11_get_humidity()`

```c
uint8_t dht11_get_humidity(const DHT11_t *dev);
```

Retourne la dernière humidité lue en %.

---

### `dht11_get_temperature()`

```c
uint8_t dht11_get_temperature(const DHT11_t *dev);
```

Retourne la dernière température lue en °C.

---

## Protocole 1-wire DHT11

### Signal START
MCU         _______________                    ________
__| 18ms LOW      | 40µs HIGH _____|
DHT11                       ___________________
| 80µs LOW          |
80µs HIGH

### Format des bits
Bit '0' :  LOW 50µs + HIGH 26µs
Bit '1' :  LOW 50µs + HIGH 70µs
Seuil   :  > 50µs = '1', <= 50µs = '0'

```
### Trame de données (40 bits)
┌──────────┬──────────┬──────────┬──────────┬──────────┐
│  data[0] │  data[1] │  data[2] │  data[3] │  data[4] │
│ Hum. int │ Hum. dec │ Tmp. int │ Tmp. dec │ Checksum │
│  8 bits  │  8 bits  │  8 bits  │  8 bits  │  8 bits  │
└──────────┴──────────┴──────────┴──────────┴──────────┘
```

> Sur DHT11 : data[1] et data[3] sont toujours 0 (pas de décimales).

### Vérification checksum
checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF

---

## Timer TIM2

TIM2 est configuré en compteur libre 32 bits à **1 MHz** :
```
| Paramètre | Valeur | Calcul |
|-----------|--------|--------|
| PCLK1 | 16 MHz (HSI) | - |
| Prescaler | 15 | 16MHz / (15+1) = 1MHz |
| ARR | 0xFFFFFFFF | Débordement toutes les ~4295s |
| Résolution | 1 µs / tick | - |
```
> TIM2 est initialisé uniquement dans `dht11_init()` — ne pas l'appeler ailleurs.

---

## Corrections apportées (v2)
```
| Fix | Description |
|-----|-------------|
| **FIX 1** | `wait_level()` retourne `UINT32_MAX` en cas de timeout (evite faux positifs) |
| **FIX 2** | Suppression verification prematuree apres `pin_input()` |
| **FIX 3** | Sequence reponse DHT11 corrigee (LOW puis HIGH) |
| **FIX 4** | Pull-up interne PC0 active en mode input |
| **FIX 5** | `tim2_init()` privee — appelee uniquement dans `dht11_init()` |
```
---

## Utilisation dans main.c

```c
#include "dht11.h"

static DHT11_t dht11;

int main(void)
{
    dht11_init();

    while (1)
    {
        DHT11_Status_t ret = dht11_read(&dht11);
        if (ret == DHT11_OK)
        {
            uint8_t hum  = dht11_get_humidity(&dht11);
            uint8_t temp = dht11_get_temperature(&dht11);
        }
        delay_ms(2000);  /* Minimum 2s entre lectures */
    }
}
```

---

## Intégration avec BH1750

Dans `send_data_frame()`, le DHT11 est lu toutes les **2 secondes**
via `systick_get_tick()` pour synchroniser avec le BH1750 :

```c
static uint32_t last_dht11_ms = 0;
uint32_t        now_ms        = systick_get_tick();

if ((now_ms - last_dht11_ms) >= 2000)
{
    dht11_read(&dht11);
    last_dht11_ms = now_ms;
}
```

---

## Résultats de test
```
| Paramètre | Valeur mesurée |
|-----------|----------------|
| Température bureau | ~21 °C |
| Humidité bureau | ~52 % RH |
```
---

## Fichiers
```
| Fichier | Description |
|---------|-------------|
| `firmware/include/dht11.h` | Declarations, registres, interface publique |
| `firmware/src/dht11.c` | Implementation complete |
| `docs/dht11_driver.md` | Documentation (ce fichier) |
```