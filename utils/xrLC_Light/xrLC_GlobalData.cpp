#include "stdafx.h"

#include "xrLC_GlobalData.h"
#include "xrface.h"
#include "xrdeflector.h"
#include "lightmap.h"
#include "serialize.h"
#include "mu_model_face.h"
#include "xrmu_model.h"
#include "xrmu_model_reference.h"
#include "../../xrcdb/xrcdb.h"

bool g_using_smooth_groups = true;
bool g_smooth_groups_by_faces = false;

xrLC_GlobalData* data =0;

tread_lightmaps		*read_lightmaps		= 0;
twrite_lightmaps	*write_lightmaps	= 0;

twrite_faces		*write_faces		= 0;
tread_faces			*read_faces			= 0;
tread_vertices		*read_vertices		= 0;
twrite_vertices		*write_vertices		= 0;
tread_deflectors	*read_deflectors	= 0;
twrite_deflectors	*write_deflectors	= 0;

tread_models		*read_models		= 0;
twrite_models		*write_models		= 0;
tread_mu_refs		*read_mu_refs		= 0;
twrite_mu_refs		*write_mu_refs		= 0;

 xrLC_GlobalData*	lc_global_data()
 {
	 return data;
 }
void	create_global_data()
{
	VERIFY( !inlc_global_data() );
	data = xr_new<xrLC_GlobalData>();
}
void	destroy_global_data()
{
	VERIFY( inlc_global_data() );
	if(data)
		data->clear();
	xr_delete(data);
}


xrLC_GlobalData::xrLC_GlobalData	():
 _b_nosun(false),_gl_linear(false),
	b_vert_not_register( false )
{
	
	_cl_globs._RCAST_Model = 0;
	write_faces = xr_new< twrite_faces	>( &_g_faces );
	read_faces = xr_new< tread_faces	>( &_g_faces );
}

//void xrLC_GlobalData	::create_write_faces() const
//{
//	VERIFY(!write_faces);
//	//write_faces = xr_new< twrite_faces	>( &_g_faces );
//}
//void xrLC_GlobalData::destroy_write_faces() const
//{
//	
//	//xr_delete(write_faces);
//}


//twrite_faces*	xrLC_GlobalData::get_write_faces()	
//{
//	return write_faces;
//}

//void xrLC_GlobalData	::create_read_faces()
//{
//	//VERIFY(!read_faces);
//	//read_faces = xr_new< tread_faces	>( &_g_faces );
//}
//void xrLC_GlobalData::destroy_read_faces()
//{
//	
//	//xr_delete(read_faces);
//}
//tread_faces	*	xrLC_GlobalData::get_read_faces()	
//{
//	return read_faces;
//}
/*
poolVertices &xrLC_GlobalData	::VertexPool	()		
{
	return	_VertexPool; 
}
poolFaces &xrLC_GlobalData	::FacePool			()		
{
	return	_FacePool;
}
*/




void	xrLC_GlobalData	::destroy_rcmodel	()
{
	xr_delete		(_cl_globs._RCAST_Model);
}
void xrLC_GlobalData::clear_build_textures_surface()
{
	clLog( "mem usage before clear build textures surface: %u", Memory.mem_usage() );
	//xr_vector<b_BuildTexture>		_textures;
	xr_vector<b_BuildTexture>::iterator i = textures().begin();
	xr_vector<b_BuildTexture>::const_iterator e = textures().end();
	for(;i!=e;++i)
		::clear((*i));
	Memory.mem_compact();
	clLog( "mem usage after clear build textures surface: %u", Memory.mem_usage() );
}
void xrLC_GlobalData::clear_build_textures_surface( const xr_vector<u32> &exept )
{
	clLog( "mem usage before clear build textures surface: %u", Memory.mem_usage() );
	xr_vector<b_BuildTexture>::iterator i = textures().begin();
	xr_vector<b_BuildTexture>::const_iterator e = textures().end();
	xr_vector<b_BuildTexture>::const_iterator b = textures().begin();
	for(;i!=e;++i)
	{
		xr_vector<u32>::const_iterator ff = std::find( exept.begin(), exept.end(),u32( i - b ) );
		if( ff ==  exept.end() )
			::clear((*i));
	}
	Memory.mem_compact();
	clLog( "mem usage after clear build textures surface: %u", Memory.mem_usage() );
}

