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

## Client Python — influx_client.py

### Description

Le module `influx_client.py` est responsable de l'envoi des données
capteurs vers InfluxDB via l'API HTTP officielle.

### Installation

```bash
pip3 install influxdb-client
```

### Classe InfluxClient

```python
from influx_client import InfluxClient

# Instanciation
client = InfluxClient()

# Envoi des données
client.send_data(
    channel_names=["Humidity", "Temperature", "Luminosity"],
    values=[52, 21, 97],
    timestamp=datetime.now(timezone.utc)
)

# Fermeture
client.close()
```

### Méthodes

| Méthode | Description |
|---------|-------------|
| `__init__()` | Connexion InfluxDB |
| `send_data()` | Envoi des mesures capteurs |
| `close()` | Fermeture de la connexion |

### Paramètres de `send_data()`

| Paramètre | Type | Description | Exemple |
|-----------|------|-------------|---------|
| `channel_names` | list | Noms des canaux | `["Humidity", "Temperature", "Luminosity"]` |
| `values` | list | Valeurs mesurées | `[52, 21, 97]` |
| `timestamp` | datetime | Horodatage UTC | `datetime.now(timezone.utc)` |

### Intégration dans save_data()

Les données sont envoyées vers InfluxDB automatiquement
lorsque **Save data** est coché dans l'interface :

```python
# Dans Data_communication_Control.py
def save_data(self, gui):
    if not gui.save:
        return

    # Sauvegarde CSV
    with open(self.filename, "a", newline="") as csv_file:
        ...

    # Envoi vers InfluxDB
    channel_names = [self.ChannelName[ch] for ch in self.Channels]
    self.influx.send_data(
        channel_names=channel_names,
        values=self.int_msg,
        timestamp=datetime.now(timezone.utc)
    )
```

### Format des données dans InfluxDB
measurement : sensors
fields      : Humidity, Temperature, Luminosity, Channel_4
timestamp   : 2026-04-21T17:25:40.000Z

### Exemple de point InfluxDB
sensors Humidity=52,Temperature=21,Luminosity=97,Channel_4=0 1713718740000000000

## Requêtes Flux

### Syntaxe de base

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> filter(fn: (r) => r._field == "Humidity")
```

### Requêtes par capteur

#### DHT11 — Humidité

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> filter(fn: (r) => r._field == "Humidity")
```

#### DHT11 — Température

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> filter(fn: (r) => r._field == "Temperature")
```

#### BH1750 — Luminosité

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> filter(fn: (r) => r._field == "Luminosity")
```

#### Tous les capteurs en une seule requête

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
```

### Requêtes avancées

#### Moyenne sur 5 minutes

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> filter(fn: (r) => r._field == "Temperature")
  |> aggregateWindow(every: 5m, fn: mean)
```

#### Valeur maximale

```flux
from(bucket: "sensors")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> filter(fn: (r) => r._field == "Humidity")
  |> max()
```

#### Valeur minimale

```flux
from(bucket: "sensors")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> filter(fn: (r) => r._field == "Temperature")
  |> min()
```

#### Dernière valeur reçue

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
  |> last()
```

## Dashboard Grafana

### Création du dashboard

1. **Home → Dashboards → New Dashboard**
2. Cliquez **Add visualization**
3. Sélectionnez la source **InfluxDB**

### Configuration du panel

#### Requête pour tous les capteurs

```flux
from(bucket: "sensors")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "sensors")
```

#### Paramètres du panel

| Paramètre | Valeur |
|-----------|--------|
| Title | Embedded Data Logger |
| Type | Time series |
| Interval | Auto (200ms) |

### Configuration des couleurs

| Canal | Couleur |
|-------|---------|
| Humidity | Bleu |
| Temperature | Rouge |
| Luminosity | Jaune |
| Channel_4 | Cyan |

### Panels recommandés

| Panel | Type | Requête | Description |
|-------|------|---------|-------------|
| Humidité | Time series | `_field == "Humidity"` | Courbe humidité % |
| Température | Time series | `_field == "Temperature"` | Courbe température °C |
| Luminosité | Time series | `_field == "Luminosity"` | Courbe luminosité lux |
| Dernière humidité | Stat | `_field == "Humidity" \|> last()` | Valeur actuelle |
| Dernière température | Stat | `_field == "Temperature" \|> last()` | Valeur actuelle |
| Dernière luminosité | Stat | `_field == "Luminosity" \|> last()` | Valeur actuelle |

### Paramètres de rafraîchissement

| Paramètre | Valeur recommandée |
|-----------|-------------------|
| Auto-refresh | 5s |
| Time range | Last 1 hour |
| Timezone | Browser |

### Sauvegarde du dashboard

1. Cliquez **Save dashboard** en haut à droite
2. Nom : `Embedded Data Logger`
3. Cliquez **Save**

### Volumes persistants

Les dashboards Grafana sont sauvegardés dans le volume Docker :

```bash
# Lister les volumes
sudo docker volume ls

# Attendu :
# embedded-data-logger_grafana-data
# embedded-data-logger_influxdb-data
```

> Les données et dashboards survivent même après `docker-compose down`.

### Grafana — Dashboard 3 panels séparés

![Grafana Dashboard](docs/screenshots/Grafana_Capture2.png)

| Panel | Capteur | Couleur | Valeur exemple |
|-------|---------|---------|----------------|
| Luminosité (lux) | BH1750 | Bleu | 0 — 1500 lux |
| Température (°C) | DHT11 | Orange | ~20 °C |
| Humidité (%) | DHT11 | Vert | 58 — 64 % |