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


## Installation

### Prérequis

```bash
# Vérifier Docker
docker --version
docker-compose --version
```

### Lancement des conteneurs

```bash
cd docker
sudo docker-compose up -d
```

### Vérification

```bash
sudo docker ps
```

Résultat attendu :
CONTAINER ID   IMAGE                    STATUS        PORTS
3aed980cbef7   grafana/grafana:10.0.0   Up            0.0.0.0:3000->3000/tcp
311dfd7579ff   influxdb:2.7             Up            0.0.0.0:8086->8086/tcp

### Accès aux interfaces

| Service | URL | Login |
|---------|-----|-------|
| InfluxDB | http://localhost:8086 | admin / admin1234 |
| Grafana | http://localhost:3000 | admin / admin1234 |

### Arrêt des conteneurs

```bash
cd docker
sudo docker-compose down
```

### Commandes utiles

```bash
# Voir les logs InfluxDB
sudo docker logs embedded-influxdb

# Voir les logs Grafana
sudo docker logs embedded-grafana

# Redémarrer les conteneurs
sudo docker-compose restart
```