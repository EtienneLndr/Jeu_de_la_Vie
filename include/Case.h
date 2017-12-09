#ifndef CASE_H
#define CASE_H
#include "Forme.h"


class Case : public Forme {
	
	public:
		Case(int _x, int _y, int l, int h, int r, int v, int b);
		~Case() { }
		
		void dessine(sf::RenderWindow *win);
		void changeColor();
		string getStrCouleur();
		void setStrCouleur(string _strCouleur);
		
	private:
		double coefficient;
		string strCouleur;
	
	};

inline string Case::getStrCouleur() {
	return strCouleur;
}

inline void Case::setStrCouleur(string _strCouleur) {
	strCouleur = _strCouleur;
}


#endif
