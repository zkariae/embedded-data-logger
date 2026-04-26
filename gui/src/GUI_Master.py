"""
GUI_Master.py
-------------
Interface graphique principale de l'application embedded-data-logger.

Contient quatre classes :
    - RootGUI   : fenêtre principale, authentification, sélection du mode de communication
    - ComGui    : panneau de gestion du port série (sélection, connexion, configuration)
    - ConnGUI   : panneau de contrôle du flux de données (sync, start/stop, charts, sauvegarde)
    - DisGUI    : gestionnaire des graphiques Matplotlib embarqués dans Tkinter

Auteur  : z_benakka193
Projet  : embedded-data-logger
"""

from tkinter import *
from tkinter import ttk
import json
import os
import threading

import tkinter as tk
from tkinter import IntVar, LabelFrame, Label, Button, Entry
from tkinter import OptionMenu, StringVar, Checkbutton, N, NW
from tkinter import messagebox

import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from functools import partial
from datetime import datetime
from logger_config import setup_logger




BG_COLOR     = "#1e1e1e"   # Fond principal noir
FRAME_COLOR  = "#2d2d2d"   # Fond des frames
TEXT_COLOR   = "#ffffff"   # Texte blanc
ACCENT_COLOR = "#00ff99"   # Couleur accent vert
BTN_COLOR    = "#3d3d3d"   # Fond boutons
BTN_TEXT     = "#ffffff"   # Texte 

PLOT_BG_COLOR = "#e8e8e8"   # Fond graphique gris clair (style Matlab)
PLOT_TEXT_COLOR = "#ffffff"  # Texte axes blanc
PLOT_GRID_COLOR = "#000000"   # Grille noire

# ==============================================================================
# Dimensions de l'interface — 1920x1080, 2 colonnes, 4 graphiques max
# ==============================================================================

WINDOW_W_SINGLE = 1400   # Largeur fenetre avec 1 graphique
WINDOW_W_MULTI  = 1900   # Largeur fenetre avec 2+ graphiques
WINDOW_H_SINGLE = 640   # Hauteur fenetre avec 1 graphique
WINDOW_H_BASE   = 150    # Hauteur header (Com + Connection Manager)
WINDOW_H_CHART  = 480    # Hauteur par ligne de graphiques

FIG_W_SINGLE = 8    # Largeur graphique unique (pouces)
FIG_H_SINGLE = 5    # Hauteur graphique unique (pouces)
FIG_W_MULTI  = 7    # Largeur graphiques multiples (pouces)
FIG_H_MULTI  = 4    # Hauteur graphiques multiples (pouces)
FIG_DPI      = 90   # Resolution

# ==============================================================================
# RootGUI — Fenêtre principale et authentification
# ==============================================================================

