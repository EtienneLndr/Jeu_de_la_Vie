#include "Case.h"
#include "Couleur.h"
#include "Forme.h"


Case::Case(int _x, int _y, int l, int h, int r, int v, int b): Forme(_x, _y, l, h, Couleur(r, v, b)) {
	switch(couleur.getRouge()) {
		case 255:
			strCouleur = "blanc";
			break;
		case 0:
			strCouleur = "noir";
			break;
		default:
			break;
	}
}


void Case::changeColor() {
	if (strCouleur == "noir") {
		strCouleur = "blanc";
		couleur.setRVB(255, 255, 255);
	} else if (strCouleur == "blanc") {
		strCouleur = "noir";
		couleur.setRVB(0, 0, 0);
	}
}


void Case::dessine(sf::RenderWindow *win) { 
	int rouge, vert, bleu;
  	couleur.getRVB(rouge, vert, bleu);
  	sf::RectangleShape shape(sf::Vector2f((float)largeur, (float)hauteur));
  	shape.setFillColor(sf::Color((unsigned char)rouge, (unsigned char)vert, (unsigned char)bleu));
  	shape.setPosition((float)x, (float)y);
	win->draw(shape);
}



