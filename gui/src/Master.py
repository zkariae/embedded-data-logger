"""
Master.py
---------
Point d'entrée de l'application embedded-data-logger.

Rôle :
    - Instancie les objets principaux (communication série, données, GUI)
    - Lance la boucle principale Tkinter

Auteur  : z_benakka193
Projet  : embedded-data-logger
"""

from GUI_Master import RootGUI, ComGui
from Serial_Com_Control import Serial_Control
from Data_communication_Control import DataMaster


def main():
    """Initialise et lance l'application."""
    serial = Serial_Control()
    data = DataMaster()

    root_gui = RootGUI(serial, data)
    ComGui(root_gui.root, serial, data)

    root_gui.root.mainloop()


if __name__ == "__main__":
    main()
