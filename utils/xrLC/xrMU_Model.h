#pragma once

#undef MESHSTRUCTURE_EXSPORTS_IMPORTS
#include "../xrlc_light/base_face.h"
#include "../xrlc_light/meshstructure.h"

#include "../../xrcore/xrPool.h"

//#include "cl_collector.h"
namespace	CDB
{
	class	MODEL;
	class	CollectorPacked;
};
struct OGF;
class base_lighting;
class xrMU_Model
{
public:
	//** 
	//struct	_vertex;
	//struct	_face;
	//struct	data_face;
	//struct data_vertex;
	//**
	struct	data_face	: public ::base_Face
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

	//** 
	struct	data_vertex	: public ::base_Vertex
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

	typedef	Tvertex<data_vertex>	_vertex;
	typedef	Tface<data_vertex>		_face;

	struct	_subdiv
	{
		u32		material;
		u32		start;
		u32		count;

		OGF*	ogf;

		u32		vb_id;
		u32		vb_start;

		u32		ib_id;
		u32		ib_start;

		u32		sw_id;
	};

	typedef xr_vector<_vertex>::iterator dummy_compiler_treatment;

	//** 
	typedef	xr_vector<_vertex*>		v_vertices;
	typedef	v_vertices::iterator	v_vertices_it;

	typedef xr_vector<_face*>		v_faces;
	typedef v_faces::iterator		v_faces_it;
	typedef xr_vector<_subdiv>		v_subdivs;
	typedef v_subdivs::iterator		v_subdivs_it;

public:
	shared_str				m_name;
	u16						m_lod_ID;
	v_vertices				m_vertices;
	v_faces					m_faces;
	v_subdivs				m_subdivs;

	xr_vector<base_color>	color;
private:
	_face*					create_face			(_vertex* v0, _vertex* v1, _vertex* v2, b_face& F);
	_vertex*				create_vertex		(Fvector& P);
	_face*					load_create_face	(Fvector& P1, Fvector& P2, Fvector& P3, b_face& F);
	_vertex*				load_create_vertex	(Fvector& P);
public:
	void					Load				(IReader& fs);
	void					calc_normals		();
	void					calc_materials		();
	void					calc_faceopacity	();
	void					calc_lighting		(xr_vector<base_color>& dest, Fmatrix& xform, CDB::MODEL* M, base_lighting& lights, u32 flags);
	void					calc_lighting		();
	void					calc_ogf			();
	void					export_geometry		();
	void					export_cform_rcast	(CDB::CollectorPacked& CL, Fmatrix& xform);
};

class xrMU_Reference
{
public:
	xrMU_Model*				model;
    Fmatrix					xform;
    Flags32					flags;
	u16						sector;

	xr_vector<base_color>	color;

	base_color_c			c_scale;
	base_color_c			c_bias;
public:
	void					Load				(IReader& fs);
	void					calc_lighting		();

	void					export_cform_game	(CDB::CollectorPacked& CL);
	void					export_cform_rcast	(CDB::CollectorPacked& CL);
	void					export_ogf			();
};

extern	poolSS<xrMU_Model::_vertex,8*1024>	mu_vertices;
extern	poolSS<xrMU_Model::_face,8*1024>	mu_faces;
