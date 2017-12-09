#ifndef PONG_H
#define PONG_H
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include "SFML/Graphics.hpp"
#include "Ant.h"

using namespace std;

class Case;

class Pong {
	
	public: 
		void execute();
		
		Pong(string n="Window", int w=400, int h=400, int step=1);
		~Pong();
		
		Ant* getAnt();
		void setAnt(Ant * a);
		Case* getCase(int i);
		void addCase(Case* c);
		
	private:
		string _name;
    	int _width, _height;
    	int step;
    	sf::RenderWindow *_win;
		Ant* ant;
		vector<Case*> _case;
		
		int limite;
		bool pause=false;
		int dt=1;
		int etape;
		int positionOfCase;
		
		void move();
		void initBoard(sf::RenderWindow * win);
		void drawAll(sf::RenderWindow * win); 
		void clicSouris(int x, int y); 
	
	};
		
		
inline Ant* Pong::getAnt() { return ant; }
inline void Pong::setAnt(Ant * a) { ant = a; }
inline Case* Pong::getCase(int i) { return _case[i]; }
inline void Pong::addCase(Case* c) { _case.push_back(c); }


#endif
