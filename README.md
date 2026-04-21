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
