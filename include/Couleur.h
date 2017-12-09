#ifndef COULEUR_H
#define COULEUR_H


class Couleur {
	
	public:
		Couleur(int r, int v, int b): rouge(r), vert(v), bleu(b) { }
		~Couleur() { }
		
		void getRVB(int &r, int &v, int &b) {
			r=rouge;
			v=vert;
			b=bleu;
			}
		
		void setRVB(int r, int v, int b) {
			rouge=r;
			vert=v;
			bleu=b;
			}
		
		int getRouge() {
			return rouge;
			}
		
		int getVert() {
			return vert;
			}
		
		int getBleu() {
			return bleu;
			}
	
	private:
		int rouge, vert, bleu;
	
	
	};


#endif