void	xrLC_GlobalData	::create_rcmodel	(CDB::CollectorPacked& CL)
{
	VERIFY(!_cl_globs._RCAST_Model);
	_cl_globs._RCAST_Model				= xr_new<CDB::MODEL> ();
	_cl_globs._RCAST_Model->build		(CL.getV(),(int)CL.getVS(),CL.getT(),(int)CL.getTS());
}

void		xrLC_GlobalData	::				initialize		()
{
	if (strstr(Core.Params,"-att"))	_gl_linear	= true;
}


/*
		xr_vector<b_BuildTexture>		_textures;
		xr_vector<b_material>			_materials;
		Shader_xrLC_LIB					_shaders;				
		CMemoryWriter					_err_invalid;
		b_params						_g_params;
		vecVertex						_g_vertices;
		vecFace							_g_faces;
		vecDefl							_g_deflectors;
		base_lighting					_L_static;
		CDB::MODEL*						_RCAST_Model;
		bool							_b_nosun;
		bool							_gl_linear;
*/

//void			xrLC_GlobalData	::				cdb_read_create	() 
//{
//	VERIFY(!_RCAST_Model);
//	_RCAST_Model = xr_new<CDB::MODEL> ();
//	_RCAST_Model->build( &*verts.begin(), (int)verts.size(), &*tris.begin(), (int)tris.size() );
//}

//base_Face* F		= (base_Face*)(*((void**)&T.dummy));

//*((u32*)&F)

base_Face* convert_nax( u32 dummy )
{
	return (base_Face*)(*((void**)&dummy));
}

u32 convert_nax( base_Face* F )
{
	return *((u32*)&F);
}

void write( IWriter	&w, const CDB::TRI &tri )
{
	w.w_u32( tri.verts[ 0 ] );
	w.w_u32( tri.verts[ 1 ] );
	w.w_u32( tri.verts[ 2 ] );
}
void write( IWriter	&w, const CDB::TRI &tri, const xrLC_GlobalData  &lc_global_data )
{
	::write( w, tri );
	const base_Face* F=  convert_nax( tri.dummy );
	VERIFY( &lc_global_data );
	lc_global_data.write( w, F );
}

void read( INetReader	&r, CDB::TRI &tri )
{
	tri.verts[ 0 ]  = r.r_u32( );
	tri.verts[ 1 ]  = r.r_u32( );
	tri.verts[ 2 ]  = r.r_u32( );
}

void read( INetReader	&r, CDB::TRI &tri, xrLC_GlobalData  &lc_global_data  )
{
	::read( r, tri );
	VERIFY( &lc_global_data );
	base_Face* F = 0;
	lc_global_data.read( r, F );
	tri.dummy = convert_nax( F );
}

static xr_vector<Fvector> verts;
static xr_vector<CDB::TRI> tris;

void read( INetReader	&r, CDB::MODEL* &m, xrLC_GlobalData  &lc_global_data )
{
	
	verts.clear();
	tris.clear();
	r_pod_vector( r, verts );

	u32 tris_count = r.r_u32();
	tris.resize( tris_count );
	for( u32 i = 0; i < tris_count; ++i)
		::read( r, tris[i], lc_global_data );

	VERIFY(!m);
	m = xr_new<CDB::MODEL> ();
	m->build( &*verts.begin(), (int)verts.size(), &*tris.begin(), (int)tris.size() );
	verts.clear();
	tris.clear();
}

void read( INetReader	&r, CDB::MODEL &m )
{
	verts.clear();
	tris.clear();
	r_pod_vector( r, verts );
	u32 tris_count = r.r_u32();
	tris.resize( tris_count );
	for( u32 i = 0; i < tris_count; ++i)
		::read( r, tris[i] );
	m.build( &*verts.begin(), (int)verts.size(), &*tris.begin(), (int)tris.size() );
	verts.clear();
	tris.clear();
}







