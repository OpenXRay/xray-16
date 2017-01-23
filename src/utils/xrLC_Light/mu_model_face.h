#ifndef _MU_MODEL_FACE_
#define _MU_MODEL_FACE_



class  INetReader;

#include "base_face.h"
#include "meshstructure.h"
#include "mu_model_face_defs.h"
//#ifndef MESHSTRUCTURE_EXSPORTS_IMPORTS
//#define MESHSTRUCTURE_EXSPORTS_IMPORTS
//#endif





struct	XRLC_LIGHT_API data_face	: public base_Face
{
public:
	//_vertex*	v	[3];
	Fvector2	tc	[3];
	Fvector		N;
	u32			sm_group;
public:
	virtual Fvector2*	getTC0				( ) { return tc; };

	//bool				VContains			( _vertex* pV);					// Does the face contains this vertex?
	//void				VReplace			( _vertex* what, _vertex* to);	// Replace ONE vertex by ANOTHER
	//void				VReplace_NoRemove	( _vertex* what, _vertex* to);
	//int					VIndex				( _vertex* pV);
	//void				VSet				( int idx, _vertex* V);
	//void				VSet				( _vertex *V1, _vertex *V2, _vertex *V3);
	//BOOL				isDegenerated		( );
	//BOOL				isEqual				( _face& F );
	//float				EdgeLen				( int edge);
	//void				EdgeVerts			( int e, _vertex** A, _vertex** B);
	//void				CalcNormal			( );
	//void				CalcNormal2			( );
	//float				CalcArea			( )const ;
	//float				CalcMaxEdge			( );
	//void				CalcCenter			( Fvector &C );

	//BOOL				RenderEqualTo		( Face *F );

	data_face()				{ sm_group = 0;};
	virtual ~data_face()	{ };
};


struct	XRLC_LIGHT_API data_vertex	: public base_Vertex
{
	//v_faces		adjacent;
	typedef		data_face			DataFaceType;
public:
	//		void		prep_add			(_face* F);
	//		void		prep_remove			(_face* F);
	//		void		calc_normal_adjacent();

	data_vertex()			{ };
	virtual ~data_vertex()	{ };
};




#endif