class RootGUI():
    """
    Fenêtre principale de l'application embedded-data-logger.

    Responsabilités :
        - Afficher l'écran de connexion (username / password)
        - Vérifier les identifiants depuis users.json
        - Permettre la réinitialisation du mot de passe par e-mail
        - Rediriger vers le mode de communication choisi (Série ou SSH)
        - Gérer la fermeture propre de l'application
    """

    def __init__(self, serial, data):
        """
        Initialise la fenêtre principale et construit les widgets.

        Args:
            serial : instance de Serial_Control
            data   : instance de DataMaster
        """
        # Création de la fenêtre principale Tkinter
        self.root = tk.Tk()
        self.root.title("Data Management")
        self.root.config(bg=BG_COLOR)

        # Références aux objets de communication et de données
        self.serial = serial
        self.data   = data

        # Chemin vers le fichier de gestion des utilisateurs
        self.users_file = os.path.join(os.path.dirname(__file__), "..", "config", "users.json")

        # Construction des cadres de l'interface
        self._build_login_frame()
        self._build_comm_frame()

        # Chargement du mot de passe de secours (fichier legacy password.txt)
        self.saved_password = self._load_legacy_password()

        # Interception de la fermeture de la fenêtre (clic sur la croix)
        self.root.protocol("WM_DELETE_WINDOW", self.close_window)
        
        self.logger = setup_logger("RootGUI")

    # ------------------------------------------------------------------
    # Construction des widgets
    # ------------------------------------------------------------------

    def _build_login_frame(self):
        """
        Construit le cadre de login contenant :
            - Champ username
            - Champ password (masqué, avec option d'affichage)
            - Bouton Connexion
            - Lien 'Forgot password?'
            - Label pour les messages d'erreur / confirmation
        """
        # LabelFrame
        login_frame = LabelFrame(self.root, text="Log in",
            padx=15, pady=15, bg=FRAME_COLOR, fg=TEXT_COLOR)
        login_frame.pack(padx=20, pady=20, fill="x")

        # --- Champ username ---
        Label(login_frame, text="User name :", bg=FRAME_COLOR, fg=TEXT_COLOR).grid(
            row=0, column=0, sticky="e", pady=5, padx=5)
        self.username_entry = Entry(login_frame, width=30,
            bg=BTN_COLOR, fg=TEXT_COLOR, insertbackground=TEXT_COLOR)
        self.username_entry.grid(row=0, column=1, pady=5, padx=5, sticky="ew")

        # --- Champ password ---
        Label(login_frame, text="Password :",  bg=FRAME_COLOR, fg=TEXT_COLOR).grid(
            row=1, column=0, sticky="e", pady=5, padx=5)
        self.password_entry = Entry(login_frame, width=30, show="*",
            bg=BTN_COLOR, fg=TEXT_COLOR, insertbackground=TEXT_COLOR)
        self.password_entry.grid(row=1, column=1, pady=5, padx=5, sticky="ew")

        # Case à cocher pour afficher / masquer le mot de passe
        self.show_password_var = tk.BooleanVar()
        tk.Checkbutton(login_frame, text="Display",
            variable=self.show_password_var,
            bg=FRAME_COLOR, fg=TEXT_COLOR,
            selectcolor=BTN_COLOR,
            activebackground=FRAME_COLOR,
            activeforeground=TEXT_COLOR,
            command=self._toggle_password).grid(row=1, column=1, sticky="e", padx=(5, 0))

        # --- Bouton Connexion ---
        Button(login_frame, text="Connexion", width=20,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR,
            activeforeground=BG_COLOR,
            command=self.check_login).grid(row=2, column=0, columnspan=2, pady=10)

        # --- Label messages d'erreur / confirmation ---
        self.message_label = Label(login_frame, text="",
            fg="red", bg=FRAME_COLOR)
        self.message_label.grid(row=3, column=0, columnspan=2, pady=5)

        # --- Lien 'Forgot password?' ---
        forgot_link = Label(login_frame, text="Forgot password?",
            fg=ACCENT_COLOR, cursor="hand2", bg=FRAME_COLOR,
            font=("Arial", 9, "underline"))
        forgot_link.grid(row=4, column=0, columnspan=2, pady=(0, 5))
        forgot_link.bind("<Button-1>", self.forgot_password)

        # Ajustement automatique des colonnes
        login_frame.columnconfigure(0, weight=1)
        login_frame.columnconfigure(1, weight=2)

    def _build_comm_frame(self):
        """
        Construit le cadre de sélection du mode de communication contenant :
            - Bouton Serial (désactivé jusqu'au login réussi)
            - Bouton SSH    (désactivé jusqu'au login réussi)
        """
        comm_frame = LabelFrame(self.root,
            text="Specify communication method",
            padx=15, pady=15,
            bg=FRAME_COLOR, fg=TEXT_COLOR)
        comm_frame.pack(padx=20, pady=10, fill="x")

        self.btn_serial = Button(comm_frame, text="Serial",
            state="disabled", width=15,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR,
            activeforeground=BG_COLOR,
            command=self.open_serial)
        self.btn_serial.grid(row=0, column=0, padx=10, pady=5)

        self.btn_ssh = Button(comm_frame, text="SSH",
            state="disabled", width=15,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR,
            activeforeground=BG_COLOR,
            command=self.open_ssh)
        self.btn_ssh.grid(row=0, column=1, padx=10, pady=5)

        comm_frame.columnconfigure(0, weight=1)
        comm_frame.columnconfigure(1, weight=1)

    # ------------------------------------------------------------------
    # Gestion des utilisateurs (users.json)
    # ------------------------------------------------------------------

    def load_users(self):
        """
        Charge le dictionnaire des utilisateurs depuis users.json.

        Returns:
            dict : {username: {"password": ..., "email": ...}}
                   ou {} si le fichier est absent.
        """
        if os.path.exists(self.users_file):
            with open(self.users_file, "r") as f:
                return json.load(f)
        return {}

    def save_users(self, users):
        """
        Sauvegarde le dictionnaire des utilisateurs dans users.json.

        Args:
            users (dict) : dictionnaire des utilisateurs à persister.
        """
        with open(self.users_file, "w") as f:
            json.dump(users, f, indent=4)

    def _load_legacy_password(self):
        """
        Lit le mot de passe de secours depuis password.txt (fichier legacy).

        Returns:
            str : mot de passe lu, ou '1234' par défaut si le fichier est absent.
        """
        if os.path.exists(os.path.join(os.path.dirname(__file__), "..", "config", "password.txt")):
            with open(os.path.join(os.path.dirname(__file__), "..", "config", "password.txt"), "r") as f:
                return f.read().strip()
        return "1234"

    def email_exists(self, email):
        """
        Vérifie si un e-mail est enregistré dans users.json.

        Args:
            email (str) : adresse e-mail à rechercher.

        Returns:
            bool : True si l'e-mail existe, False sinon.
        """
        users = self.load_users()
        return any(user_data["email"] == email for user_data in users.values())

    def get_username_from_email(self, email):
        """
        Retourne le nom d'utilisateur associé à un e-mail.

        Args:
            email (str) : adresse e-mail recherchée.

        Returns:
            str | None : nom d'utilisateur, ou None si non trouvé.
        """
        users = self.load_users()
        for username, data in users.items():
            if data["email"] == email:
                return username
        return None

    # ------------------------------------------------------------------
    # Authentification
    # ------------------------------------------------------------------

    def check_login(self):
        """
        Vérifie les identifiants saisis et active les boutons de communication
        si l'authentification réussit.

        Returns:
            bool : True si le login est valide, False sinon.
        """
        username = self.username_entry.get()
        password = self.password_entry.get()
        users    = self.load_users()

        if username in users and users[username]["password"] == password:
            self.btn_serial.config(state="active")
            self.btn_ssh.config(state="active")
            messagebox.showinfo("Login successful", f"Welcome, {username}!")
            return True

        messagebox.showerror("Error", "Invalid username or password")
        return False

    def _toggle_password(self):
        """Affiche ou masque le mot de passe selon l'état de la case à cocher."""
        show = "" if self.show_password_var.get() else "*"
        self.password_entry.config(show=show)

    # ------------------------------------------------------------------
    # Réinitialisation du mot de passe
    # ------------------------------------------------------------------

    def forgot_password(self, event=None):
        """
        Ouvre une fenêtre popup pour saisir l'e-mail de réinitialisation.
        Redirige vers new_password_window() si l'e-mail est reconnu.
        """
        popup = tk.Toplevel(self.root)
        popup.title("Password Reset")
        popup.geometry("300x150")

        tk.Label(popup, text="Enter your email:", pady=10).pack()
        email_entry = tk.Entry(popup, width=30)
        email_entry.pack(pady=5)

        def validate_email():
            email = email_entry.get()
            if self.email_exists(email):
                popup.destroy()
                self.new_password_window(email)
            else:
                messagebox.showerror("Error", "This email is not registered")

        tk.Button(popup, text="Next", command=validate_email).pack(pady=10)

    def new_password_window(self, email):
        """
        Ouvre une fenêtre pour saisir et confirmer le nouveau mot de passe.

        Args:
            email (str) : e-mail de l'utilisateur ayant demandé la réinitialisation.
        """
        popup = tk.Toplevel(self.root)
        popup.title("Set New Password")
        popup.geometry("300x180")

        tk.Label(popup, text=f"Reset for: {email}", pady=10).pack()

        tk.Label(popup, text="New password:").pack()
        new_pass_entry = tk.Entry(popup, width=30, show="*")
        new_pass_entry.pack(pady=5)

        tk.Label(popup, text="Confirm password:").pack()
        confirm_pass_entry = tk.Entry(popup, width=30, show="*")
        confirm_pass_entry.pack(pady=5)

        tk.Button(
            popup, text="Save",
            command=lambda: self.save_new_password(
                email,
                new_pass_entry.get(),
                confirm_pass_entry.get(),
                popup
            )
        ).pack(pady=10)

    def save_new_password(self, email, new_pass, confirm_pass, popup):
        """
        Valide et enregistre le nouveau mot de passe dans users.json.

        Args:
            email        (str)         : e-mail de l'utilisateur
            new_pass     (str)         : nouveau mot de passe saisi
            confirm_pass (str)         : confirmation du mot de passe
            popup        (tk.Toplevel) : fenêtre popup à fermer après succès
        """
        if not new_pass or new_pass != confirm_pass:
            self.message_label.config(text="Passwords do not match!", fg="red")
            return

        users    = self.load_users()
        username = self.get_username_from_email(email)
        users[username]["password"] = new_pass
        self.save_users(users)

        popup.destroy()
        self.message_label.config(text="Password updated successfully", fg="green")

    # ------------------------------------------------------------------
    # Navigation entre les modes de communication
    # ------------------------------------------------------------------

    def open_serial(self):
        """
        Efface les widgets de la fenêtre principale et charge
        l'interface de communication série (ComGui).
        """
        for widget in self.root.winfo_children():
            widget.destroy()
        self.root.title("Serial Communication")
        self.root.geometry("1000x120")
        ComGui(self.root, self.serial, self.data)

    def open_ssh(self):
        """
        Efface les widgets de la fenêtre principale et affiche
        un placeholder pour le mode SSH (à implémenter).
        """
        for widget in self.root.winfo_children():
            widget.destroy()
        self.root.title("SSH Communication")
        Label(
            self.root,
            text="SSH interface — à implémenter",
            font=("Arial", 14)
        ).pack(pady=30)
        Button(self.root, text="Fermer", command=self.close_window).pack()

    # ------------------------------------------------------------------
    # Fermeture de l'application
    # ------------------------------------------------------------------

    def close_window(self):
        """
        Ferme proprement la connexion série active puis détruit
        la fenêtre principale Tkinter.
        Appelée lors du clic sur la croix de fermeture.
        """
        self.logger.info("Fermeture de l'application.")
        self.root.destroy()
        try:
            self.serial.serial_disconnect(self)
            self.serial.serial_close(self)
            self.serial.threading = False
        except Exception as e:
            self.logger.error(f"Erreur fermeture : {e}")



