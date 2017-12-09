#include "Pong.h"
#include "Case.h"


//==============================================================//
//						PARTIE THREADS                          //
//==============================================================//
void drawCases(int i, int indice, int step, SharedData &sd, Pong * pong) {
	(void)pong;
	
	while (!sd.stop_thread) {
		cout << "debut du numéro " << i << ": " << indice + (step*step)/sd.thread_count << endl;
		
		for (int j = indice; j < indice + (step*step)/sd.thread_count; j++) {
			//cout << "j: " << j << endl;
			//pong->getCase(j)->dessine(pong->getWin());
		}

		// Attente de la fin du traitement de chaque case
		sd.mtx.lock();

		sd.actual_action++;
		while (sd.actual_action != 0) {
			sd.cnd.wait(sd.mtx);
		}

		sd.mtx.unlock();
	}
}

//==============================================================//
//						 PARTIE OBJET                           //
//==============================================================//
Pong::Pong(string n, int w, int h, int _step) : name(n), width(w), height(h), step(_step) {
	// Création de la fenêtre de jeu
	_win = new sf::RenderWindow(sf::VideoMode(width, height), name);
	
	// Création de toutes les cases
	for(int i = 0; i < step; i++) {
		for(int j = 0; j < step; j++) {
			addCase(new Case(width/step*j, height/step*i, width/step, height/step, 255, 255, 255));
		}
	}

	// Initialisation du paramètre permettant de mettre le programme en pause
	pause = false;
}

Pong::~Pong(void) {
  	// Destruction des objets graphiques
  	delete _win;
  	for (unsigned int i = 0; i < cases.size(); i++) {
  		delete cases[i];
  	}
}

void Pong::clicSouris(int x, int y) {
	cout << "x = " << x << ", y = " << y << endl;
	positionOfCase = x/(width/step) + y/(height/step)*step;
	cases[positionOfCase]->changeColor();
}

void Pong::drawAll(sf::RenderWindow * win) {
	// On dessine tous les objets graphiques en appelant leur fonction de dessin
	for (unsigned int i = 0; i < cases.size(); i++) {
		cases[i]->dessine(win);
	}
}

int Pong::execute(void) {
	int nbrCpu = crs::len(crs::detectCpuInfo(false));

	cout << nbrCpu << endl;

	// Lancement des threads
	SharedData sd;
	sd.td.resize(nbrCpu + 1);
	sd.thread_count = nbrCpu;
	
	for (int i = 0; i < nbrCpu; i++) {
		sd.td[i].th = thread(drawCases, i, ((step*step)/nbrCpu) * i, step, ref(sd), this);
	}

	sd.td[nbrCpu].th = thread(&Pong::executeTraitements, this, ref(sd));

	for (unsigned int i = 0; i < sd.td.size(); i++) {
	  	sd.td[i].th.join();
	}

	return 0;
}

int Pong::executeTraitements(SharedData &sd) {
	/**
	 * Cette fonction est l'élément clé de notre programme. En effet, une boucle while permet
	 * de lancer indéfiniment notre programme jusqu'à ce que l'utilisateur décide de mettre
	 * fin à la partie en fermant la fenêtre de jeu. Dans cette boucle while nous faisons appel
	 * à différentes fonctions permettant de mettre en place notre jeu. 
	 */
	_win->clear(sf::Color(240, 240, 240));

	int timeMax = 0;

  	while (_win->isOpen()) {
		high_resolution_clock::time_point t1 = high_resolution_clock::now();

    	_win->clear(sf::Color(240, 240, 240));
		
		if (!pause) {
			_win->display();
			
			if (sd.actual_action != 2);
			else {
				sd.actual_action = 0;
				sd.cnd.notify_all();
			}

			//if (etape%1000 == 0) cout << "etape = " << etape << endl;
			etape++;
		}
		
    	sf::Event event;
    	if(_win->pollEvent(event)) {
      		switch (event.type) {
      			case sf::Event::Closed:
					// Arrêt des threads permettant de traiter les cases
					sd.mtx.lock();

				  	sd.stop_thread = !sd.stop_thread;

					sd.mtx.unlock();

					// Fermeture de la fenêtre graphique
             		_win->close();
             		break;
             	// Si une touche du clavier est pressée:
        		case sf::Event::KeyPressed:
        			// Si c'est la touche 'echap' on met le jeu en pause
        			if(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        				// On met le jeu en pause --> plus aucun mouvement
        				if (!pause) pause = true;
        				else pause = false;
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
		
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		
		// Calcul de l'intervalle de temps
		auto duration = (int)duration_cast<microseconds>( t2 - t1 ).count();
		
		if (duration > timeMax) {
			timeMax = duration;
			cout << "Maximum time -> " << timeMax << endl;
		}

    	/**
    	 * On met le programme en pause pendant une petite durée car sinon celui-ci est trop rapide et
		 * il plante.
    	 */
    	usleep(500);
	}
	  
	return timeMax;
}






















