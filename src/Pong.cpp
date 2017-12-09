#include "Pong.h"
#include "Case.h"


using namespace std;


Pong::Pong(string n, int w, int h, int _step) : _name(n), _width(w), _height(h), step(_step) {
	// Création de la fenêtre de jeu
	_win = new sf::RenderWindow(sf::VideoMode(_width, _height), _name);
	
	for(int i=0; i<step; i++) {
		for(int j=0; j<step; j++) {
			addCase(new Case(_width/step*j, _height/step*i, _width/step, _height/step, 255, 255, 255));
			}
		}
	
	ant = new Ant(
		_width/step*(rand()%step),
		_width/step*(rand()%step),
		_width/step,
		_height/step,
		25 + rand()%200,
		25 + rand()%200,
		25 + rand()%200,
		rand()%4,
		_width/step,
		_width,
		_height);

	}


Pong::~Pong(void) {
  	// Destruction des objets graphiques
  	delete _win;
  	for (unsigned int i=0; i<_case.size(); i++) {
  		delete _case[i];
  		}
  	delete ant;
	}


void Pong::clicSouris(int x, int y) {
	cout << "x = " << x << ", y = " << y << endl;
	positionOfCase = x/(_width/step) + y/(_height/step)*step;
	_case[positionOfCase]->changeColor();
	}


void Pong::move() {
	positionOfCase = ant->getX()/(_width/step) + ant->getY()/(_height/step)*step;
	ant->moveAnt(_case[positionOfCase]);
	}


void Pong::drawAll(sf::RenderWindow * win) {
	// On dessine tous les objets graphiques en appelant leur fonction de dessin
	_case[positionOfCase]->dessine(win);
	ant->dessine(win);
	}


void Pong::initBoard(sf::RenderWindow * win) {
	// On dessine tous les objets graphiques en appelant leur fonction de dessin
	for (unsigned int i=0; i<_case.size(); i++) {
		_case[i]->dessine(win);
		}
	
	ant->dessine(win);
	}


void Pong::execute(void) { 
	/**
	 * Cette fonction est l'élément clé de notre programme. En effet, une boucle while permet
	 * de lancer indéfiniment notre programme jusqu'à ce que l'utilisateur décide de mettre
	 * fin à la partie en fermant la fenêtre de jeu. Dans cette boucle while on fait appel
	 * à différentes fonctions permettant de mettre en place notre jeu. 
	 */
	_win->clear(sf::Color(240, 240, 240));
	
	initBoard(_win);
	
  	while (_win->isOpen()) {
    	//_win->clear(sf::Color(240, 240, 240));
    
    	// Dessin des objets graphiques
		drawAll(_win);
		
		// On déplace la fourmi
		if (!pause) {
			move();
			etape++;
			if (etape%1000 == 0) cout << "etape = " << etape << endl;
			}
		
		_win->display();
		
    	sf::Event event;
    	if(_win->pollEvent(event)) {
      		switch (event.type) {
      			case sf::Event::Closed:
             		_win->close();
             		break;
             	// Si une touche du clavier est pressée:
        		case sf::Event::KeyPressed:
        			// Si c'est la touche 'echap' on met le jeu en pause
        			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        				// On met le jeu en pause --> plus aucun mouvement
        				if (!pause) pause = true;
        				else pause = false;
        			} else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
        				ant->move("haut");
        			} else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
        				ant->move("droite");
        			} else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        				ant->move("bas");
        			} else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
        				ant->move("gauche");
        			}
             		break;
             	// Si on clique avec la souris:
             	case sf::Event::MouseButtonPressed:
    				// Si c'est un clic gauche:
    				if (event.mouseButton.button == sf::Mouse::Left) {
        				/**
        				 * On teste si le clic est sur une forme à l'aide de la méthode 
        				 * cliqueSouris(int, int)
        				 */
        				clicSouris(event.mouseButton.x, event.mouseButton.y);
    					}
    				break;
        		default:
         			break;
      			}
    		}
    	/**
    	 * On met le programme en pause pendant une petite durée car sinon celui-ci est trop rapide et
		 * il plante.
    	 */
    	usleep(500);
  		}
	}






