# ==============================================================================
# ComGui — Panneau de gestion de la connexion série
# ==============================================================================

class ComGui():
    """
    Panneau de sélection et de connexion au port série.

    Responsabilités :
        - Afficher les menus déroulants port COM et baudrate
        - Rafraîchir la liste des ports disponibles
        - Établir / fermer la connexion série
        - Sauvegarder et charger automatiquement la configuration (com_config.json)
    """

    # Liste des baudrates supportés
    BAUD_RATES = [
        "-", "300", "600", "1200", "2400", "4800", "9600",
        "14400", "19200", "28800", "38400", "56000", "57600",
        "115200", "128000", "256000"
    ]

    def __init__(self, root, serial, data):
        """
        Initialise le panneau Com Manager et charge la configuration sauvegardée.

        Args:
            root   : fenêtre Tkinter parente
            serial : instance de Serial_Control
            data   : instance de DataMaster
        """
        self.root   = root
        self.serial = serial
        self.data   = data
        self.padx   = 20
        self.pady   = 5

        # Chemin du fichier de configuration série
        self.config_file = os.path.join(
            os.path.dirname(__file__), "..", "config", "com_config.json"
        )

        # Cadre principal du panneau  
        self.frame = LabelFrame(root, text="Com Manager",
            padx=5, pady=5,
            bg=FRAME_COLOR, fg=TEXT_COLOR)

        # Labels des menus déroulants
        self.label_com = Label(self.frame, text="Available Port(s): ",
            bg=FRAME_COLOR, fg=TEXT_COLOR, width=15, anchor="w")
        self.label_bd  = Label(self.frame, text="Baud Rate: ",
            bg=FRAME_COLOR, fg=TEXT_COLOR, width=15, anchor="w")

        # Construction des menus déroulants
        self._build_baud_menu()
        self._build_com_menu()

        # Boutons de contrôle
        self.btn_refresh = Button(self.frame, text="Refresh", width=10,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            command=self.refresh_btn)

        self.btn_connect = Button(self.frame, text="Connect", width=10,
            state="disabled",
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            command=self.serial_connect)

        self.btn_save_config = Button(self.frame, text="Save Configuration",
            width=20, state="disabled",
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            command=self.save_configuration)

        self.logger = setup_logger("ComGui")  
        # Placement des widgets dans la grille
        self._publish()

        # Chargement automatique de la configuration au démarrage
        self.auto_load_configuration()

        

    # ------------------------------------------------------------------
    # Construction des menus déroulants
    # ------------------------------------------------------------------

    def _build_com_menu(self):
        """
        Scanne les ports série disponibles et construit le menu déroulant COM.
        La valeur par défaut est '-' (aucun port sélectionné).
        """
        self.serial.get_com_list()
        self.clicked_com = StringVar()
        self.clicked_com.set(self.serial.com_list[0])
        self.drop_com = OptionMenu(self.frame, self.clicked_com,
            *self.serial.com_list, command=self.connect_ctrl)
        self.drop_com.config(width=10,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR)
        self.drop_com["menu"].config(bg=BTN_COLOR, fg=BTN_TEXT)
    
    def _build_baud_menu(self):
        """
        Construit le menu déroulant des baudrates disponibles.
        La valeur par défaut est '-' (aucun baudrate sélectionné).
        """
        self.clicked_bd = StringVar()
        self.clicked_bd.set(self.BAUD_RATES[0])
        self.drop_baud = OptionMenu(self.frame, self.clicked_bd,
            *self.BAUD_RATES, command=self.connect_ctrl)
        self.drop_baud.config(width=10,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR)
        self.drop_baud["menu"].config(bg=BTN_COLOR, fg=BTN_TEXT)    

    def _publish(self):
        """Place tous les widgets du panneau dans la grille Tkinter."""
        self.frame.grid(row=0, column=0, rowspan=3, columnspan=3, padx=5, pady=5)
        self.label_com.grid(column=1, row=2)
        self.drop_com.grid(column=2,  row=2, padx=self.padx)
        self.label_bd.grid(column=1,  row=3)
        self.drop_baud.grid(column=2, row=3, padx=self.padx, pady=self.pady)
        self.btn_refresh.grid(column=3,     row=2)
        self.btn_connect.grid(column=3,     row=3)
        self.btn_save_config.grid(column=4, row=2, padx=self.padx)

    # ------------------------------------------------------------------
    # Contrôle de l'état des boutons
    # ------------------------------------------------------------------

    def connect_ctrl(self, value):
        """
        Active ou désactive le bouton Connect selon les sélections courantes.
        Le bouton n'est actif que si un port ET un baudrate valides sont choisis.

        Args:
            value : valeur sélectionnée (fournie automatiquement par OptionMenu).
        """
        if "-" in self.clicked_com.get() or "-" in self.clicked_bd.get():
            self.btn_connect.config(state="disabled")
            self.btn_save_config.config(state="disabled")
        else:
            self.btn_connect.config(state="active")
            self.btn_save_config.config(state="active")

    # ------------------------------------------------------------------
    # Rafraîchissement des ports
    # ------------------------------------------------------------------

    def refresh_btn(self):
        """
        Rafraîchit la liste des ports série disponibles :
            - Supprime le fichier de configuration existant
            - Détruit et recrée les menus déroulants COM et baudrate
            - Réinitialise l'état du bouton Connect
        """
        # Suppression de la configuration sauvegardée
        if os.path.exists(self.config_file):
            os.remove(self.config_file)

        # Reconstruction des menus déroulants
        self.drop_com.destroy()
        self.drop_baud.destroy()
        self._build_com_menu()
        self._build_baud_menu()

        # Replacement dans la grille
        self.drop_com.grid(column=2,  row=2, padx=self.padx)
        self.drop_baud.grid(column=2, row=3, padx=self.padx, pady=self.pady)

        # Mise à jour de l'état du bouton Connect
        self.connect_ctrl(None)

    # ------------------------------------------------------------------
    # Connexion / déconnexion série
    # ------------------------------------------------------------------

    def serial_connect(self):
        """
        Établit ou ferme la connexion série selon l'état du bouton Connect.

        Connexion (bouton = 'Connect') :
            - Ouvre le port série
            - Désactive les menus de sélection
            - Crée le panneau ConnGUI
            - Lance le thread de synchronisation (serial_sync)

        Déconnexion (bouton = 'Disconnect') :
            - Arrête le flux de données
            - Ferme le port série
            - Détruit le panneau ConnGUI et les graphiques
            - Réinitialise l'interface
        """
        if self.btn_connect["text"] == "Connect":

            self.serial.serial_open(self)

            if self.serial.ser and self.serial.ser.status:
                # Mise à jour de l'interface après connexion réussie
                self.btn_connect.config(text="Disconnect")
                self.btn_refresh.config(state="disabled")
                self.drop_baud.config(state="disabled")
                self.drop_com.config(state="disabled")

                messagebox.showinfo(
                    "Connection",
                    f"UART connected on {self.clicked_com.get()}"
                )

                # Création du panneau de contrôle ConnGUI
                self.conn = ConnGUI(self.root, self.serial, self.data, self)

                # Lancement du thread de synchronisation avec la carte STM32
                self.serial.t1 = threading.Thread(
                    target=self.serial.serial_sync,
                    args=(self,),
                    daemon=True   # Le thread se termine avec le programme principal
                )
                self.serial.t1.start()

            else:
                messagebox.showerror(
                    "Connection error",
                    f"Failed to open {self.clicked_com.get()}"
                )

        else:
            # --- Déconnexion ---
            self.serial.serial_disconnect(self)
            self.serial.threading = False
            self.conn.save = False
            self.serial.serial_close(self)

            # Fermeture du panneau ConnGUI et des graphiques
            self.conn.conn_gui_close()
            try:
                self.conn.kill_chart()
            except Exception as e:
                self.logger.error(f"Impossible de fermer le graphique : {e}")

            self.data.clear_data()

            messagebox.showwarning(
                "Disconnected",
                f"UART connection on {self.clicked_com.get()} closed."
            )

            # Réinitialisation de l'interface
            self.btn_connect.config(text="Connect")
            self.btn_refresh.config(state="active")
            self.drop_baud.config(state="active")
            self.drop_com.config(state="active")

    # ------------------------------------------------------------------
    # Sauvegarde / chargement de la configuration
    # ------------------------------------------------------------------

    def save_configuration(self):
        """
        Sauvegarde le port COM et le baudrate sélectionnés dans com_config.json.
        Affiche un avertissement si aucune valeur valide n'est sélectionnée.
        """
        current_port = self.clicked_com.get()
        current_baud = self.clicked_bd.get()

        if current_port == "-" or current_baud == "-":
            messagebox.showwarning(
                "Incomplete configuration",
                "Please select a COM port and a baud rate before saving."
            )
            return

        config = {
            "port":       current_port,
            "baud_rate":  current_baud,
            "last_saved": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        }

        try:
            os.makedirs(os.path.dirname(self.config_file), exist_ok=True)
            with open(self.config_file, "w") as f:
                json.dump(config, f, indent=4)

            self.logger.info(f"Config sauvegardee — Port={current_port}, Baud={current_baud}")
            messagebox.showinfo(
                "Configuration saved",
                f"Port: {current_port}\nBaud Rate: {current_baud}"
            )
        except Exception as e:
            self.logger.error(f"Erreur sauvegarde config : {e}")
            messagebox.showerror("Error", f"Cannot save configuration: {e}")

    def auto_load_configuration(self):
        """
        Charge automatiquement la configuration depuis com_config.json au démarrage.
        Active le bouton Connect si le port et le baudrate chargés sont valides.

        Returns:
            bool : True si le chargement a réussi, False sinon.
        """
        if not os.path.exists(self.config_file):
            self.logger.warning("Aucune configuration sauvegardee trouvee.")
            return False

        try:
            with open(self.config_file, "r") as f:
                config = json.load(f)

            saved_port = config.get("port", "-")
            saved_baud = config.get("baud_rate", "-")

            # Application du port sauvegardé s'il est disponible
            if saved_port in self.serial.com_list:
                self.clicked_com.set(saved_port)
                self.logger.info(f"Port '{saved_port}' charge automatiquement.")
            else:
                self.logger.warning(f"Port '{saved_port}' non disponible.")

            # Application du baudrate sauvegardé s'il est valide
            if saved_baud in self.BAUD_RATES:
                self.clicked_bd.set(saved_baud)
                self.logger.info(f"Baud rate '{saved_baud}' charge automatiquement.")
            else:
                self.logger.warning(f"Baud rate '{saved_baud}' non valide.")

            # Activation du bouton Connect si les deux valeurs sont valides
            if (saved_port in self.serial.com_list and saved_baud in self.BAUD_RATES
                    and saved_port != "-" and saved_baud != "-"):
                self.btn_connect.config(state="active")
                self.logger.info("Bouton Connect active automatiquement.")

            return True

        except Exception as e:
            self.logger.error(f"Erreur chargement automatique : {e}")
            return False