void write( IWriter	&w, const  CDB::MODEL &m )
{
	w.w_u32( (u32)m.get_verts_count() );
	w.w( m.get_verts(), m.get_verts_count() * sizeof(Fvector) );
	
	u32 tris_count = (u32) m.get_tris_count() ;
	w.w_u32( tris_count );
	for( u32 i = 0; i < tris_count; ++i)
		::write( w, m.get_tris()[i] );

//	w.w( m.get_tris(), m.get_tris_count() * sizeof(CDB::TRI) );
}

void write( IWriter	&w, const  CDB::MODEL &m, const  xrLC_GlobalData  &lc_global_data )
{
	w.w_u32( (u32)m.get_verts_count() );
	w.w( m.get_verts(), m.get_verts_count() * sizeof(Fvector) );
	
	u32 tris_count = (u32) m.get_tris_count() ;
	w.w_u32( tris_count );
	for( u32 i = 0; i < tris_count; ++i)
		::write( w, m.get_tris()[i], lc_global_data );

//	w.w( m.get_tris(), m.get_tris_count() * sizeof(CDB::TRI) );
}

void			xrLC_GlobalData	::read_base		( INetReader &r )
{
	_b_nosun = !!r.r_u8();
	_gl_linear = !!r.r_u8();
	r_pod( r, _cl_globs._g_params );
	
	_cl_globs._L_static.read( r );

	
	r_vector( r, _cl_globs._textures );
	r_pod_vector( r, _cl_globs._materials );
	r_pod_vector( r, _cl_globs._shaders.Library	() );	
	//	CMemoryWriter					_err_invalid;


	read_lightmaps= xr_new< tread_lightmaps >( &_g_lightmaps );
	read_lightmaps->read( r );

	//
	read_mu_models( r );
	//
}
void			xrLC_GlobalData	::write_base		( IWriter	&w ) const 
{
	/////////////////////////////////////////////////////////	
	w.w_u8(_b_nosun);
	w.w_u8(_gl_linear);
	w_pod( w, _cl_globs._g_params );

	_cl_globs._L_static.write( w );

	w_vector( w, _cl_globs._textures );
	w_pod_vector( w, _cl_globs._materials );
	w_pod_vector( w, _cl_globs._shaders.Library	() );	
	//	CMemoryWriter					_err_invalid;
	write_lightmaps= xr_new< twrite_lightmaps >( &_g_lightmaps );
	write_lightmaps->write( w );

	write_mu_models( w );
////////////////////////////////////////////////////////////////////////////
}


void		xrLC_GlobalData	::read			( INetReader	&r )
{
	



	//read_faces = xr_new< tread_faces	>( &_g_faces );
	read_faces->read( r );


	
	::read( r, _cl_globs._RCAST_Model, *this );

	close_models_read( );
	xr_delete( read_lightmaps );
	//xr_delete( read_faces );

	//read_lm_data( r );
}

void	xrLC_GlobalData::write( IWriter	&w ) const
{

	//write_faces = xr_new< twrite_faces	>( &_g_faces );
	write_faces->write( w );


	//write_models
	::write( w, *_cl_globs._RCAST_Model, *this );
	close_models_write( );
	xr_delete( write_lightmaps );
	//xr_delete( write_faces );

	//write_lm_data ( w );
}

void	xrLC_GlobalData::mu_models_calc_materials()
{
	for (u32 m=0; m<mu_models().size(); m++)
			mu_models()[m]->calc_materials();

}


void	xrLC_GlobalData::read_lm_data	( INetReader	&r )
{
	read_vertices( r );
	read_deflectors = xr_new< tread_deflectors	>( &_g_deflectors );
	//create_read_faces();
	read_deflectors->read( r );
	//destroy_read_faces();
	xr_delete( ::read_vertices );
	xr_delete( read_deflectors );
}
void		xrLC_GlobalData	::				write_lm_data	( IWriter	&w )const
{
	write_vertices( w );
	write_deflectors = xr_new< twrite_deflectors	>( &_g_deflectors );
	//create_write_faces();
	write_deflectors->write( w );
	//destroy_write_faces();
	xr_delete( ::write_vertices );
	xr_delete( write_deflectors );
}

