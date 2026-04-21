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






















Système embarqué de **collecte et visualisation de données en temps réel**, composé de deux parties :

- **`firmware/`** — Firmware pour la carte **STM32F407VG-Discovery**
- **`gui/`** — Interface graphique **Python** pour la réception et visualisation des données

---

## Architecture globale
┌─────────────────────┐    UART 115200 baud    ┌──────────────────────┐
│  STM32F407VG-Disco  │ ─────────────────────► │      PC Linux        │
│  USART2 DMA         │    #D#v1#v2#v3#v4#v5#  │   Python GUI Tkinter │
└─────────────────────┘                        └──────────────────────┘
firmware/                                        gui/

---

## Structure du dépôt
embedded-data-logger/
├── firmware/
│   ├── src/                        # Fichiers sources C
│   │   ├── main.c                  # Machine a etats UART
│   │   ├── uart.c                  # Driver UART USART2
│   │   ├── gpio.c                  # Driver GPIO LEDs
│   │   ├── systick.c               # Timer SysTick + delay_ms()
│   │   └── system_stm32f4xx.c      # SystemInit (FPU + VTOR)
│   ├── include/                    # Fichiers d'en-tete
│   ├── startup/
│   │   └── startup_stm32f407.s     # Table des vecteurs + Reset_Handler
│   ├── linker/
│   │   └── STM32F407VGTx.ld        # Script de linkage (FLASH 1024K / RAM 192K)
│   ├── Makefile                    # Build GCC + OpenOCD + GDB
│   └── openocd.cfg                 # Configuration ST-Link SWD
│
├── gui/
│   ├── src/
│   │   ├── Master.py               # Point d'entree
│   │   ├── GUI_Master.py           # Interface graphique Tkinter
│   │   ├── Serial_Com_Control.py   # Communication serie UART
│   │   └── Data_communication_Control.py  # Traitement des donnees
│   ├── config/
│   │   ├── com_config.json.example # Configuration port serie
│   │   └── users.json.example      # Template utilisateurs
│   ├── tests/
│   └── requirements.txt            # Dependances Python
│
├── docs/
│   └── protocol.md                 # Documentation protocole UART
└── README.md

---

## Firmware STM32

### Prérequis

```bash
sudo apt install gcc-arm-none-eabi gdb-multiarch openocd make
```

### Compilation et flash

```bash
cd firmware

make                  # Build release
make BUILD=debug      # Build debug
make flash            # Flasher via OpenOCD
make debug-server     # Lancer OpenOCD GDB server
make debug            # Lancer GDB client
make size             # Memory usage
make clean            # Nettoyer build/
```

### Résultat build
text : 2404 bytes   (0.2% de 1024K FLASH)
data : 8 bytes
bss  : 80 bytes

### Indicateurs LED

| LED    | Etat      | Description                    |
|--------|-----------|--------------------------------|
| Orange | WAIT_SYNC | Attente de connexion PC        |
| Bleue  | IDLE      | Connecte, stream arrete        |
| Verte  | STREAMING | Envoi des donnees actif        |
| Rouge  | default   | Erreur / etat inconnu          |

---

## GUI Python

### Prérequis

```bash
sudo apt install python3-tk python3-pip
pip3 install -r gui/requirements.txt
```

### Configuration

```bash
# Copier et adapter les fichiers de configuration
cp gui/config/com_config.json.example gui/config/com_config.json
cp gui/config/users.json.example      gui/config/users.json
```

### Lancement

```bash
cd gui/src
python3 Master.py
```

### Utilisation

1. Se connecter avec **username** et **password** (définis dans `users.json`)
2. Cliquer sur **Serial**
3. Sélectionner le port (`/dev/ttyUSB0` ou `/dev/ttyACM0`) et baudrate `115200`
4. Cliquer **Connect** → synchronisation automatique
5. Cliquer **Start** → visualisation en temps réel

---

## Protocole UART

Voir [`docs/protocol.md`](docs/protocol.md) pour la documentation complète.

---

## Versions

| Tag     | Contenu                                          |
|---------|--------------------------------------------------|
| v1.0.0  | Refactoring GUI Python                           |
| v1.1.0  | Ajout dossier config                             |
| v1.3.0  | Firmware STM32 complet                           |
| v1.4.0  | Correction chemins config + point d'entree       |
| v1.5.0  | Correction reception UART                        |
| v1.6.0  | Fix streaming UART + donnees dynamiques          |
| v1.7.0  | Ajout requirements.txt                           |
| v1.8.0  | Documentation protocole UART                     |
| v1.9.0  | README complet                                   |

---

## Auteur

Zakariae BEN-AKKA — Projet embedded-data-logger
