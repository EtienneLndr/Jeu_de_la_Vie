#include <iostream>
#include "Pong.h"
#include "save_datas.h"


int main(void) {
	srand(time(NULL));
	
	// Création du plateau
  	Pong pong("Jeu de la Vie", 600, 600, 300);
  	
  	// Lancement de l'exécution du programme
	int timeMax = pong.execute();
	  
	cout << "Temps d'exécution maximale: " << timeMax << endl;

	// Enregistrement du temps maximal d'exécution dans un fichier
	writeInFile("Temps d'exécution maximale: " + to_string(timeMax));
	
	return 0;
}
