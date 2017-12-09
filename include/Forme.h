#ifndef FORME_H
#define FORME_H
#include <iostream>
#include "Couleur.h"
#include "SFML/Graphics.hpp"


using namespace std;
class Couleur;


class Forme {
	public:
		Forme(int _x, int _y, int l, int h, Couleur c): x(_x), y(_y), largeur(l), hauteur(h), couleur(c) { }
		virtual ~Forme() {}
		
		void setCouleur(Couleur c) {
			couleur=c;
			}
		Couleur getCouleur(void) {
			return couleur;
			}
		void setX(int _x) { x=_x; }
		int getX() { return x; }
		void setY(int _y) { y=_y; }
		int getY() { return y; }
		void setLargeur(int l) { largeur=l; }
		int getLargeur() { return largeur; }
		void setHauteur(int h) { hauteur=h; }
		int getHauteur() { return hauteur; }
		
		virtual void dessine(sf::RenderWindow *win) { (void)win; }
	
	protected:
		int x, y, largeur, hauteur;
		Couleur couleur;
		double orientation;
	
	};


#endif