void	xrLC_GlobalData	::	read_vertices	( INetReader	&r )
{
		//not used for light//
	::read_vertices = xr_new< tread_vertices	>( &_g_vertices );
	::read_vertices->read( r );
	vecFaceIt i = _g_faces.begin(), e = _g_faces.end();
	for( ; e!=i ; ++i )
		(*i)->read_vertices( r );
}
void	xrLC_GlobalData	::	write_vertices	( IWriter	&w )const
{

	::write_vertices = xr_new< twrite_vertices	>( &(_g_vertices) );
	::write_vertices->write( w );
	vecFaceCit i = _g_faces.begin(), e = _g_faces.end();
	for( ; e!=i ; ++i )
		(*i)->write_vertices( w );
	
}
void	xrLC_GlobalData	::						read_mu_models			( INetReader &r )
{
	
	read_models =  xr_new< tread_models	>( &_mu_models );
	read_models->read( r );
	xr_delete( read_models );

}
void	xrLC_GlobalData	::						write_mu_models			( IWriter	&w ) const 
{
	write_models =  xr_new< twrite_models	>( &_mu_models );
	write_models ->write( w );
	xr_delete( write_models );

}

void			xrLC_GlobalData	::				read_modes_color( INetReader	&r )
{
	xr_vector<xrMU_Model*>::iterator i = _mu_models.begin(), e = _mu_models.end();
	for(;e!=i;++i)
		(*i)->read_color( r );

}
void			xrLC_GlobalData	::				write_modes_color( IWriter	&w )const
{
 	xr_vector<xrMU_Model*>::const_iterator i = _mu_models.begin(), e = _mu_models.end();
	for(;e!=i;++i)
		(*i)->write_color( w );
}

void	xrLC_GlobalData	::						read_mu_model_refs			( INetReader &r )
{
	read_mu_refs	=  xr_new< tread_mu_refs	>( &_mu_refs );
	read_models		=  xr_new< tread_models	>( &_mu_models );
	//

	read_mu_refs->read( r );

	xr_delete( read_models );
	xr_delete( read_mu_refs );
	
}
void	xrLC_GlobalData	::						write_mu_model_refs			( IWriter	&w ) const 
{
	write_mu_refs =  xr_new< twrite_mu_refs	>( &_mu_refs );
	write_models	=  xr_new< twrite_models	>( &_mu_models );

	write_mu_refs->write( w );

	xr_delete( write_models );
	xr_delete( write_mu_refs );
}

bool			xrLC_GlobalData	::			b_r_vertices	()		
{
	return !!::read_vertices;
}

//bool			xrLC_GlobalData	::			b_r_faces		()		
//{
//	return !!read_faces;
//}


void			xrLC_GlobalData	::			close_models_read		()
{
	xr_vector<xrMU_Model*> :: iterator i = _mu_models.begin() , e = _mu_models.end();
	for( ; e!= i; ++i )
		(*i)->reading_close();
}
void			xrLC_GlobalData	::			close_models_write		()const
{
	xr_vector<xrMU_Model*> :: const_iterator i = _mu_models.begin() , e = _mu_models.end();
	for( ; e!= i; ++i )
		(*i)->writting_close();
}

template<typename T>
std::pair<u32,u32>	get_id( const xr_vector<xrMU_Model*>& mu_models, const T * v )
{


	u32 face_id = u32(-1);
	struct find
	{
		const T * _v;
		u32& _id;
		find( const T * v, u32& id) : _v(v), _id( id )
		{}
		bool operator () ( const xrMU_Model * m )
		{	
			VERIFY(m);
			u32 id = m->find( _v );
			if( id == u32(-1) )
				return false;
			_id = id;
			return true;
		}
	} f( v, face_id );

	xr_vector<xrMU_Model*> :: const_iterator ii =std::find_if( mu_models.begin(), mu_models.end(), f );
	if( ii == mu_models.end() )
		return std::pair<u32,u32>(u32(-1), u32(-1));
	return std::pair<u32,u32>(u32(ii-mu_models.begin()), face_id );
}

//std::pair<u32,u32>			xrLC_GlobalData	::		get_id		( const _face * v ) const
//{
//	return ::get_id( _mu_models, v );
//}
//
//std::pair<u32,u32>			xrLC_GlobalData	::		get_id		( const _vertex * v ) const
//{
//	return ::get_id( _mu_models, v );
//}
enum serialize_mesh_item_type
{
	smit_plain = u8(0),
	smit_model = u8(1),
	smit_null  = u8(-1)
};





