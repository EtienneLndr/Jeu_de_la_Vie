#include "Fenetre.h"


Fenetre::Fenetre(int _largeur, int _hauteur, string name): largeur(_largeur), hauteur(_hauteur), window(new sf::RenderWindow(sf::VideoMode(_largeur, _hauteur), name, sf::Style::Default)) { }
