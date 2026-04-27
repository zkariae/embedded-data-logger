<div align="center">

# embedded-data-logger

![Build](https://gitlab.com/z_benakka193/embedded-data-logger/badges/main/pipeline.svg)
![Version](https://img.shields.io/badge/version-v3.0.0-green)
![License](https://img.shields.io/badge/license-MIT-blue)
![Python](https://img.shields.io/badge/python-3.10-blue)
![STM32](https://img.shields.io/badge/STM32-F407VG-red)
![DHT11](https://img.shields.io/badge/capteur-DHT11-orange)
![BH1750](https://img.shields.io/badge/capteur-BH1750-yellow)

Système embarqué de **collecte, visualisation et stockage cloud**
de données en temps réel via UART.

</div>


---

## Description

**embedded-data-logger** est un système embarqué complet permettant de :

- Collecter des données depuis une carte **STM32F407VG-Discovery** via **UART**
- Mesurer la **température** et l'**humidité** via le capteur **DHT11**
- Mesurer l'**intensité lumineuse** via le capteur **BH1750FVI**
- Visualiser les données en **temps réel** via une interface graphique Python
- Sauvegarder les données localement en **CSV**
- Stocker et visualiser les données dans le **cloud** via InfluxDB + Grafana

---

## Capteurs intégrés

| Capteur | Mesure | Broche | Protocole |
|---------|--------|--------|-----------|
| **DHT11** | Température + Humidité | PC0 | 1-Wire |
| **BH1750FVI** | Luminosité (lux) | PB6/PB7 | I2C |

## Architecture globale


![Architecture système](docs/screenshots/Architecture_System.png)

---

## Structure du dépôt

```
embedded-data-logger/
│
├── firmware/                          # Firmware STM32 (C, Makefile)
│   ├── src/
│   │   ├── main.c                     # Machine a etats UART + LEDs
│   │   ├── uart.c                     # Driver UART USART2
│   │   ├── gpio.c                     # Driver GPIO LEDs
│   │   ├── systick.c                  # Timer SysTick + delay_ms()
│   │   ├── iwdg.c                     # Driver Watchdog IWDG
│   │   ├── dht11.c                    # Driver capteur DHT11 (1-Wire)
│   │   ├── bh1750.c                   # Driver capteur BH1750 (I2C)
│   │   └── system_stm32f4xx.c         # SystemInit (FPU + VTOR)
│   ├── include/                       # Fichiers d'en-tete (.h)
│   ├── startup/
│   │   └── startup_stm32f407.s        # Table des vecteurs + Reset_Handler
│   ├── linker/
│   │   └── STM32F407VGTx.ld           # Script de linkage (FLASH 1024K / RAM 192K)
│   ├── Makefile                       # Build GCC + OpenOCD + GDB
│   └── openocd.cfg                    # Configuration ST-Link SWD
│
├── gui/                               # Interface graphique Python
│   ├── src/
│   │   ├── Master.py                  # Point d'entree
│   │   ├── GUI_Master.py              # Interface Tkinter (dark theme)
│   │   ├── Serial_Com_Control.py      # Communication serie UART
│   │   ├── Data_communication_Control.py  # Traitement donnees
│   │   ├── influx_client.py           # Client InfluxDB
│   │   └── logger_config.py           # Configuration logging
│   ├── config/
│   │   ├── com_config.json.example    # Configuration port serie
│   │   └── users.json.example         # Template utilisateurs
│   ├── logs/                          # Fichiers CSV + logs (ignores par Git)
│   ├── tests/                         # Tests unitaires
│   └── requirements.txt               # Dependances Python
│
├── docker/
│   └── docker-compose.yml             # Stack InfluxDB + Grafana
│
├── docs/
│   ├── Architecture_System.png        # Schema architecture systeme
│   ├── protocol.md                    # Documentation protocole UART
│   ├── dht11_driver.md                # Documentation driver DHT11
│   └── bh1750_driver.md               # Documentation driver BH1750
│
├── .gitlab-ci.yml                     # Pipeline CI/CD
└── README.md
```


---

## Prérequis

### Matériel

| Composant | Description |
|-----------|-------------|
| `STM32F407VG-Discovery` | Carte de développement |
| `Câble USB` | Connexion ST-Link + UART |
| `Adaptateur USB-UART` | CP2102 / CH340 / FTDI |
| `DHT11` | Capteur température + humidité |
| `BH1750FVI` | Capteur luminosité (I2C) |
| `PC Linux` / WSL | Ubuntu 22.04 recommandé |

### Logiciels
| Outil | Installation |
|-------|-------------|
| arm-none-eabi-gcc | `sudo apt install gcc-arm-none-eabi` |
| OpenOCD | `sudo apt install openocd` |
| make | `sudo apt install make` |
| Python 3.10+ | `sudo apt install python3` |
| Docker | `sudo apt install docker.io docker-compose` |

---

## Installation

### 1. Cloner le dépôt

```bash
git clone https://gitlab.com/z_benakka193/embedded-data-logger.git
cd embedded-data-logger
```

### 2. Firmware STM32

```bash
cd firmware

# Compiler
make

# Flasher la carte
make flash
```

### 3. Interface graphique Python

```bash
cd gui

# Installer les dependances
pip3 install -r requirements.txt

# Copier les fichiers de configuration
cp config/com_config.json.example config/com_config.json
cp config/users.json.example      config/users.json
```

### 4. Stack Cloud (InfluxDB + Grafana)

```bash
cd docker

# Lancer les conteneurs
sudo docker-compose up -d

# Vérifier
sudo docker ps
```

| Service | URL | Login |
|---------|-----|-------|
| InfluxDB | http://localhost:8086 | admin / admin1234 |
| Grafana  | http://localhost:3000 | admin / admin1234 |


---

## Guide d'utilisation

### 1. Lancement de l'application

```bash
cd gui/src
python3 Master.py
```

### 2. Authentification

![Login](docs/screenshots/login.png)

Connectez-vous avec vos identifiants définis dans `gui/config/users.json` :

| Champ | Valeur par défaut |
|-------|------------------|
| User name | `admin` |
| Password | défini dans users.json |

![Login](docs/screenshots/login2.png)

### 3. Sélection du mode de communication

Après login réussi, cliquez sur **Serial** pour ouvrir l'interface de communication série.

### 4. Connexion série

![Serial](docs/screenshots/Serial.png)

| Etape | Action |
|-------|--------|
| 1 | Sélectionnez le port (`/dev/ttyUSB0` ou `/dev/ttyACM0`) |
| 2 | Sélectionnez le baudrate (`115200`) |
| 3 | Cliquez **Connect** |
| 4 | Attendez **Sync Status : OK** et **Active channels : 4** |

### 5. Démarrage du streaming

![Streaming](docs/screenshots/Streaming2.png)

- Cliquez **Start** pour démarrer la réception des données
- Les courbes s'affichent en temps réel sur le graphique
- Cochez **Save data** pour sauvegarder en CSV et envoyer vers InfluxDB

### 6. Gestion des graphiques

![MultiChart](docs/screenshots/Multi_chart.png)

| Bouton | Action |
|--------|--------|
| **Add Chart** | Ajoute un nouveau graphique |
| **Delete Chart** | Supprime le dernier graphique |
| **+** | Ajoute un canal sur le graphique |
| **-** | Supprime un canal du graphique |


### Aperçu de l'interface — Streaming en temps réel

![Streaming DHT11 + BH1750](docs/screenshots/Streaming_mesures_H_T_L.png)

| Graphique | Canal | Capteur | Mesure |
|-----------|-------|---------|--------|
| Display Manager-1 | Ch0 | DHT11 | Humidité (%) |
| Display Manager-2 | Ch1 | DHT11 | Température (°C) |
| Display Manager-3 | Ch2 | BH1750 | Luminosité (lux) |

> Maximum **4 graphiques** simultanés

### 7. Indicateurs LED STM32

![Indicateurs](docs/screenshots/Indicateurs_LED.png)

| LED | Etat | Description |
|-----|------|-------------|
| Orange | WAIT_SYNC | En attente de connexion PC |
| Bleue | IDLE | Connecte, stream arrete |
| Verte | STREAMING | Envoi des donnees actif |
| Rouge | default | Erreur / etat inconnu |

### 8. Reconnexion automatique

En cas de deconnexion USB, une popup apparait automatiquement :
- Cliquez **Yes** pour tenter la reconnexion (5 tentatives, 2s de delai)
- Cliquez **No** pour annuler

![Reconnexion automatique](docs/screenshots/Reconnexion_automatique.png)


---

### 9. Données CSV enregistrées

Les fichiers CSV sont sauvegardés automatiquement dans `gui/logs/`
avec un nom horodaté :

```
gui/logs/
└── 20260422005625.csv   ← YYYYMMDDHHMMSS.csv
```

#### Format des données

```
timestamp,Humidity,Temperature,Luminosity,Channel_4
0.0,52,21,97,0
0.38,52,21,95,0
0.76,52,21,97,0
1.14,52,21,96,0
...
```

| Colonne | Description | Unité |
|---------|-------------|-------|
| `timestamp` | Temps relatif depuis le debut du stream | secondes |
| `Humidity` | Canal 0 — DHT11 humidite | % |
| `Temperature` | Canal 1 — DHT11 temperature | °C |
| `Luminosity` | Canal 2 — BH1750 luminosite | lux |
| `Channel_4` | Canal 3 — libre | — |

> Le dossier `gui/logs/` est ignore par Git (`.gitignore`).


---


### 10. Dashboard Grafana

![Grafana Dashboard](docs/screenshots/Grafana_Capture2.png)

Accédez au dashboard via `http://localhost:3000` :

| Panel | Capteur | Couleur | Valeur exemple |
|-------|---------|---------|----------------|
| Luminosité (lux) | BH1750 | Bleu | 0 — 1500 lux |
| Température (°C) | DHT11 | Orange | ~20 °C |
| Humidité (%) | DHT11 | Vert | 58 — 64 % |

> Documentation complète : [`docs/influxdb_grafana.md`](docs/influxdb_grafana.md)

## Protocole de communication UART

### Paramètres de la liaison série

| Paramètre | Valeur |
|-----------|--------|
| Interface | USART2 (PA2=TX, PA3=RX) |
| Baudrate | 115200 |
| Bits de données | 8 |
| Parité | Aucune |
| Bits de stop | 1 |
| Reception | Non bloquant (polling) |

---

### Machine à états STM32
```
          '?'                    'A'
WAIT_SYNC ──────────────► IDLE ──────────────► STREAMING
▲                       │  ▲                    │
│          'P'          │  │       'S'          │
└───────────────────────┘  └────────────────────┘
'P'
```

| Etat | LED | Description |
|------|-----|-------------|
| `WAIT_SYNC` | Orange | Attente de la commande de synchronisation |
| `IDLE` | Bleue | Connecte, stream arrete |
| `STREAMING` | Verte | Envoi periodique des donnees |
| `default` | Rouge | Erreur / etat inconnu |

---

### Commandes PC → STM32

| Commande | Trame | Description |
|----------|-------|-------------|
| Sync | `#?#\n` | Demande de synchronisation |
| Start | `#A#\n` | Demarrage du flux de donnees |
| Stop | `#S#\n` | Arret du flux de donnees |
| Disconnect | `#P#\n` | Deconnexion |

---

### Reponses STM32 → PC

| Situation | Trame | Description |
|-----------|-------|-------------|
| Sync OK | `#!#4#\r\n` | Sync reussie — 4 canaux actifs |
| Donnees | `#D#v1#v2#v3#v4#v5#\n` | Trame de donnees |
| Stop OK | `#STOP#\r\n` | Stream arrete |
| Disconnect | `#DISCONNECTED#\r\n` | Deconnexion confirmee |
| Erreur | `#E#OVF#\n` | Depassement buffer TX |

---

## Canaux de données

| Canal | Capteur | Mesure | Broche | Exemple |
|-------|---------|--------|--------|---------|
| `val1` | DHT11 | Humidité | PC0 (1-Wire) | 52 % |
| `val2` | DHT11 | Température | PC0 (1-Wire) | 21 °C |
| `val3` | BH1750FVI | Luminosité | PB6/PB7 (I2C) | 97 lux |
| `val4` | Libre | — | — | 0 |

### Trame UART

#D#val1#val2#val3#val4#val5#\n


**Exemple :**

#D#52#21#97#0#7#\n

val1 = 52   → Humidite 52%
val2 = 21   → Temperature 21°C
val3 = 97   → Luminosite 97 lux
val4 = 0    → libre
val5 = 7    → integrite (nb chiffres : 2+2+2+1=7)


> Documentation complète : [`docs/protocol.md`](docs/protocol.md)