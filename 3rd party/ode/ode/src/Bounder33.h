#ifndef BOUNDER33_H
class Lcp33
{
static	const	int			MSIZE=3;
static	const	int			RLENGTH=4;
				int			NBn																		;
				int			savedNbn																;
				bool		unresolved																;
				dReal		A[MSIZE*RLENGTH]														;
				dReal		*sourceA																;
				dReal		*hi																		;
				dReal		*lo																		;
				dReal		*b																		;
				dReal		*x																		;
				dReal		w[3]																	;
				dReal		bounds[3]																;
				dReal		tw[3]																	;
				int			state[3]																;
				int			skip																	;

				int			index[3]																;
	inline		dReal*		RowA			(int i)								{ return A+i*skip;}
	inline		dReal*		RowI			(int i)								{ return RowA(index[i]);}
	inline		dReal&		EinRowI			(dReal* row,int i)					{ return row[index[i]];}
	inline		dReal&		EinRowA			(dReal* row,int i)					{ return row[i];}
	inline		dReal&		EI				(int row,int pos)					{ return EinRowI(RowI(row),pos);}
	inline		dReal&		EA				(int row,int pos)					{ return A[skip*row+pos];}
	inline		dReal&		EsA				(int row,int pos)					{ return sourceA[RLENGTH*row+pos];}
				void		Swap			(int i, int j)											;
				void		ToBn			(int i)													;
				bool 		BoundX			(int i)													;
				bool 		BoundIndex		(int i)													;
				bool		CheckX			(int i)													;
				bool		CheckIndex		(int i)													;
				
				bool		CheckState		(int i)													;
				void		ToNBn			()														;
				void		ToNBn			(int i)													;
				void		SolveW			()														;
				void		UpdateX			()														;
				void		BoundUnBn		()														;
				bool		CheckUnBn		()														;
				void		BoundAll		()														;
				void		FillA			()														;
				void		FillX			()														;
				bool		SolveForLines	()														;
				bool		SolveForPlanes	()														;

public:
							Lcp33			(dReal* aA,int askip,dReal* ahi, dReal* alo,dReal* ab,dReal* ax)	;
				void		Solve			()														;
};


#endif