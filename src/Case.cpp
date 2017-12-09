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
  	sf::RectangleShape shape(sf::Vector2f(largeur, hauteur));
  	shape.setFillColor(sf::Color(rouge, vert, bleu));
  	shape.setPosition(x,y);
	win->draw(shape);
}



