#ifndef PONG_H
#define PONG_H
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "SFML/Graphics.hpp"

using namespace std;
using namespace chrono;

class Pong;

//==============================================================//
//						PARTIE THREADS                          //
//==============================================================//

struct ThreadData {
	thread th;
	bool can_be_used = false;
};

//---- some data common to every thread ----
struct SharedData {
	vector<ThreadData> td;
	condition_variable_any cnd;
	mutex mtx;
	int actual_action;
	bool stop_thread = false;
	int thread_count;
};

void drawCases(int i, int indice, int step, SharedData &sd, Pong * pong);

//==============================================================//
//						 PARTIE OBJET                           //
//==============================================================//

class Case;

class Pong {
	
	public: 
		int execute();
		
		Pong(string n = "Window", int w = 400, int h = 400, int step = 1);
		~Pong();
		
		Case* getCase(int i);
		void addCase(Case* c);
		sf::RenderWindow * getWin();
		void setWin(sf::RenderWindow * win);
		
	private:
		string name;
    	int width, height;
    	int step;
    	sf::RenderWindow *_win;
		vector<Case*> cases;
		
		int limite;
		int etape;
		int positionOfCase;
		bool pause;
		
		void drawAll(sf::RenderWindow * win);
		void clicSouris(int x, int y);
		int executeTraitements(SharedData &sd);
	
	};

inline Case* Pong::getCase(int i) { return cases[i]; }
inline void Pong::addCase(Case* c) { cases.push_back(c); }
inline 	sf::RenderWindow * Pong::getWin() { return _win; }
inline 	void Pong::setWin(sf::RenderWindow * win) { _win = win; }

#endif
