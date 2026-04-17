# Protocole de communication UART

## Paramètres de la liaison série

| Paramètre       | Valeur                  |
|-----------------|-------------------------|
| Interface       | USART2 (PA2=TX, PA3=RX) |
| Baudrate        | 115200                  |
| Bits de données | 8                       |
| Parité          | Aucune                  |
| Bits de stop    | 1                       |
| Mode reception  | Non bloquant            |

---

## Machine à états STM32
                '?'                  'A'
WAIT_SYNC  ─────────────►  IDLE  ─────────────►  STREAMING
    ▲                       │  ▲                     │
    │         'P'           │  │        'S'          │
    └───────────────────────┘  └─────────────────────┘
              'P'

| Etat      | LED    | Description                        |
|-----------|--------|------------------------------------|
| WAIT_SYNC | Orange | Attente de la commande de sync     |
| IDLE      | Bleue  | Connecte, stream arrete            |
| STREAMING | Verte  | Envoi periodique des donnees       |
| default   | Rouge  | Erreur / etat inconnu              |

---

## Commandes PC → STM32

| Commande | Trame    | Description                        |
|----------|----------|------------------------------------|
| Sync     | `#?#\n`  | Demande de synchronisation         |
| Start    | `#A#\n`  | Demarrage du flux de donnees       |
| Stop     | `#S#\n`  | Arret du flux de donnees           |
| Disconnect | `#P#\n` | Deconnexion                       |

---

## Reponses STM32 → PC

| Situation  | Trame               | Description                    |
|------------|---------------------|--------------------------------|
| Sync OK    | `#!#4#\r\n`         | Sync reussie, 4 canaux actifs  |
| Donnees    | `#D#v1#v2#v3#v4#v5#\n` | Trame de donnees            |
| Stop OK    | `#STOP#\r\n`        | Stream arrete                  |
| Disconnect | `#DISCONNECTED#\r\n`| Deconnexion confirmee          |
| Erreur     | `#E#OVF#\n`         | Depassement buffer TX          |

---

## Format de la trame de données
#D#val1#val2#val3#val4#val5#\n

| Champ | Description                                      |
|-------|--------------------------------------------------|
| `D`   | Marqueur de trame de donnees                     |
| `val1`| Canal 0 — increment 0 → 2000 (pas de 2)         |
| `val2`| Canal 1 — decrement 2000 → 0 (pas de 2)         |
| `val3`| Canal 2 — valeur fixe (1282)                     |
| `val4`| Canal 3 — valeur fixe (7677)                     |
| `val5`| Controle integrite = somme des chiffres decimaux |

### Exemple
#D#206#157#1282#7677#14#\n

| val1 | val2 | val3 | val4 | val5 (integrite)              |
|------|------|------|------|-------------------------------|
| 206  | 157  | 1282 | 7677 | 3+3+4+4 = 14                  |

---

## Format CSV enregistré

```csv
timestamp,val1,val2,val3,val4
1694614257.123,206,157,1282,7677
1694614257.133,208,155,1282,7677
```