# ==============================================================================
# ConnGUI — Panneau de contrôle du flux de données
# ==============================================================================

class ConnGUI():
    """
    Panneau de contrôle affiché après la connexion série réussie.

    Responsabilités :
        - Afficher le statut de synchronisation et le nombre de canaux actifs
        - Démarrer / arrêter le flux de données (streaming)
        - Ajouter / supprimer des graphiques dynamiquement
        - Activer / désactiver la sauvegarde CSV
        - Rafraîchir les graphiques toutes les 40 ms (boucle Tkinter after)
        - Fermer proprement tous les widgets à la déconnexion
    """

    def __init__(self, root, serial, data, com_gui):
        """
        Initialise le panneau Connection Manager et crée le premier graphique
        automatiquement après 500 ms (délai pour attendre la fin de la sync).

        Args:
            root   : fenêtre Tkinter parente
            serial : instance de Serial_Control
            data   : instance de DataMaster
        """
        self.root   = root
        self.serial = serial
        self.data   = data
        self.save   = False
        self.com_gui = com_gui   # Reference vers ComGui
        self.padx   = 20
        self.pady   = 15

        # Cadre principal du panneau
        self.frame = LabelFrame(root, text="Connection Manager",
            padx=5, pady=5,
            bg=FRAME_COLOR, fg=TEXT_COLOR, width=60)

        # --- Labels de statut ---
        self.sync_label  = Label(self.frame, text="Sync Status: ",
            bg=FRAME_COLOR, fg=TEXT_COLOR, width=15, anchor="w")
        self.sync_status = Label(self.frame, text="..Sync..",
            bg=FRAME_COLOR, fg="orange", width=5)
        self.ch_label    = Label(self.frame, text="Active channels: ",
            bg=FRAME_COLOR, fg=TEXT_COLOR, width=15, anchor="w")
        self.ch_status   = Label(self.frame, text="...",
            bg=FRAME_COLOR, fg="orange", width=5)

        # --- Boutons Start / Stop du flux ---
        self.btn_start_stream = Button(self.frame, text="Start",
            state="disabled", width=5, bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            command=self.start_stream)

        self.btn_stop_stream = Button(self.frame, text="Stop",
            state="disabled", width=5, bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            command=self.stop_stream)

        # --- Boutons gestion des graphiques ---
        self.btn_add_chart = Button(self.frame, text="Add Chart",
            state="disabled", width=10, bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            command=self.new_chart)

        self.btn_kill_chart = Button(self.frame, text="Delete Chart",
            state="disabled", width=10, bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            command=self.kill_chart)

        # --- Case à cocher sauvegarde CSV ---
        self.save_var   = IntVar()
        self.save_check = Checkbutton(self.frame, text="Save data",
            variable=self.save_var,
            onvalue=1, offvalue=0,
            bg=FRAME_COLOR, fg=TEXT_COLOR,
            selectcolor=BTN_COLOR,
            activebackground=FRAME_COLOR,
            activeforeground=TEXT_COLOR,
            state="disabled",
            command=self._toggle_save)

        # Placement des widgets et création du gestionnaire de graphiques
        self.conn_gui_open()
        self.chart_master = DisGUI(self.root, self.serial, self.data)

        # Création automatique du premier graphique après 500 ms
        self.root.after(500, self._delayed_new_chart)

        self.logger = setup_logger("ConnGUI")

    # ------------------------------------------------------------------
    # Ouverture / fermeture du panneau
    # ------------------------------------------------------------------

    def conn_gui_open(self):
        """Place tous les widgets du panneau Connection Manager dans la grille."""
        self.root.geometry(f"{WINDOW_W_SINGLE}x{WINDOW_H_SINGLE}")
        self.frame.grid(row=0, column=4, rowspan=3, columnspan=5, padx=5, pady=5)

        self.sync_label.grid(column=1,  row=1)
        self.sync_status.grid(column=2, row=1)
        self.ch_label.grid(column=1,    row=2)
        self.ch_status.grid(column=2,   row=2, pady=self.pady)

        self.btn_start_stream.grid(column=3, row=1, padx=self.padx)
        self.btn_stop_stream.grid(column=3,  row=2, padx=self.padx)
        self.btn_add_chart.grid(column=4,    row=1, padx=self.padx)
        self.btn_kill_chart.grid(column=5,   row=1, padx=self.padx)
        self.save_check.grid(column=4,       row=2, columnspan=2)

    def conn_gui_close(self):
        """
        Ferme le panneau Connection Manager :
            - Détruit tous les widgets enfants de la frame
            - Détruit la frame elle-même
            - Supprime tous les graphiques
            - Redimensionne la fenêtre principale
        """
        for widget in self.frame.winfo_children():
            widget.destroy()
        self.frame.destroy()
        self.kill_all_charts()
        self.root.geometry("1000x120")

    # ------------------------------------------------------------------
    # Contrôle du flux de données
    # ------------------------------------------------------------------

    def start_stream(self):
        """
        Démarre la réception du flux de données dans un thread dédié.
        Désactive le bouton Start et active le bouton Stop.
        """
        self.btn_start_stream.config(state="disabled")
        self.btn_stop_stream.config(state="active")

        self.serial.t1 = threading.Thread(
            target=self.serial.serial_data_stream,
            args=(self,),
            daemon=True
        )
        self.serial.t1.start()

    def stop_stream(self):
        """
        Arrête la réception du flux de données.
        Active le bouton Start et désactive le bouton Stop.
        """
        self.btn_start_stream.config(state="active")
        self.btn_stop_stream.config(state="disabled")
        self.serial.threading = False
        self.serial.serial_stop(self)

    # ------------------------------------------------------------------
    # Mise à jour des graphiques
    # ------------------------------------------------------------------

    def update_chart(self):
        """
        Rafraîchit tous les graphiques actifs avec les données courantes.

        Pour chaque graphique (chart_idx) :
            - Efface le subplot
            - Pour chaque canal coché, trace les données via FunctionMaster
            - Redessine le canvas Matplotlib

        Se rappelle toutes les 40 ms via root.after() tant que le flux est actif.
        """
        try:
            for chart_idx in range(len(self.chart_master.view_vars)):

                # Effacement du subplot avant redessin
                self.chart_master.figs[chart_idx][1].clear()

                for ch_cnt, state in enumerate(self.chart_master.view_vars[chart_idx]):
                    if not state.get():
                        continue

                    # Récupération du canal et de la fonction sélectionnés
                    channel   = self.chart_master.option_vars[chart_idx][ch_cnt].get()
                    func_name = self.chart_master.fun_vars[chart_idx][ch_cnt].get()
                    ch_index  = self.data.ChannelNum[channel]

                    # Préparation des données pour le tracé
                    self.chart = self.chart_master.figs[chart_idx][1]  # Axes Matplotlib
                    self.color = self.data.ChannelColor[channel]        # Couleur du canal
                    self.y     = self.data.YDisplay[ch_index]           # Données Y (capteur)
                    self.x     = self.data.XDisplay                     # Données X (temps)

                    # Appel de la fonction d'affichage sélectionnée (raw ou tension)
                    # Exemple : self.data.FunctionMaster["RowData"](self)
                    self.data.FunctionMaster[func_name](self)

                # Affichage de la grille et redessein du canvas
                    self.chart_master.figs[chart_idx][1].grid(
                        color=PLOT_GRID_COLOR, linestyle="--", linewidth=0.3)
                self.chart_master.figs[chart_idx][0].canvas.draw()

        except Exception as e:
            self.logger.error(f"Erreur mise a jour graphique : {e}")

        # Rappel automatique toutes les 40 ms tant que le flux est actif
        if self.serial.threading:
            self.root.after(40, self.update_chart)

    # ------------------------------------------------------------------
    # Gestion des graphiques
    # ------------------------------------------------------------------

    def _delayed_new_chart(self):
        """
        Crée le premier graphique après le délai de démarrage (500 ms).
        Le délai garantit que la synchronisation avec la carte est terminée.
        """
        try:
            self.new_chart()
            self.logger.info("Premier graphique cree automatiquement.")
        except Exception as e:
            self.logger.error(f"Erreur creation automatique graphique : {e}")

    def new_chart(self):
        """Ajoute un nouveau graphique via le gestionnaire DisGUI."""
        self.chart_master.add_channel_master()

    def kill_chart(self):
        """
        Supprime le dernier graphique ajouté et libère toutes les ressources
        associées (frame, figure, canvas, variables de canaux).
        """
        try:
            if not self.chart_master.frames:
                return

            last_idx = len(self.chart_master.frames) - 1

            # Destruction du widget visuel principal
            self.chart_master.frames[last_idx].destroy()

            # Suppression des références dans les listes parallèles
            self.chart_master.frames.pop()
            self.chart_master.figs.pop()
            self.chart_master.control_frames.pop()

            # Destruction du panneau de sélection des canaux
            self.chart_master.channel_frames[last_idx][0].destroy()
            self.chart_master.channel_frames.pop()

            # Suppression des variables Tkinter associées
            self.chart_master.view_vars.pop()
            self.chart_master.option_vars.pop()
            self.chart_master.fun_vars.pop()

            # Ajustement de la taille de la fenêtre principale
            self.chart_master.adjust_root_frame()

        except Exception as e:
            self.logger.error(f"Impossible de supprimer le graphique : {e}")

        try:
            self.chart_master.update_master_frame()
        except Exception as e:
            self.logger.error(f"Impossible de mettre a jour le frame principal : {e}")

    def kill_all_charts(self):
        """Supprime tous les graphiques existants un par un."""
        try:
            while self.chart_master.frames:
                self.kill_chart()
            self.logger.info("Tous les graphiques detruits.")
        except Exception as e:
            self.logger.error(f"Erreur suppression graphiques : {e}")

    # ------------------------------------------------------------------
    # Sauvegarde CSV
    # ------------------------------------------------------------------

    def _toggle_save(self):
        """Active ou désactive la sauvegarde CSV selon l'état de la case à cocher."""
        self.save = not self.save


