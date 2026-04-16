"""
Master.py
---------
Point d'entree de l'application embedded-data-logger.

Role :
    - Instancie les objets principaux (communication serie, donnees, GUI)
    - Lance la boucle principale Tkinter
    - ComGui est cree depuis RootGUI apres login et clic sur Serial

Auteur  : z_benakka193
Projet  : embedded-data-logger
"""

from GUI_Master import RootGUI
from Serial_Com_Control import Serial_Control
from Data_communication_Control import DataMaster


def main():
    """Initialise et lance l'application."""
    serial = Serial_Control()
    data   = DataMaster()

    root_gui = RootGUI(serial, data)
    root_gui.root.mainloop()


if __name__ == "__main__":
    main()
