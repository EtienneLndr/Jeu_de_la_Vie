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
		
		void setCouleur(Couleur c);
		Couleur getCouleur(void);
		void setX(int _x);
		int getX();
		void setY(int _y);
		int getY();
		void setLargeur(int l);
		int getLargeur();
		void setHauteur(int h);
		int getHauteur();
		
		virtual void dessine(sf::RenderWindow *win) { (void)win; }
	
	protected:
		int x, y, largeur, hauteur;
		Couleur couleur;
		double orientation;
	
	};


inline void Forme::setCouleur(Couleur c) { couleur = c; }
inline Couleur Forme::getCouleur(void) { return couleur; }
inline void Forme::setX(int x) { this->x = x; }
inline int Forme::getX() { return x; }
inline void Forme::setY(int y) { this->y = y; }
inline int Forme::getY() { return y; }
inline void Forme::setLargeur(int l) { largeur = l; }
inline int Forme::getLargeur() { return largeur; }
inline void Forme::setHauteur(int h) { hauteur = h; }
inline int Forme::getHauteur() { return hauteur; }


#endif