# ==============================================================================
# DisGUI — Gestionnaire des graphiques Matplotlib
# ==============================================================================

class DisGUI():
    """
    Gestionnaire dynamique des graphiques Matplotlib intégrés dans Tkinter.

    Permet d'ajouter et supprimer des graphiques à la volée.
    Chaque graphique contient :
        - Un subplot Matplotlib (Figure + Axes + Canvas)
        - Un panneau de boutons +/- pour ajouter ou retirer des canaux
        - Des menus déroulants de sélection de canal et de fonction d'affichage

    Structure de self.figs (liste de listes) :
        self.figs = [
            [Figure, Axes, FigureCanvasTkAgg],   # graphique 0
            [Figure, Axes, FigureCanvasTkAgg],   # graphique 1
            ...
        ]

    Structure de self.channel_frames (liste de listes) :
        self.channel_frames = [
            [LabelFrame_widget, frame_index],    # graphique 0
            [LabelFrame_widget, frame_index],    # graphique 1
            ...
        ]
    """

    # Dimensions des boutons +/- (en unites Tkinter)
    BTN_H = 2
    BTN_W = 4

    # Nombre maximum de canaux par graphique
    MAX_CHANNELS_PER_FRAME = 8

    def __init__(self, root, serial, data):
        """
        Initialise le gestionnaire sans créer de graphique.
        Les graphiques sont créés dynamiquement via add_channel_master().

        Args:
            root   : fenêtre Tkinter parente
            serial : instance de Serial_Control
            data   : instance de DataMaster
        """
        self.root   = root
        self.serial = serial
        self.data   = data

        # Listes parallèles — un element par graphique
        self.frames         = []   # LabelFrames principaux
        self.figs           = []   # [Figure, Axes, Canvas] par graphique
        self.control_frames = []   # Panneaux de boutons +/-
        self.channel_frames = []   # [LabelFrame_widget, frame_index] par graphique

        # Variables Tkinter — un sous-tableau par graphique
        self.view_vars   = []   # IntVar  : etat des cases a cocher (afficher/masquer canal)
        self.option_vars = []   # StringVar : canal selectionne dans le menu deroulant
        self.fun_vars    = []   # StringVar : fonction selectionnee (raw ou tension)

        # Position courante dans la grille Tkinter
        self.frames_col   = 0
        self.frames_row   = 4
        self.total_frames = 0

        self.logger = setup_logger("DisGUI")

    # ------------------------------------------------------------------
    # Ajout d'un graphique complet
    # ------------------------------------------------------------------

    def add_channel_master(self):
        """
        Ajoute un nouveau graphique complet :
            1. Frame principal (LabelFrame)
            2. Ajustement de la taille de la fenêtre
            3. Subplot Matplotlib (Figure + Axes + Canvas)
            4. Panneau de sélection des canaux
            5. Boutons +/-
        """
        self._add_master_frame()
        self.adjust_root_frame()
        self._add_graph()
        self._add_channel_frame()
        self._add_btn_frame()

    # ------------------------------------------------------------------
    # Frame principal
    # ------------------------------------------------------------------

    def _add_master_frame(self):
        """
        Crée et place le LabelFrame principal du nouveau graphique.

        Disposition en grille (2 colonnes) :
            - Index 0 (1er graphique) : centré, colonne 2
            - Index 1 (2e graphique)  : repositionne le 1er a gauche (colonne 0)
            - Index pair  : colonne 0 (gauche)
            - Index impair : colonne 8 (droite)
            - Ligne : 4 + 4 * (index // 2)  — une nouvelle ligne toutes les 2 frames
        """
        self.frames.append(LabelFrame(self.root,
            text=f"Display Manager-{len(self.frames) + 1}",
            pady=5, padx=5,
            bg=FRAME_COLOR, fg=TEXT_COLOR))
        self.total_frames = len(self.frames) - 1

        if self.total_frames == 0:
            # Premier graphique : centre
            self.frames_col = 2
            self.frames_row = 4
        else:
            if self.total_frames == 1:
                # Repositionne le premier graphique a gauche
                self.frames[0].grid_forget()
                self.frames[0].grid(padx=5, column=0, row=4, columnspan=4, sticky=NW)

            # Colonne : 0 pour index pair, 8 pour index impair
            self.frames_col = 0 if self.total_frames % 2 == 0 else 8

            # Ligne : incremente toutes les 2 frames
            self.frames_row = 4 + 4 * int(self.total_frames / 2)

        self.frames[self.total_frames].grid(
            padx=5,
            column=self.frames_col,
            row=self.frames_row,
            columnspan=4,
            sticky=NW
        )
        self.frames[self.total_frames].grid_rowconfigure(0, weight=1)
        self.frames[self.total_frames].grid_columnconfigure(1, weight=1)

    # ------------------------------------------------------------------
    # Ajustement de la fenetre principale
    # ------------------------------------------------------------------

    def adjust_root_frame(self):
        """
        Redimensionne la fenêtre principale selon le nombre de graphiques actifs.

        Largeur :
            - 1 graphique  : 1100 px
            - 2+ graphiques : 2200 px

        Hauteur :
            - 120 px de base + 430 px par ligne de graphiques
            - Exemple : 2 graphiques (1 ligne)  -> 120 + 430 * 1 = 550 px
                        4 graphiques (2 lignes) -> 120 + 430 * 2 = 980 px
        """
        self.total_frames = len(self.frames) - 1
        root_w = WINDOW_W_MULTI if self.total_frames > 0 else WINDOW_W_SINGLE
        root_h = WINDOW_H_BASE + WINDOW_H_CHART * (int(self.total_frames / 2) + 1)
        self.root.geometry(f"{root_w}x{root_h}")

    # ------------------------------------------------------------------
    # Subplot Matplotlib
    # ------------------------------------------------------------------

    def _add_graph(self):
        """
        Crée le subplot Matplotlib pour le graphique courant et l'intègre
        dans le LabelFrame via FigureCanvasTkAgg.

        Tailles :
            - 1er graphique : 8x4 pouces, 80 dpi (plus grand car seul)
            - Suivants      : 6x4 pouces, 60 dpi

        Lors de l'ajout du 2e graphique, le 1er est redimensionne en 6x4
        pour uniformiser l'affichage.
        """
        if self.total_frames == 0:
            figsize = (FIG_W_SINGLE, FIG_H_SINGLE)
            dpi     = FIG_DPI
        else:
            figsize = (FIG_W_MULTI, FIG_H_MULTI)
            dpi     = FIG_DPI

        fig = plt.Figure(figsize=figsize, dpi=dpi, facecolor=BG_COLOR)
        axes   = fig.add_subplot(111)
        axes.set_facecolor(PLOT_BG_COLOR)
        axes.tick_params(colors=PLOT_TEXT_COLOR)
        axes.xaxis.label.set_color(PLOT_TEXT_COLOR)
        axes.yaxis.label.set_color(PLOT_TEXT_COLOR)
        for spine in axes.spines.values():
            spine.set_edgecolor(PLOT_TEXT_COLOR)
        canvas = FigureCanvasTkAgg(fig, master=self.frames[self.total_frames])

        self.figs.append([fig, axes, canvas])

        canvas.get_tk_widget().grid(
            column=1, row=0, columnspan=4, rowspan=17, sticky="nsew")

        # Redimensionne le premier graphique lors de l'ajout du second
        if self.total_frames == 1:
            self.figs[0][2].get_tk_widget().destroy()
            fig0    = plt.Figure(figsize=(FIG_W_MULTI, FIG_H_MULTI), dpi=FIG_DPI, facecolor=BG_COLOR)
            axes0   = fig0.add_subplot(111)
            axes0.set_facecolor(PLOT_BG_COLOR)
            axes0.tick_params(colors=PLOT_TEXT_COLOR)
            axes0.xaxis.label.set_color(PLOT_TEXT_COLOR)
            axes0.yaxis.label.set_color(PLOT_TEXT_COLOR)
            for spine in axes0.spines.values():
                spine.set_edgecolor(PLOT_TEXT_COLOR)
            canvas0 = FigureCanvasTkAgg(fig0, master=self.frames[0])
            self.figs[0] = [fig0, axes0, canvas0]
            canvas0.get_tk_widget().grid(
                column=1, row=0, columnspan=4, rowspan=17, sticky=N)

    # ------------------------------------------------------------------
    # Panneau de boutons +/-
    # ------------------------------------------------------------------

    def _add_btn_frame(self):
        """
        Ajoute un panneau contenant les boutons '+' et '-' pour ajouter
        ou retirer un canal sur le graphique courant.

        La commande des boutons utilise functools.partial pour conserver
        la référence au channel_frame correct au moment de la creation.
        """
        self.control_frames.append([])

        btn_frame = LabelFrame(self.frames[self.total_frames],pady=5, bg=FRAME_COLOR)
        btn_frame.grid(column=0, row=0, padx=5, pady=5, sticky=N)
        self.control_frames[self.total_frames].append(btn_frame)

        btn_add = Button(
            btn_frame, text="+",
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            width=self.BTN_W, height=self.BTN_H,
            command=partial(self._add_channel, self.channel_frames[self.total_frames]))
        btn_add.grid(column=0, row=0, padx=5, pady=5)
        self.control_frames[self.total_frames].append(btn_add)

        btn_del = Button(
            btn_frame, text="-",
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR,
            width=self.BTN_W, height=self.BTN_H,
            command=partial(self._delete_channel, self.channel_frames[self.total_frames]))
        btn_del.grid(column=1, row=0, padx=5, pady=5)
        self.control_frames[self.total_frames].append(btn_del)

    # ------------------------------------------------------------------
    # Panneau de selection des canaux
    # ------------------------------------------------------------------

    def _add_channel_frame(self):
        """
        Initialise le panneau contenant les sélecteurs de canaux pour le
        graphique courant, puis ajoute un premier canal par defaut.

        self.channel_frames[i] = [LabelFrame_widget, frame_index]
        """
        self.channel_frames.append([])
        self.view_vars.append([])
        self.option_vars.append([])
        self.fun_vars.append([])

        ch_frame = LabelFrame(self.frames[self.total_frames], pady=5, bg=FRAME_COLOR)
        ch_frame.grid(column=0, row=1, padx=5, pady=5, rowspan=16, sticky=N)

        self.channel_frames[self.total_frames].append(ch_frame)
        self.channel_frames[self.total_frames].append(self.total_frames)

        # Ajout du premier canal par defaut
        self._add_channel(self.channel_frames[self.total_frames])

    def _add_channel(self, channel_frame):
        """
        Ajoute une ligne de sélection de canal dans le panneau des canaux.
        Chaque ligne contient : checkbox + menu canal + menu fonction.
        Limite : MAX_CHANNELS_PER_FRAME canaux par graphique.

        Args:
            channel_frame (list) : [LabelFrame_widget, frame_index]
        """
        frame_widget = channel_frame[0]
        frame_idx    = channel_frame[1]

        if len(frame_widget.winfo_children()) >= self.MAX_CHANNELS_PER_FRAME:
            return

        row_frame = LabelFrame(frame_widget, bg=FRAME_COLOR)
        row_frame.grid(column=0, row=len(frame_widget.winfo_children()) - 1)

        # Case a cocher pour afficher/masquer le canal
        self.view_vars[frame_idx].append(IntVar())
        Checkbutton(row_frame,
            variable=self.view_vars[frame_idx][-1],
            onvalue=1, offvalue=0,
            bg=FRAME_COLOR,
            selectcolor=BTN_COLOR,
            activebackground=FRAME_COLOR).grid(row=0, column=0, padx=1)

        self._add_channel_option(row_frame, frame_idx)
        self._add_channel_func(row_frame, frame_idx)

    def _add_channel_option(self, frame, frame_idx):
        """
        Ajoute le menu déroulant de sélection du canal (Ch0 a Ch7).
        Un trace est posé sur la variable pour mettre a jour les labels
        du graphique a chaque changement de canal.

        Args:
            frame     : LabelFrame parent de la ligne de canal
            frame_idx : index du graphique concerné
        """
        var = StringVar()
        var.set(self.data.Channels[0])
        self.option_vars[frame_idx].append(var)

        # Mise a jour automatique des labels du graphique au changement de canal
        var.trace_add(
            "write",
            lambda *args, v=var, idx=frame_idx: self.update_graph_labels(idx, v.get())
        )

        drop = OptionMenu(frame, var, *self.data.Channels)
        drop.config(width=5,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR)
        drop["menu"].config(bg=BTN_COLOR, fg=BTN_TEXT)
        drop.grid(row=0, column=1, padx=1)

        # Premiere mise a jour immediate des labels
        self.update_graph_labels(frame_idx, var.get())

    def _add_channel_func(self, frame, frame_idx):
        """
        Ajoute le menu déroulant de sélection de la fonction d'affichage.
        Les fonctions disponibles sont définies dans DataMaster.FunctionMaster.

        Args:
            frame     : LabelFrame parent de la ligne de canal
            frame_idx : index du graphique concerné
        """
        var = StringVar()
        functions = list(self.data.FunctionMaster.keys())
        var.set(functions[0])
        self.fun_vars[frame_idx].append(var)

        drop = OptionMenu(frame, var, *functions)
        drop.config(width=5,
            bg=BTN_COLOR, fg=BTN_TEXT,
            activebackground=ACCENT_COLOR, activeforeground=BG_COLOR)
        drop["menu"].config(bg=BTN_COLOR, fg=BTN_TEXT)
        drop.grid(row=0, column=2, padx=1)

    def _delete_channel(self, channel_frame):
        """
        Supprime le dernier canal ajouté dans le panneau de canaux.
        Garde toujours au minimum un canal (protection contre la liste vide).

        Args:
            channel_frame (list) : [LabelFrame_widget, frame_index]
        """
        frame_widget = channel_frame[0]
        frame_idx    = channel_frame[1]

        if len(frame_widget.winfo_children()) > 1:
            frame_widget.winfo_children()[-1].destroy()
            self.view_vars[frame_idx].pop()
            self.option_vars[frame_idx].pop()
            self.fun_vars[frame_idx].pop()

    # ------------------------------------------------------------------
    # Mise a jour du frame principal apres suppression
    # ------------------------------------------------------------------

    def update_master_frame(self):
        """
        Recentre et agrandit le graphique restant s'il est le seul affiché.
        Appelé par ConnGUI.kill_chart() après chaque suppression de graphique.
        """
        if self.total_frames != 0:
            return

        self.frames_col = 2
        self.frames_row = 4

        self.frames[0].grid_forget()
        self.frames[0].grid(
            padx=5, column=self.frames_col, row=self.frames_row,
            columnspan=4, sticky=NW
        )
        self.frames[0].grid_rowconfigure(0, weight=1)
        self.frames[0].grid_columnconfigure(1, weight=1)

        # Recreation de la figure en taille maximale (8x4)
        self.figs[0][2].get_tk_widget().destroy()
        fig = plt.Figure(figsize=(FIG_W_SINGLE, FIG_H_SINGLE), dpi=FIG_DPI, facecolor=BG_COLOR)
        axes = fig.add_subplot(111)
        axes.set_facecolor(PLOT_BG_COLOR)
        axes.tick_params(colors=PLOT_TEXT_COLOR)
        axes.xaxis.label.set_color(PLOT_TEXT_COLOR)
        axes.yaxis.label.set_color(PLOT_TEXT_COLOR)
        for spine in axes.spines.values():
            spine.set_edgecolor(PLOT_TEXT_COLOR)
        canvas = FigureCanvasTkAgg(fig, master=self.frames[0])
        self.figs[0] = [fig, axes, canvas]
        canvas.get_tk_widget().grid(
            column=1, row=0, columnspan=4, rowspan=17, sticky="nsew")

    # ------------------------------------------------------------------
    # Mise a jour des labels du graphique
    # ------------------------------------------------------------------

    def update_graph_labels(self, frame_index, channel):
        """
        Met à jour les labels des axes du graphique selon le canal sélectionné.
            - Axe X : 'Temps (s)'
            - Axe Y : nom du capteur associé au canal (ex. 'Temperature')

        Args:
            frame_index (int) : index du graphique a mettre a jour
            channel     (str) : identifiant du canal selectionne (ex. 'Ch0')
        """
        try:
            ax = self.figs[frame_index][1]
            ax.set_xlabel("Temps (s)", fontsize=12)
            ax.set_ylabel(self.data.ChannelName[channel], fontsize=12)
            handles, labels = ax.get_legend_handles_labels()
            if handles:
                ax.legend()
            self.figs[frame_index][2].draw()
        except Exception as e:
            self.logger.error(f"Impossible de mettre a jour les labels : {e}")


# ==============================================================================
# Point d'entree standalone (test)
# ==============================================================================

if __name__ == "__main__":
    # Ces appels sont conserves pour compatibilite avec le code original.
    # Le point d'entree recommande reste Master.py qui fournit
    # les instances serial et data necessaires a chaque classe.
    RootGUI()
    ComGui()
    ConnGUI()


