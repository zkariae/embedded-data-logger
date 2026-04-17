"""
Data_communication_Control.py
------------------------------
Gestion des données échangées entre la carte STM32 et l'application Python.

Responsabilités :
    - Définition des commandes du protocole UART
    - Décodage des trames reçues
    - Gestion des buffers de données X (temps) et Y (capteurs)
    - Sauvegarde des données au format CSV
    - Fonctions d'affichage des courbes (données brutes ou converties en tension)

Auteur  : z_benakka193
Projet  : embedded-data-logger
"""

import csv
import time
from datetime import datetime

import numpy as np

from logger_config import setup_logger
logger = setup_logger("DataMaster")


class DataMaster:
    """
    Centralise les données de communication et les traitements associés.

    Attributs principaux :
        CMD_*           : commandes UART envoyées à la carte STM32
        SYNC_OK         : réponse attendue lors de la synchronisation
        synch_channel   : nombre de canaux actifs détectés à la synchronisation
        raw_msg         : dernière trame binaire reçue (bytes)
        msg             : trame décodée et découpée (liste de str)
        XData           : buffer des horodatages (secondes)
        YData           : buffer des valeurs par canal (liste de listes)
    """

    DISPLAY_TIME_RANGE = 5  # Fenêtre d'affichage glissante (secondes)

    def __init__(self):
        # ------------------------------------------------------------------
        # Commandes du protocole UART (envoyées à la carte STM32)
        # ------------------------------------------------------------------
        self.CMD_SYNC         = "#?#\n"   # Demande de synchronisation
        self.CMD_START_STREAM = "#A#\n"   # Démarrage du flux de données
        self.CMD_STOP_STREAM  = "#S#\n"   # Arrêt du flux de données
        self.CMD_DISCONNECT   = "#P#\n"   # Déconnexion de la carte

        self.SYNC_OK = "!"                # Préfixe de la réponse de sync réussie

        # ------------------------------------------------------------------
        # État de la communication
        # ------------------------------------------------------------------
        self.synch_channel     = 0        # Nombre de canaux actifs
        self.raw_msg           = b""      # Trame brute reçue (bytes)
        self.msg               = []       # Trame décodée (liste de chaînes)
        self.stream_data_valid = False    # True si la trame reçue est valide

        self.message_len       = 0        # Longueur attendue (champ de contrôle)
        self.message_len_check = 0        # Longueur calculée (vérification intégrité)

        # ------------------------------------------------------------------
        # Buffers de données
        # ------------------------------------------------------------------
        self.XData     = []             # Horodatages en secondes (axe temporel)
        self.YData     = []             # Valeurs capteurs par canal [[ch0], [ch1], ...]
        self.XDisplay  = np.array([])   # Tableau NumPy pour l'affichage
        self.YDisplay  = np.array([])   # Tableau NumPy pour l'affichage
        self.int_msg   = []             # Valeurs entières du cycle courant
        self._ref_time = 0.0            # Temps de référence (horodatages relatifs)
        self.filename  = ""             # Nom du fichier CSV courant

        # ------------------------------------------------------------------
        # Fonctions d'affichage disponibles
        # ------------------------------------------------------------------
        self.FunctionMaster = {
            "RowData":        self.plot_raw_data,
            "VoltageDisplay": self.plot_voltage_data,
        }

        # ------------------------------------------------------------------
        # Métadonnées des canaux
        # ------------------------------------------------------------------
        self.ChannelNum = {
            "Ch0": 0, "Ch1": 1, "Ch2": 2, "Ch3": 3,
            "Ch4": 4, "Ch5": 5, "Ch6": 6, "Ch7": 7,
        }

        self.ChannelColor = {
            "Ch0": "blue",    "Ch1": "green",   "Ch2": "red",
            "Ch3": "cyan",    "Ch4": "magenta",  "Ch5": "yellow",
            "Ch6": "black",   "Ch7": "white",
        }

        self.ChannelName = {
            "Ch0": "Voltage",     "Ch1": "Current",     "Ch2": "Temperature",
            "Ch3": "Pressure",    "Ch4": "Speed",        "Ch5": "Light",
            "Ch6": "Energy",      "Ch7": "Force",
        }

    # ------------------------------------------------------------------
    # Initialisation des structures de données
    # ------------------------------------------------------------------

    def generate_channels(self):
        """
        Construit la liste des identifiants de canaux actifs.
        Exemple pour 3 canaux : ['Ch0', 'Ch1', 'Ch2']
        """
        self.Channels = [f"Ch{i}" for i in range(self.synch_channel)]

    def build_y_data(self):
        """
        Initialise le buffer YData avec une liste vide par canal actif.
        Doit être appelé après generate_channels().
        """
        self.YData = [[] for _ in range(self.synch_channel)]

    def set_filename(self):
        """
        Génère un nom de fichier CSV horodaté pour la session courante.
        Format : YYYYMMDDHHMMSS.csv
        """
        self.filename = datetime.now().strftime("%Y%m%d%H%M%S") + ".csv"

    def clear_data(self):
        """Réinitialise tous les buffers de données (appelé à la déconnexion)."""
        self.raw_msg = b""
        self.msg     = []
        self.XData   = []
        self.YData   = []

    # ------------------------------------------------------------------
    # Décodage des trames UART
    # ------------------------------------------------------------------

    def decode_message(self):
        """
        Décode la trame binaire reçue (raw_msg) en une liste de valeurs (msg).

        Format attendu :
            #D#<val0>#<val1>#...#<valN>#<longueur_totale>#

        Le champ 'D' indique une trame de données.
        Le dernier champ est la longueur cumulée (contrôle d'intégrité).
        """
        try:
            decoded = self.raw_msg.decode("utf-8").strip()
            logger.debug(f"Trame recue : {decoded.strip()}")
        except UnicodeDecodeError as e:
            logger.error(f"Erreur decodage UTF-8 : {e}")
            return

        if not decoded or "#" not in decoded:
            return

        self.msg = decoded.split("#")
        del self.msg[0]

        logger.debug(f"Message decode : {self.msg}")

        if self.msg and self.msg[0] == "D":
            self.message_len       = 0
            self.message_len_check = 0

            del self.msg[0]                       # Supprime le marqueur 'D'
            del self.msg[-1]                      # Supprime le dernier élément vide

            self.message_len = int(self.msg[-1])  # Longueur attendue (dernier champ)
            del self.msg[-1]

            for item in self.msg:
                self.message_len_check += len(item)

    def _parse_int_msg(self):
        """
        Convertit les chaînes du message décodé en entiers.
        Les valeurs non numériques sont ignorées.
        """
        self.int_msg = [
            int(val.strip())
            for val in self.msg
            if val.strip().isdigit()
        ]

    def check_stream_data(self):
        """
        Valide la trame courante :
            - nombre de valeurs == nombre de canaux actifs
            - longueur calculée == longueur attendue (intégrité)

        Met à jour stream_data_valid et int_msg.
        """
        self.stream_data_valid = False

        if self.synch_channel == len(self.msg):
            if self.message_len == self.message_len_check:
                self.stream_data_valid = True
                self._parse_int_msg()

    # ------------------------------------------------------------------
    # Gestion du temps de référence
    # ------------------------------------------------------------------

    def set_ref_time(self):
        """
        Initialise le temps de référence pour les horodatages relatifs.
        Assure la continuité après une reprise du flux.
        """
        if len(self.XData) == 0:
            self._ref_time = time.perf_counter()
        else:
            self._ref_time = time.perf_counter() - self.XData[-1]

    # ------------------------------------------------------------------
    # Mise à jour des buffers
    # ------------------------------------------------------------------

    def update_x_data(self):
        """
        Ajoute le timestamp courant (secondes depuis la référence) à XData.
        La première valeur est toujours 0.
        """
        if len(self.XData) == 0:
            self.XData.append(0.0)
        else:
            self.XData.append(time.perf_counter() - self._ref_time)

    def update_y_data(self):
        """
        Ajoute les valeurs entières du cycle courant à chaque canal dans YData.
        """
        for ch_index in range(self.synch_channel):
            self.YData[ch_index].append(self.int_msg[ch_index])

    def adjust_display_window(self):
        """
        Applique une fenêtre glissante sur les données pour l'affichage.

        Supprime les échantillons les plus anciens si la durée dépasse
        DISPLAY_TIME_RANGE secondes.
        Construit les tableaux NumPy XDisplay et YDisplay.
        """
        if (self.XData[-1] - self.XData[0]) > self.DISPLAY_TIME_RANGE:
            del self.XData[0]
            for y_channel in self.YData:
                del y_channel[0]

        x = np.array(self.XData)
        self.XDisplay = np.linspace(x.min(), x.max(), len(x), endpoint=False)
        self.YDisplay = np.array(self.YData)

    # ------------------------------------------------------------------
    # Sauvegarde CSV
    # ------------------------------------------------------------------

    def save_data(self, gui):
        """
        Ajoute une ligne au fichier CSV si la sauvegarde est activée.

        Format : [timestamp, val_ch0, val_ch1, ..., val_chN]

        Args:
            gui: Objet exposant gui.save (bool).
        """
        if not gui.save:
            return

        row = list(self.int_msg)
        row.insert(0, self.XData[-1])

        with open(self.filename, "a", newline="") as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(row)

    # ------------------------------------------------------------------
    # Fonctions de tracé
    # ------------------------------------------------------------------

    def plot_raw_data(self, gui):
        """
        Trace les données brutes (valeurs ADC) sur le graphe.

        Args:
            gui: Objet exposant gui.chart, gui.x, gui.y, gui.color.
        """
        gui.chart.plot(
            gui.x, gui.y,
            color=gui.color,
            dash_capstyle="projecting",
            linewidth=1
        )

    def plot_voltage_data(self, gui):
        """
        Trace les données converties en tension (V).

        Conversion : V = (valeur_ADC / 4096) × 3.3 V
        ADC 12 bits, Vref = 3.3 V

        Args:
            gui: Objet exposant gui.chart, gui.x, gui.y, gui.color.
        """
        voltage = (gui.y / 4096) * 3.3
        gui.chart.plot(
            gui.x, voltage,
            color=gui.color,
            dash_capstyle="projecting",
            linewidth=1
        )


# ------------------------------------------------------------------
# Test standalone
# ------------------------------------------------------------------
if __name__ == "__main__":
    data = DataMaster()
    data.synch_channel = 3
    data.generate_channels()
    data.build_y_data()
    data.set_filename()
    logger.info(f"Canaux : {data.Channels}")
    logger.info(f"Fichier CSV : {data.filename}")
