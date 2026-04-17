"""
logger_config.py
----------------
Configuration du système de logging de l'application embedded-data-logger.

Niveaux disponibles :
    DEBUG   : messages de debug (trames UART, valeurs capteurs)
    INFO    : messages d'information (connexion, sync, etat)
    WARNING : avertissements (config manquante, port indisponible)
    ERROR   : erreurs (echec connexion, trame invalide)

Auteur  : z_benakka193
Projet  : embedded-data-logger
"""

import logging
import os
from datetime import datetime

# ------------------------------------------------------------------
# Repertoire des logs
# ------------------------------------------------------------------
LOG_DIR = os.path.join(os.path.dirname(__file__), "..", "logs")
os.makedirs(LOG_DIR, exist_ok=True)

# Nom du fichier log horodate
LOG_FILE = os.path.join(LOG_DIR, datetime.now().strftime("%Y%m%d_%H%M%S") + ".log")

# ------------------------------------------------------------------
# Format des messages
# ------------------------------------------------------------------
LOG_FORMAT  = "%(asctime)s [%(levelname)-8s] %(name)-25s : %(message)s"
DATE_FORMAT = "%Y-%m-%d %H:%M:%S"

# ------------------------------------------------------------------
# Configuration globale
# ------------------------------------------------------------------

def setup_logger(name: str, level=logging.DEBUG) -> logging.Logger:
    """
    Cree et retourne un logger configure pour le module demande.

    Args:
        name  (str)           : nom du logger (ex. 'SerialControl')
        level (logging.LEVEL) : niveau minimum de logging

    Returns:
        logging.Logger : logger configure avec handler console et fichier
    """
    logger = logging.getLogger(name)
    logger.setLevel(level)

    # Eviter les doublons si le logger existe deja
    if logger.handlers:
        return logger

    formatter = logging.Formatter(LOG_FORMAT, datefmt=DATE_FORMAT)

    # --- Handler console ---
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    console_handler.setFormatter(formatter)

    # --- Handler fichier ---
    file_handler = logging.FileHandler(LOG_FILE, encoding="utf-8")
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(formatter)

    logger.addHandler(console_handler)
    logger.addHandler(file_handler)

    return logger