void			xrLC_GlobalData	::	read			( INetReader &r, base_Face* &f )
{
	VERIFY(!f);
	u8 type  = r.r_u8( );

	switch( type  )
	{
		case smit_plain:
			{
				VERIFY(read_faces);
				Face * face = 0;
				read_faces->read( r, face );
				f = face;
				return;
			}
		case smit_model:
			{
				u32 model_id = r.r_u32();
				_face *model_face = 0;
				_mu_models[ model_id ]->read( r, model_face );
				f = model_face;
				return;
			}
		case smit_null: 
			return;
	}
}

void xrLC_GlobalData	::	write( IWriter &w, const base_Face *f ) const 
{
	
	if(!f)
	{
		w.w_u8( smit_null );
		return;
	}

	const Face *face = dynamic_cast<const Face*>( f );
	if(face)
	{
		VERIFY( write_faces );
		w.w_u8( smit_plain );
		write_faces->write( w, face );
		return;
	}

	const _face *model_face = dynamic_cast<const _face*>( f );
	VERIFY(model_face);

	w.w_u8( smit_model );

	std::pair<u32,u32> id = get_id( _mu_models, model_face );
	
	w.w_u32( id.first );

	_mu_models[ id.first ]->write( w, id.second, model_face  );

}

xrLC_GlobalData::~xrLC_GlobalData()
{
	xr_delete(write_faces);
	xr_delete(read_faces);
	//u32 i;
	//i++;
}



template<typename T>
void vec_clear( xr_vector<T*> &v )
{
	typename xr_vector<T*>::iterator i = v.begin(), e = v.end();
	for(;i!=e;++i)
			xr_delete(*i);
	v.clear();
}

template<typename T>
void vec_spetial_clear( xr_vector<T> &v )
{
	typename xr_vector<T>::iterator i = v.begin(), e = v.end();
	for(;i!=e;++i)
		clear(*i);
	v.clear();
}

void mu_mesh_clear();
void	xrLC_GlobalData::clear_mu_models	()
{	

		clLog( "mem usage before mu_clear %d", Memory.mem_usage() );
		vec_clear(_mu_models);// not clear ogf
		vec_clear(_mu_refs);
		mu_mesh_clear();
		Memory.mem_compact();
		clLog( "mem usage after mu_clear: %d", Memory.mem_usage() );

}
void		xrLC_GlobalData::				clear			()
{
		vec_spetial_clear(_cl_globs._textures );
		_cl_globs._materials.clear();
		_cl_globs._shaders.Unload();
	//	CMemoryWriter					_err_invalid;
	//	b_params						_g_params;
		close_models_read();
		close_models_write();

		vec_clear(_g_lightmaps);
		vec_clear(_mu_models);//mem leak
		vec_clear(_mu_refs);
		mu_mesh_clear();
		gl_mesh_clear();
		//VertexPool;
		//FacePool;

	

	//	vecVertex						_g_vertices;
	//	vecFace							_g_faces;
		gl_mesh_clear	();
	    vec_clear		(_g_deflectors);

		//base_lighting					_L_static;
		xr_delete(_cl_globs._RCAST_Model);

		xr_delete( write_lightmaps );
		xr_delete( ::write_vertices );
		//xr_delete( write_faces );
		xr_delete( write_deflectors );

		xr_delete( read_lightmaps );
		xr_delete( ::read_vertices );
		//xr_delete( read_faces );
		xr_delete( read_deflectors );
//		bool							_b_nosun;
//		bool							_gl_linear;
}


void		xrLC_GlobalData::set_faces_indexses		()
{
	//const u32 number = g_faces		().size();
	//for( u32 i=0; i< number; ++i	)
	//	g_faces()[i]->set_index( i );
}
void		xrLC_GlobalData::set_vertices_indexses	()
{
//	const u32 number = g_vertices().size();
//	for( u32 i=0; i< number; ++i	)
//		g_vertices()[i]->set_index( i );
}

