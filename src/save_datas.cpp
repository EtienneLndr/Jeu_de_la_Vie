#include "save_datas.h"


void writeInFile(string data) {
    ofstream fout("time_saved.txt", ios::app);
	
	if(fout){
		// Ecriture de l'en-tête
		fout << data << endl;
		
		// Fermeture du fichier
		fout.close();
	} else{
		cout << "Impossible d'ouvrir le fichier " << "time_saved.txt" << endl;
    }
}