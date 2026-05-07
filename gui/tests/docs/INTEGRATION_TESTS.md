# Tests d'intégration GUI — data pipeline

## Architecture des tests

```
gui/tests/
├── docs/
│   └── INTEGRATION_TESTS.md     ← Ce document
├── stm32_simulator.py           ← Simulateur UART STM32F407 (host)
├── test_integration.py          ← Tests d'intégration du pipeline de données
├── test_influx_client.py        ← Tests du client InfluxDB (mocké)
├── test_data.py                 ← Réservé : tests DataMaster
└── __init__.py
```

---

## Description des fonctions par fichier

---

### `stm32_simulator.py` — Simulateur STM32

#### Classe `STM32Simulator`

Simule le comportement série de la carte STM32F407VG-Discovery. Génère des trames UART conformes au protocole.

| Fonction | Description | Entrée | Sortie |
|----------|-------------|--------|--------|
| `__init__(num_channels, port)` | Initialise le simulateur avec N canaux et des valeurs capteur par défaut (humidity=52, temperature=21, luminosity=97, channel_4=0) | `num_channels: int = 4` | `None` |
| `generate_data_frame()` | Construit une trame `#D#<val0>#...#<valN>#<checksum>#\n` à partir des valeurs capteur actuelles. Le checksum = somme des longueurs des chaînes de valeurs | `self` | `bytes` |
| `generate_sync_response()` | Construit la réponse sync `#D#!<nb_channels>#<checksum>#\r\n` où checksum = `len(str(nb_channels))` | `self` | `bytes` |
| `generate_stop_response()` | Retourne `b"#STOP#\r\n"` | `self` | `bytes` |
| `generate_disconnect_response()` | Retourne `b"#DISCONNECTED#\r\n"` | `self` | `bytes` |
| `set_luminosity(lux)` | Modifie la valeur simulée de luminosité | `lux: int` | `None` |
| `set_humidity(hum)` | Modifie la valeur simulée d'humidité | `hum: int` | `None` |
| `set_temperature(temp)` | Modifie la valeur simulée de température | `temp: int` | `None` |
| `process_command(command)` | Route la commande reçue vers la réponse appropriée. Gère `#?#\n` (sync), `#A#\n` (start), `#S#\n` (stop), `#P#\n` (disconnect) | `command: bytes` | `Optional[bytes]` |
| `simulate_error_frame()` | Génère une trame `#D#100#200#300#9999#\n` avec checksum invalide (9999 au lieu de 9) | `self` | `bytes` |
| `simulate_overflow_frame()` | Génère une trame d'erreur `#E#OVF#\n` | `self` | `bytes` |
| `simulate_noise()` | Génère une trame invalide `#garbage#data#\n` (bruit sur la ligne) | `self` | `bytes` |
| `simulate_data_stream(num_frames, interval)` | Génère une liste de N trames de données valides consécutives | `num_frames: int = 5` | `list[bytes]` |
| `simulate_sync_sequence()` | Retourne une liste contenant la réponse sync | `self` | `list[bytes]` |
| `simulate_full_session(num_frames)` | Simule une session complète : sync → N data frames → stop | `num_frames: int = 10` | `list[bytes]` |

---

### `test_integration.py` — Tests d'intégration

#### Fonction utilitaire

| Fonction | Description | Entrée | Sortie |
|----------|-------------|--------|--------|
| `parse_data_frame(frame, num_channels)` | Parse une trame `#D#` et retourne un dict `{"values": [...], "checksum": int}`. Vérifie que `parts[1] == "D"` et que `len(values) == num_channels` | `frame: bytes, num_channels: int` | `dict` |

