#include "Ant.h"


Ant::Ant(int _x, int _y, int l, int h, int r, int v, int b, int _direction, int _step, int widthBoard, int heightBoard): Forme(_x, _y, l, h, Couleur(r, v, b)), direction(_direction), step(_step), widthOfBoard(widthBoard), heightOfBoard(heightBoard) {
	x = _x + largeur/5;
	y = _y + hauteur/5;
	largeur -= 2*largeur/5;
	hauteur -= 2*hauteur/5;
}


void Ant::dessine(sf::RenderWindow *win) {
	int red, green, blue;
  	couleur.getRVB(red, green, blue);
  	sf::RectangleShape shape(sf::Vector2f(largeur, hauteur));
  	shape.setFillColor(sf::Color(red, green, blue));
  	shape.setPosition(x+largeur/5,y+hauteur/5);
	win->draw(shape);
}


void Ant::moveAnt(Case * _case) {
	if (_case->getStrCouleur() == "noir") {
		_case->changeColor();
		move(-1);
	} else if (_case->getStrCouleur() == "blanc") {
		_case->changeColor();
		move(1);
	}
}


void Ant::move(int _direction) {
	direction = (direction + _direction)%4;
	
	// Permet de prévenir un résultat négatif
	if (direction<0) direction = direction+4;
	
	switch (direction) {
		case 0:
			if (y - step < 0) y = heightOfBoard-step;
			else y -= step;
			break;
		case 1:
			if (x + step >= widthOfBoard) x = 0;
			else x += step;
			break;
		case 2:
			if (y + step >= heightOfBoard) y = 0;
			else y += step;
			break;
		case 3:
			if (x - step < 0) x = widthOfBoard - step;
			else x -= step;
			break;
		default:
			cout << "Erreur pour direction = " << direction << endl;
			break;
	}
}

void Ant::move(string direction) {
	if (direction == "haut") {
		if (y - step < 0) y = heightOfBoard-step;
		else y -= step;
	} else if (direction == "droite") {
		if (x + step >= widthOfBoard) x = 0;
		else x += step;
	} else if (direction == "bas") {
		if (y + step >= heightOfBoard) y = 0;
		else y += step;
	} else if (direction == "gauche") {
		if (x - step < 0) x = widthOfBoard - step;
		else x -= step;
	}
}


















