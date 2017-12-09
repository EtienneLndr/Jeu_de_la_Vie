#ifndef FENETRE_H
#define FENETRE_H
#include "SFML/Graphics.hpp"
#include <unistd.h>
#include <iostream>
#include <vector>
#include "Couleur.h"


using namespace std;


class Fenetre {
	
	public:
		Fenetre(int _largeur, int _hauteur, string name);
		virtual ~Fenetre() { }
		
		virtual int creationFenetre()=0;
		virtual void drawWindow()=0;
		virtual void cliqueSouris(int x, int y)=0;
		
	protected:
		int largeur, hauteur;
		sf::RenderWindow *window;
		vector<sf::RectangleShape> rects;
		int verif=0;
		int end=0;
	
	};




#endif