#### Classe `TestSTM32Simulator` — Validation du simulateur (14 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_generate_sync_response_format` | `self.sim.generate_sync_response()` | Appelle la génération de réponse sync et parse le résultat | La chaîne commence par `#D#!`, le nombre de canaux extrait de `!4` vaut 4 |
| `test_generate_data_frame_format` | `self.sim.generate_data_frame()` | Appelle la génération de trame de données | La trame commence par `#D#`, se termine par `#`, contient 4 valeurs après parsing |
| `test_generate_data_frame_checksum_valid` | `self.sim.generate_data_frame()` | Calcule le checksum attendu et le compare | `checksum == sum(len(str(v)) for v in values)` |
| `test_generate_data_frame_num_values` | `self.sim.generate_data_frame()` | Parse la trame et compte les valeurs | `len(values) == 4` |
| `test_process_sync_command` | `self.sim.process_command(b"#?#\n")` | Envoie la commande de synchronisation | La réponse n'est pas None et contient `#D#!` |
| `test_process_start_stream` | `self.sim.process_command(b"#A#\n")` | Envoie la commande de démarrage | La réponse est None, `streaming == True` |
| `test_process_stop_stream` | `self.sim.process_command(b"#S#\n")` | Envoie la commande d'arrêt (avec streaming=True au préalable) | La réponse contient `#STOP#`, `streaming == False` |
| `test_process_disconnect` | `self.sim.process_command(b"#P#\n")` | Envoie la commande de déconnexion | La réponse contient `#DISCONNECTED#` |
| `test_simulate_error_frame` | `self.sim.simulate_error_frame()` | Génère une trame avec checksum invalide | La trame contient `#D#`, le checksum parsé (9999) ≠ checksum attendu (9) |
| `test_simulate_overflow_frame` | `self.sim.simulate_overflow_frame()` | Génère une trame d'overflow | La trame contient `#E#OVF#` |
| `test_set_luminosity_updates_frame` | `set_luminosity(1500)` puis `set_luminosity(97)` | Modifie la luminosité entre deux trames | Les valeurs parsées de channel 2 sont différentes |
| `test_simulate_data_stream` | `self.sim.simulate_data_stream(5)` | Génère 5 trames de données | `len(frames) == 5`, chaque trame commence par `#D#` |
| `test_simulate_full_session` | `self.sim.simulate_full_session(5)` | Simule sync + 5 data + stop | 7 trames : la 1ère contient `#D#!`, la dernière `#STOP#`, les 5 du milieu commencent par `#D#` |
| `test_simulate_noise` | `self.sim.simulate_noise()` | Génère une trame de bruit | La trame contient `garbage` |

#### Classe `TestDataFrameParsing` — Parsing des trames UART (5 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_parse_valid_data_frame` | `parse_data_frame(sim.generate_data_frame(), 4)` | Parse une trame valide du simulateur | 4 valeurs parsées, la valeur de luminosité correspond à `sensor_values["luminosity"]` |
| `test_parse_sync_response` | `sim.generate_sync_response()` | Parse la réponse sync | `parts[2]` commence par `!`, le nombre de canaux extrait vaut 4, le checksum = `len("4")` |
| `test_parse_with_changed_luminosity` | `set_luminosity(1200)` puis parse | Parse une trame après modification de la luminosité | La valeur parsée channel 2 vaut 1200 |
| `test_parse_invalid_frame` | `sim.simulate_noise()` | Tente de parser une trame de bruit | `parts[1] != "D"` (trame rejetée) |
| `test_parse_overflow_frame` | `sim.simulate_overflow_frame()` | Vérifie le format de la trame overflow | La trame contient `#E#OVF#` |

#### Classe `TestErrorRecovery` — Récupération d'erreurs (9 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_disconnect_response_bytes` | `sim.process_command(b"#P#\n")` | Vérifie le type et la valeur de la réponse disconnect | `resp == b"#DISCONNECTED#\r\n"`, `isinstance(resp, bytes)` |
| `test_stop_response_bytes` | `sim.process_command(b"#S#\n")` | Vérifie le type et la valeur de la réponse stop | `resp == b"#STOP#\r\n"`, `isinstance(resp, bytes)` |
| `test_streaming_stops_on_disconnect` | `streaming=True` puis `process_command(b"#P#\n")` | Vérifie que le flag streaming passe à False | `streaming == False` |
| `test_streaming_stops_on_stop` | `streaming=True` puis `process_command(b"#S#\n")` | Vérifie que le flag streaming passe à False | `streaming == False` |
| `test_reconnect_after_disconnect` | Déconnexion → sync → start → data | Vérifie qu'on peut reconnecter et streamer après une déconnexion | Sync OK, streaming True, data frame valide |
| `test_malformed_frame_no_checksum` | Trame `b"#D#100#200#300#\n"` | Vérifie qu'une trame sans checksum a moins de 5 éléments | `len(raw) < 5` |
| `test_malformed_frame_empty_values` | Trame `b"#D####\n"` | Vérifie qu'une trame avec valeurs vides est détectée | Tous les éléments entre `D` et le dernier `#` sont vides |
| `test_overflow_frame_detected` | `sim.simulate_overflow_frame()` | Vérifie le format de la trame overflow | Commence par `#E#`, contient `OVF` |
| `test_full_disconnect_cycle` | Sync → Start → Data → Stop → Disconnect | Exécute le cycle complet de communication | Chaque étape retourne la réponse attendue, streaming correctement mis à jour |

