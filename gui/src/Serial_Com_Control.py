"""
Serial_Com_Control.py
---------------------
Gestion de la communication série (UART) entre le PC et la carte STM32F407.

Responsabilités :
    - Lister les ports série disponibles
    - Ouvrir / fermer la connexion série
    - Synchronisation initiale avec la carte (handshake)
    - Réception du flux de données en continu (streaming)
    - Déconnexion propre

Auteur  : z_benakka193
Projet  : embedded-data-logger
"""

import threading
import time

import serial
import serial.tools.list_ports

from logger_config import setup_logger
logger = setup_logger("SerialControl")


class Serial_Control:
    """Contrôleur de la communication série avec la carte STM32."""

    SYNC_MAX_ATTEMPTS = 200

    def __init__(self):
        self.com_list  = []    # Liste des ports série disponibles
        self.ser       = None  # Objet serial.Serial (initialisé à l'ouverture)
        self.threading = False # Drapeau de contrôle des threads de lecture

    # ------------------------------------------------------------------
    # Gestion des ports disponibles
    # ------------------------------------------------------------------

    def get_com_list(self):
        """
        Scanne et retourne la liste des ports série disponibles sur le système.
        Un élément '-' est inséré en tête pour servir de valeur par défaut
        dans le menu déroulant de l'interface.
        """
        ports = serial.tools.list_ports.comports()
        self.com_list = [port[0] for port in ports]
        self.com_list.insert(0, "-")

    # ------------------------------------------------------------------
    # Ouverture / fermeture du port série
    # ------------------------------------------------------------------

    def serial_open(self, gui):
        """
        Ouvre le port série avec les paramètres sélectionnés dans l'interface.

        Si le port est déjà ouvert, la méthode ne fait rien.
        En cas d'échec, gui.serial.status est mis à False.

        Args:
            gui: Objet de l'interface graphique exposant clicked_com et clicked_bd.
        """
        port = gui.clicked_com.get()
        baud = gui.clicked_bd.get()

        try:
            if self.ser is not None and self.ser.is_open:
                logger.info(f"Port {port} deja ouvert.")
                self.ser.status = True
                return

            self.ser = serial.Serial(
                port=port,
                baudrate=baud,
                timeout=0.1
            )
            self.ser.status = True
            logger.info(f"Port {port} ouvert a {baud} baud.")

        except serial.SerialException as e:
            logger.error(f"Echec ouverture port {port} : {e}")
            if self.ser:
                self.ser.status = False

    def serial_close(self, gui):
        """
        Ferme le port série s'il est ouvert.

        Args:
            gui: Objet de l'interface (non utilisé ici, conservé pour cohérence).
        """
        try:
            if self.ser is not None and self.ser.is_open:
                self.ser.close()
                logger.info("Port ferme.")
            self.ser.status = False
        except Exception as e:
            logger.error(f"Erreur fermeture : {e}")
            if self.ser:
                self.ser.status = False

    def serial_disconnect(self, gui):
        """
        Envoie la commande de déconnexion à la carte puis ferme le port série.

        Args:
            gui: Objet de l'interface exposant gui.data (DataMaster).
        """
        if self.ser is None or not self.ser.is_open:
            logger.warning("Deconnexion ignoree : port deja ferme.")
            return

        try:
            self.ser.write(gui.data.CMD_DISCONNECT.encode())
            self.ser.close()
            logger.info("Deconnexion propre effectuee.")
        except Exception as e:
            logger.error(f"Erreur deconnexion : {e}")

    # ------------------------------------------------------------------
    # Synchronisation initiale (handshake)
    # ------------------------------------------------------------------

    def serial_sync(self, gui):
        """
        Thread de synchronisation avec la carte STM32.

        Envoie périodiquement la commande de sync et attend la réponse '!'.
        Met à jour l'interface selon le résultat (OK / failed).
        S'arrête après SYNC_MAX_ATTEMPTS tentatives infructueuses.

        Args:
            gui: Objet de l'interface exposant gui.data et gui.conn.
        """
        logger.info("Thread synchronisation demarre.")
        self.threading = True
        attempts = 0

        while self.threading:
            try:
                self.ser.write(gui.data.CMD_SYNC.encode())
                gui.conn.sync_status.config(text="..Sync..", fg="orange")

                gui.data.raw_msg = self.ser.readline()
                gui.data.decode_message()

                if len(gui.data.msg) >= 2 and gui.data.SYNC_OK in gui.data.msg[0] and int(gui.data.msg[1]) > 0:
                    self._on_sync_success(gui)
                    self.threading = False
                    break

            except Exception as e:
                logger.error(f"Erreur synchronisation : {e}")

            attempts += 1

            if attempts > self.SYNC_MAX_ATTEMPTS:
                attempts = 0
                gui.conn.sync_status.config(text="failed", fg="red")
                time.sleep(0.5)

            if not self.threading:
                break

        logger.info("Thread synchronisation termine.")

    def _on_sync_success(self, gui):
        """
        Actions effectuées lors d'une synchronisation réussie :
        activation des boutons, mise à jour des labels, initialisation des données.

        Args:
            gui: Objet de l'interface graphique.
        """
        nb_channels = int(gui.data.msg[1])

        for widget in [
            gui.conn.btn_start_stream,
            gui.conn.btn_add_chart,
            gui.conn.btn_kill_chart,
            gui.conn.save_check,
        ]:
            widget.config(state="active")

        gui.conn.sync_status.config(text="OK", fg="green")
        gui.conn.ch_status.config(text=str(nb_channels), fg="green")

        gui.data.synch_channel = nb_channels
        gui.data.generate_channels()
        gui.data.build_y_data()
        gui.data.set_filename()

        logger.info(f"Sync OK - {nb_channels} canal(aux) detecte(s).")

    # ------------------------------------------------------------------
    # Flux de données (streaming)
    # ------------------------------------------------------------------

    def serial_stop(self, gui):
        """
        Envoie la commande d'arrêt du flux de données à la carte.

        Args:
            gui: Objet de l'interface exposant gui.data.
        """
        self.ser.write(gui.data.CMD_STOP_STREAM.encode())

    def serial_data_stream(self, gui):
        """
        Thread de réception du flux de données depuis la carte STM32.

        Phase 1 : attente de la première trame valide pour caler le temps de référence.
        Phase 2 : réception continue — mise à jour des données X/Y et déclenchement
                  de la sauvegarde CSV si activée.

        Args:
            gui: Objet de l'interface exposant gui.data et gui.save.
        """
        self.threading = True

        # --- Phase 1 : première trame valide ---
        while self.threading:
            try:
                self.ser.write(gui.data.CMD_START_STREAM.encode())
                gui.data.raw_msg = self.ser.readline()
                gui.data.decode_message()
                gui.data.check_stream_data()

                if gui.data.stream_data_valid:
                    gui.data.set_ref_time()
                    break

            except Exception as e:
                logger.error(f"Erreur stream phase 1 : {e}")

        gui.update_chart()

        # --- Phase 2 : flux continu ---
        while self.threading:
            try:
                gui.data.raw_msg = self.ser.readline()
                gui.data.decode_message()
                gui.data.check_stream_data()

                if gui.data.stream_data_valid:
                    gui.data.update_x_data()
                    gui.data.update_y_data()
                    gui.data.adjust_display_window()

                    if gui.save:
                        t = threading.Thread(
                            target=gui.data.save_data,
                            args=(gui,),
                            daemon=True
                        )
                        t.start()

            except Exception as e:
                logger.error(f"Erreur stream phase 2 : {e}")


# ------------------------------------------------------------------
# Test standalone
# ------------------------------------------------------------------
if __name__ == "__main__":
    ctrl = Serial_Control()
    ctrl.get_com_list()
    logger.info(f"Ports disponibles : {ctrl.com_list}")
