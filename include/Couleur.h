#ifndef COULEUR_H
#define COULEUR_H


class Couleur {
	
	public:
		Couleur(int r, int v, int b): rouge(r), vert(v), bleu(b) { }
		~Couleur() { }
		
		void getRVB(int &r, int &v, int &b) {
			r = rouge;
			v = vert;
			b = bleu;
		}
		
		void setRVB(int r, int v, int b) {
			rouge = r;
			vert = v;
			bleu = b;
		}
		
		int getRouge();
		int getVert();
		int getBleu();
	
	private:
		int rouge, vert, bleu;
	
};


inline int Couleur::getRouge() { return rouge; }
inline int Couleur::getVert() { return vert; }
inline int Couleur::getBleu() { return bleu; }


#endif
