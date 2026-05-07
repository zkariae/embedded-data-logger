"""Simulateur UART de la carte STM32F407 pour les tests d'intégration.

Génère des trames conformes au protocole du firmware embedded-data-logger
sans nécessiter de matériel réel.
"""

import time
import threading
from typing import Optional


class STM32Simulator:
    """Simule le comportement série de la carte STM32F407VG-Discovery.

    Peut être connecté à un port série virtuel (pty) ou utilisé en mémoire
    via des buffers pour les tests unitaires.
    """

    CMD_SYNC = b"#?#\n"
    CMD_START_STREAM = b"#A#\n"
    CMD_STOP_STREAM = b"#S#\n"
    CMD_DISCONNECT = b"#P#\n"

    RESP_SYNC_OK = "#D#!{nb_channels}#{checksum}#\r\n"
    RESP_DATA = "#D#{values}#{checksum}#\n"
    RESP_STOP_OK = b"#STOP#\r\n"
    RESP_DISCONNECT = b"#DISCONNECTED#\r\n"

    def __init__(self, num_channels: int = 4, port: Optional[object] = None):
        self.num_channels = num_channels
        self.port = port
        self.streaming = False
        self._thread: Optional[threading.Thread] = None
        self._stop_event = threading.Event()

        self.sensor_values = {
            "humidity": 52,
            "temperature": 21,
            "luminosity": 97,
            "channel_4": 0,
        }

    def generate_data_frame(self) -> bytes:
        """Génère une trame de données #D# valide."""
        values = [
            self.sensor_values["humidity"],
            self.sensor_values["temperature"],
            self.sensor_values["luminosity"],
            self.sensor_values["channel_4"],
        ][: self.num_channels]

        str_values = [str(v) for v in values]
        checksum = sum(len(v) for v in str_values)
        body = "#".join(str_values)
        frame = f"#D#{body}#{checksum}#\n"
        return frame.encode("utf-8")

    def generate_sync_response(self) -> bytes:
        """Génère une réponse de synchronisation valide."""
        checksum = len(str(self.num_channels))
        frame = f"#D#!{self.num_channels}#{checksum}#\r\n"
        return frame.encode("utf-8")

    def generate_stop_response(self) -> bytes:
        return self.RESP_STOP_OK

    def generate_disconnect_response(self) -> bytes:
        return self.RESP_DISCONNECT

    def set_luminosity(self, lux: int):
        self.sensor_values["luminosity"] = lux

    def set_humidity(self, hum: int):
        self.sensor_values["humidity"] = hum

    def set_temperature(self, temp: int):
        self.sensor_values["temperature"] = temp

    def process_command(self, command: bytes) -> Optional[bytes]:
        """Traite une commande recue et retourne la reponse appropriee."""
        if command == self.CMD_SYNC:
            return self.generate_sync_response()
        elif command == self.CMD_START_STREAM:
            self.streaming = True
            return None
        elif command == self.CMD_STOP_STREAM:
            self.streaming = False
            return self.generate_stop_response()
        elif command == self.CMD_DISCONNECT:
            self.streaming = False
            return self.generate_disconnect_response()
        return None

    def simulate_error_frame(self) -> bytes:
        """Génère une trame avec un checksum invalide."""
        return b"#D#100#200#300#9999#\n"

    def simulate_overflow_frame(self) -> bytes:
        """Génère une trame d'erreur overflow."""
        return b"#E#OVF#\n"

    def simulate_noise(self) -> bytes:
        """Génère une trame invalide (bruit)."""
        return b"#garbage#data#\n"

    def simulate_data_stream(self, num_frames: int = 5, interval: float = 0.1):
        """Génère `num_frames` trames de données à intervalle régulier."""
        frames = []
        for _ in range(num_frames):
            frames.append(self.generate_data_frame())
        return frames

    def simulate_sync_sequence(self) -> list[bytes]:
        """Simule la séquence complète de synchronisation."""
        return [self.generate_sync_response()]

    def simulate_full_session(self, num_frames: int = 10) -> list[bytes]:
        """Simule une session complète : sync, stream, stop."""
        sequence = []
        sequence.append(self.generate_sync_response())
        for _ in range(num_frames):
            sequence.append(self.generate_data_frame())
        sequence.append(self.generate_stop_response())
        return sequence