#### Classe `TestFrameIntegrity` — Validation d'intégrité (3 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_checksum_valid` | `parse_data_frame(sim.generate_data_frame(), 4)` | Vérifie qu'une trame valide a un checksum correct | `checksum == sum(len(str(v)) for v in values)` |
| `test_checksum_invalid` | Corruption du checksum d'une trame valide (remplace par 9999) | Vérifie qu'un checksum invalide est détecté | `int(cs_raw) != expected` |
| `test_wrong_channel_count_rejected` | Trame `#D#100#200#300#9#\n` avec 3 valeurs | Vérifie qu'un mauvais nombre de valeurs est détecté | `len(vals) != 4` |
| `_corrupt_checksum()` (helper) | `sim.generate_data_frame()` | Corrompt le checksum d'une trame valide en le remplaçant par 9999 | `bytes` |

#### Classe `TestCSVOutput` — Sauvegarde CSV (3 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_csv_headers_format` | Liste d'en-têtes construite manuellement | Vérifie le format des en-têtes CSV | `headers == ["timestamp", "Humidity (%)", "Temperature (°C)", "Luminosity (lux)", "Channel_4"]` |
| `test_csv_write_and_read` | Simulateur → parse → écriture CSV → relecture | Écrit une ligne CSV dans un fichier temporaire et la relit | `len(rows[1]) == 5`, valeur de luminosité = "97" |
| `test_csv_multiple_rows` | 5 trames avec luminosité croissante (100,110,...,140) | Écrit 5 lignes CSV dans un fichier temporaire et les relit | `len(rows) == 6` (1 header + 5 data), valeurs correctes |

#### Classe `TestEndToEnd` — Tests de bout en bout (3 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_full_session_data_flow` | Sync → Start → 10 data frames → Stop | Exécute un flux complet de données | Sync retourne `#D#!`, streaming True, 10 frames valides, stop retourne `#STOP#`, streaming False |
| `test_data_consistency_across_session` | Start → 5 trames avec luminosité [100,200,300,400,500] | Vérifie la cohérence des données sur une session | `values == [100, 200, 300, 400, 500]` |
| `test_invalid_frame_in_stream` | Data frame, noise, overflow | Vérifie la détection des trames invalides dans le flux | Data commence par `#D#`, noise et overflow ne commencent pas par `#D#` |

---

### `test_influx_client.py` — Tests du client InfluxDB

#### Fonction utilitaire

| Fonction | Description | Entrée | Sortie |
|----------|-------------|--------|--------|
| `mock_influx_module()` (fixture pytest) | Mocke `InfluxDBClient` et `Point` du module `influx_client`. Configure les retours de chaînage (`field()` retourne le point, `time()` retourne le point) | `None` (fixture) | `dict` contenant les mocks : `client_class`, `client_instance`, `write_api`, `point_class`, `point_instance` |

#### Classe `TestInfluxClientInit` — Initialisation (2 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_init_success` | `InfluxClient()` avec mock réussi | Instancie le client InfluxDB avec une connexion valide | `client.client` non None, `client.write_api` non None, `InfluxDBClient` appelé une fois |
| `test_init_failure_sets_none` | `InfluxClient()` avec `InfluxDBClient` qui lève une exception | Simule un échec de connexion à la base | `client.client is None`, `client.write_api is None` |

