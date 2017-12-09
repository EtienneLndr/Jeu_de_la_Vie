#ifndef ANT_H
#define ANT_H
#include "Forme.h"
#include "Case.h"


class Ant : public Forme {
	
	public:
		void dessine(sf::RenderWindow *win);
		void moveAnt(Case* _case);
		void move(int _direction);
		void move(string direction);
		
		Ant(int _x, int _y, int l, int h, int r, int v, int b, int _direction, int _step, int widthBoard, int heightBoard);
		~Ant() { }
		
	protected:
		int direction;
		int step;
		int widthOfBoard;
		int heightOfBoard;
	
	};


#endif
