#include <iostream>
#include "Pong.h"
#include "save_datas.h"


int main(void) {
	srand((unsigned int)time(NULL));
	
	// Création du plateau
  	Pong pong("Jeu de la Vie", 600, 600, 300);
  	
  	// Lancement de l'exécution du programme
	int time_max = pong.execute();
	  
	cout << "Temps d'exécution maximale: " << time_max << endl;

	// Enregistrement du temps maximal d'exécution dans un fichier
	writeInFile("Temps d'exécution maximale: " + to_string(time_max));
	
	return 0;
}
