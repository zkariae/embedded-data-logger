<div align="center">

# embedded-data-logger

![Build](https://gitlab.com/z_benakka193/embedded-data-logger/badges/main/pipeline.svg)
![Version](https://img.shields.io/badge/version-v2.7.0-green)
![License](https://img.shields.io/badge/license-MIT-blue)
![Python](https://img.shields.io/badge/python-3.10-blue)
![STM32](https://img.shields.io/badge/STM32-F407VG-red)

Système embarqué de **collecte, visualisation et stockage cloud**
de données en temps réel via UART DMA.

</div>


---

## Description

**embedded-data-logger** est un système embarqué complet permettant de :

- Collecter des données depuis une carte **STM32F407VG-Discovery** via **UART**
- Visualiser les données en **temps réel** via une interface graphique Python
- Sauvegarder les données localement en **CSV**
- Stocker et visualiser les données dans le **cloud** via InfluxDB + Grafana

---

## Architecture globale

```
┌─────────────────────┐        UART 115200 baud       ┌──────────────────────┐
│  STM32F407VG-Disco  │ ────────────────────────────► │      PC Linux        │
│                     │    #D#v1#v2#v3#v4#v5#\n       │   Python GUI Tkinter │
│  - GPIO LEDs        │                               │                      │
│  - UART USART2      │                               │  ┌────────────────┐  │
│  - SysTick          │                               │  │ Affichage      │  │
│  - IWDG Watchdog    │                               │  │ temps reel     │  │
└─────────────────────┘                               │  ├────────────────┤  │
                                                      │  │ Sauvegarde CSV │  │
       firmware/                                      │  ├────────────────┤  │
                                                      │  │ Envoi InfluxDB │  │
                                                      │  └────────────────┘  │
                                                      └──────────┬───────────┘
                                                                 │
                                                                 ▼
                                                      ┌──────────────────────┐
                                                      │  InfluxDB            │
                                                      │  (time-series DB)    │
                                                      └──────────┬───────────┘
                                                                 │
                                                                 ▼
                                                      ┌──────────────────────┐
                                                      │  Grafana Dashboard   │
                                                      │  localhost:3000      │
                                                      └──────────────────────┘
```


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
│   └── requirements.txt              # Dependances Python
│
├── docker/
│   └── docker-compose.yml             # Stack InfluxDB + Grafana
│
├── docs/
│   └── protocol.md                    # Documentation protocole UART
│
├── .gitlab-ci.yml                     # Pipeline CI/CD
└── README.md
```


---

## Prérequis

### Matériel
| Composant | Description |
|-----------|-------------|
| STM32F407VG-Discovery | Carte de développement |
| Câble USB | Connexion ST-Link + UART |
| PC Linux / WSL | Ubuntu 22.04 recommandé |

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
