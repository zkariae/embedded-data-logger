"""
influx_client.py
----------------
Module d'envoi des donnees capteurs vers InfluxDB.

Auteur  : z_benakka193
Projet  : embedded-data-logger
"""

from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from logger_config import setup_logger

logger = setup_logger("InfluxClient")

# ------------------------------------------------------------------
# Configuration InfluxDB
# ------------------------------------------------------------------
INFLUX_URL    = "http://localhost:8086"
INFLUX_TOKEN  = "my-super-secret-token"
INFLUX_ORG    = "embedded-data-logger"
INFLUX_BUCKET = "sensors"


class InfluxClient:
    """
    Client InfluxDB pour l'envoi des donnees capteurs.
    """

    def __init__(self):
        """Initialise la connexion InfluxDB."""
        try:
            self.client    = InfluxDBClient(
                url=INFLUX_URL,
                token=INFLUX_TOKEN,
                org=INFLUX_ORG
            )
            self.write_api = self.client.write_api(write_options=SYNCHRONOUS)
            logger.info("Connexion InfluxDB etablie.")
        except Exception as e:
            logger.error(f"Erreur connexion InfluxDB : {e}")
            self.client    = None
            self.write_api = None

    def send_data(self, channel_names, values, timestamp):
        """
        Envoie les donnees capteurs vers InfluxDB.

        Args:
            channel_names (list) : noms des canaux ['Voltage', 'Current', ...]
            values        (list) : valeurs correspondantes [206, 157, ...]
            timestamp     (float): horodatage en secondes
        """
        if self.write_api is None:
            return

        try:
            point = Point("sensors")

            for name, value in zip(channel_names, values):
                point = point.field(name, float(value))

            point = point.time(
                int(timestamp * 1e9),
                WritePrecision.NANOSECONDS
            )

            self.write_api.write(
                bucket=INFLUX_BUCKET,
                org=INFLUX_ORG,
                record=point
            )
            logger.debug(f"Donnees envoyees : {dict(zip(channel_names, values))}")

        except Exception as e:
            logger.error(f"Erreur envoi InfluxDB : {e}")

    def close(self):
        """Ferme la connexion InfluxDB."""
        if self.client:
            self.client.close()
            logger.info("Connexion InfluxDB fermee.")