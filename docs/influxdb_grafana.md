# Pipeline Cloud — InfluxDB + Grafana

## Description

**InfluxDB** est une base de données **time-series** optimisée pour le stockage
de données horodatées. Elle est idéale pour les données de capteurs IoT car
chaque mesure est associée à un timestamp précis.

**Grafana** est un outil de **visualisation et monitoring** open-source qui
se connecte à InfluxDB pour afficher les données sous forme de dashboards
interactifs accessibles via un navigateur web.

---

## Rôle dans le projet

| Composant | Rôle |
|-----------|------|
| **InfluxDB** | Stockage time-series des mesures DHT11 et BH1750 |
| **Grafana** | Visualisation en temps réel via dashboard web |
| **influx_client.py** | Envoi des données depuis la GUI Python |
| **Docker** | Déploiement rapide et isolé des deux services |

---

## Avantages d'InfluxDB pour l'IoT

| Fonctionnalité | Description |
|----------------|-------------|
| Time-series natif | Optimisé pour les données horodatées |
| Haute performance | Millions de points par seconde |
| Rétention configurable | Suppression automatique des anciennes données |
| Langage Flux | Requêtes puissantes et flexibles |
| API HTTP | Intégration facile avec Python |