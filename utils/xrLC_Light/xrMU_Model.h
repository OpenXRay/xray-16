#ifndef XRMU_MODEL_H
#define XRMU_MODEL_H

#include "mu_model_face.h"
//#include "mu_model_face_defs.h"
#include "serialize.h"


//#include "cl_collector.h"
namespace	CDB
{
	class	MODEL;
	class	CollectorPacked;
};
struct OGF;
class base_lighting;
class XRLC_LIGHT_API xrMU_Model
{
public:
	//** 
	//struct	_vertex;
	//struct	_face;
	//struct	data_face;
	//struct data_vertex;
	//**

	//** 




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
	typedef	xr_vector<_vertex*>			v_vertices;
	typedef	v_vertices::iterator		v_vertices_it;
	typedef	v_vertices::const_iterator	v_vertices_cit;

	typedef xr_vector<_face*>		v_faces;
	typedef v_faces::iterator		v_faces_it;
	typedef v_faces::const_iterator	v_faces_cit;
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
	_face*					create_face			( _vertex* v0, _vertex* v1, _vertex* v2, b_face& F );
	_vertex*				create_vertex		( Fvector& P );
	_face*					load_create_face	( Fvector& P1, Fvector& P2, Fvector& P3, b_face& F );
	_vertex*				load_create_vertex	( Fvector& P );
public:
							xrMU_Model			();
							~xrMU_Model			();
	void					clear_mesh			();
	void					Load				( IReader& F, u32 version );
//	void					calc_normals		();
	void					calc_materials		();
	void					calc_faceopacity	();
	void					calc_lighting		( xr_vector<base_color>& dest, const Fmatrix& xform, CDB::MODEL* M, base_lighting& lights, u32 flags );
	void					calc_lighting		();
//	void					calc_ogf			();
//	void					export_geometry		();
	void					export_cform_rcast	( CDB::CollectorPacked& CL, Fmatrix& xform );
	void					read				( INetReader	&r );
	void					write				( IWriter	&w ) const ;

	void					read_color			( INetReader	&r );
	void					write_color			( IWriter	&w ) const ;
	void					read_subdivs		( INetReader	&r );
	void					write_subdivs		( IWriter	&w ) const ;

	u32						find				( const _vertex *v )const;
	u32						find				( const _face *v )const;

	void					read				( INetReader	&r, _vertex* &v )const;
	void					read				( INetReader	&r, _face*	&v )const;


	void					write				( IWriter	&r, u32 id, const _vertex* v )const;
	void					write				( IWriter	&r, u32 id, const _face*	v )const;


static 	xrMU_Model			*read_create		();
	void					reading_open		();
	void					reading_close		();
	void					writting_open		() const;
	void					writting_close		() const;

private:

	typedef  vector_serialize< t_read<_face, get_id_self_index<_face> > >		tread_faces;
	typedef  vector_serialize< t_write<_face, get_id_self_index<_face>>  >		twrite_faces;

	typedef  vector_serialize< t_read<_vertex, get_id_self_index<_vertex> >>	tread_vertices;
	typedef  vector_serialize< t_write<_vertex, get_id_self_index<_vertex> >>	twrite_vertices;

		tread_faces				*read_faces;
mutable	twrite_faces			*write_faces;

		tread_vertices			*read_vertices;
mutable	twrite_vertices			*write_vertices;

private:
			void				read_adjacents		( INetReader	&r );
			void				write_adjacents		( IWriter	&w ) const;
	static	void				read_adjacents		( INetReader	&r, xrMU_Model::tread_faces &read_faces, _vertex &v );
	static	void				write_adjacents		( IWriter	&w, xrMU_Model::twrite_faces &write_faces, const _vertex &v );


			void				read_face_verts		( INetReader	&r );
			void				write_face_verts	( IWriter	&w ) const;
	static	void				read_face_verts		( INetReader	&r, xrMU_Model::tread_vertices &read_verts, _face &v );
	static	void				write_face_verts	( IWriter	&w, xrMU_Model::twrite_vertices &write_verts, const _face &v );

};

void XRLC_LIGHT_API	calc_normals	( xrMU_Model &model );


typedef  vector_serialize< t_read<xrMU_Model, get_id_standart<xrMU_Model> >  >		tread_models;
typedef  vector_serialize< t_write<xrMU_Model, get_id_standart<xrMU_Model> > >		twrite_models;

extern	tread_models		*read_models		;
extern	twrite_models		*write_models		;

#endif
