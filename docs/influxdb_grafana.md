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

## Configuration

### docker-compose.yml

```yaml
services:

  influxdb:
    image: influxdb:2.7
    container_name: embedded-influxdb
    ports:
      - "8086:8086"
    environment:
      - DOCKER_INFLUXDB_INIT_MODE=setup
      - DOCKER_INFLUXDB_INIT_USERNAME=admin
      - DOCKER_INFLUXDB_INIT_PASSWORD=admin1234
      - DOCKER_INFLUXDB_INIT_ORG=embedded-data-logger
      - DOCKER_INFLUXDB_INIT_BUCKET=sensors
      - DOCKER_INFLUXDB_INIT_RETENTION=30d
      - DOCKER_INFLUXDB_INIT_ADMIN_TOKEN=my-super-secret-token

  grafana:
    image: grafana/grafana:10.0.0
    container_name: embedded-grafana
    ports:
      - "3000:3000"
    environment:
      - GF_SECURITY_ADMIN_USER=admin
      - GF_SECURITY_ADMIN_PASSWORD=admin1234
    depends_on:
      - influxdb
```

### Paramètres InfluxDB

| Paramètre | Valeur | Description |
|-----------|--------|-------------|
| `URL` | http://localhost:8086 | Adresse du serveur |
| `Organisation` | embedded-data-logger | Namespace du projet |
| `Bucket` | sensors | Conteneur des données |
| `Token` | my-super-secret-token | Authentification API |
| `Retention` | 30 jours | Durée de conservation |

### Paramètres Grafana

| Paramètre | Valeur |
|-----------|--------|
| `URL` | http://localhost:3000 |
| `Admin` | admin / admin1234 |
| `Data source` | InfluxDB (Flux) |

### Configuration data source Grafana

1. **Connections → Data Sources → Add data source**
2. Choisissez **InfluxDB**
3. Configurez :
Query Language : Flux
URL            : http://influxdb:8086
Organization   : embedded-data-logger
Token          : my-super-secret-token
Default Bucket : sensors

4. Cliquez **Save & Test** → `datasource is working. 3 buckets found`