#### Classe `TestInfluxClientSendData` — Envoi de données (4 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_send_data_valid` | `send_data(channel_names=[4 canaux], values=[52,21,97,0], timestamp=1.234)` | Envoie 4 champs de données capteur | Chaque `field(name, value)` appelé avec les bons arguments, `write()` appelé une fois |
| `test_send_data_no_write_api` | `send_data(...)` après échec de connexion | Tente d'envoyer des données sans connexion InfluxDB | `write_api.write` non appelé (early return) |
| `test_send_data_single_channel` | `send_data(channel_names=["Temperature (C)"], values=[25])` | Envoie un seul canal | `point.field` appelé une seule fois avec `"Temperature (C)", 25.0` |
| `test_send_data_empty_lists` | `send_data(channel_names=[], values=[])` | Envoie avec listes vides (aucun champ ajouté) | `write()` appelé une fois (point sans field) |

#### Classe `TestInfluxClientClose` — Fermeture (2 tests)

| Test | Entrée | Description | Vérifie |
|------|--------|-------------|---------|
| `test_close_with_client` | `InfluxClient()` puis `close()` | Ferme la connexion InfluxDB | `client.client.close()` appelé une fois |
| `test_close_no_client` | `InfluxClient()` (échec connexion) puis `close()` | Ferme sans connexion active | Aucune erreur levée (safe path) |

---

## Format des trames

### Trame de données (`generate_data_frame`)
```
#D#<val0>#<val1>#...#<valN>#<checksum>#\n
```
- `checksum = sum(len(str(v)) for v in values)`
- Les valeurs simulées par défaut : humidity=52, temperature=21, luminosity=97, channel_4=0

### Réponse sync (`generate_sync_response`)
```
#D#!<nb_channels>#<checksum>#\r\n
```
- `checksum = len(str(nb_channels))`

### Réponse stop (`generate_stop_response`)
```
#STOP#\r\n
```

### Réponse déconnexion (`generate_disconnect_response`)
```
#DISCONNECTED#\r\n
```

### Trame d'erreur overflow (`simulate_overflow_frame`)
```
#E#OVF#\n
```

### Trame d'erreur (checksum invalide — `simulate_error_frame`)
```
#D#100#200#300#9999#\n
```
- Checksum attendu : `len("100") + len("200") + len("300") = 9`
- Checksum réel : 9999 → invalide

---

## Exécution

```bash
# Depuis gui/
python3 -m pytest tests/ -v

# Exécuter une classe spécifique
python3 -m pytest tests/test_integration.py -v -k TestErrorRecovery

# Exécuter les tests InfluxDB
python3 -m pytest tests/test_influx_client.py -v

# Exécuter un test spécifique
python3 -m pytest tests/test_integration.py::TestEndToEnd::test_full_session_data_flow -v
```

### Exemple de sortie

```
tests/test_integration.py::TestSTM32Simulator::test_generate_sync_response_format PASSED
tests/test_integration.py::TestSTM32Simulator::test_generate_data_frame_format PASSED
...
tests/test_influx_client.py::TestInfluxClientSendData::test_send_data_valid PASSED
...
-------------------------------
45 passed in 0.55s
```

---

## Dépendances

| Dépendance | Rôle | Installation |
|------------|------|--------------|
| **pytest** | Runner de tests | `pip install pytest` |
| **influxdb-client** | Client InfluxDB (mocké dans les tests) | `pip install influxdb-client` |

---

## Ajout de nouveaux tests

1. Créer une classe de test ou ajouter une méthode à une classe existante
2. Utiliser `STM32Simulator` pour générer des trames ou `parse_data_frame` pour les parser
3. Pour l'InfluxDB, utiliser le décorateur `@patch("influx_client.InfluxDBClient")`
4. Exécuter : `python3 -m pytest tests/ -v`

---

## Fichiers modifiés par rapport à l'original

| Fichier | Modification |
|---------|-------------|
| `tests/stm32_simulator.py` | `RESP_STOP_OK` et `RESP_DISCONNECT` en bytes ; `simulate_error_frame()` avec checksum non vide |
| `tests/test_integration.py` | Correction `test_wrong_channel_count_rejected` ; ajout `TestErrorRecovery` (9 tests) |
| `tests/test_influx_client.py` | Nouveau : 8 tests pour InfluxClient avec mocks |
| `tests/docs/INTEGRATION_TESTS.md` | Nouveau : documentation complète |
