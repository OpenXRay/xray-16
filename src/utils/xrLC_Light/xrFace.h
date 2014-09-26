#pragma once


#include "../shader_xrlc.h"
//#include "r_light.h"

#include "tcf.h"
#include "base_face.h"

#include "MeshStructure.h"
#include "serialize.h"

#pragma pack(push,4)

struct DataFace;
class  CLightmap;

struct  DataVertex;
typedef	Tvertex< DataVertex> Vertex;

typedef std::pair<Vertex*, Vertex *>	PAIR_VV;
typedef xr_map<Vertex*,Vertex*>			map_v2v;	// vertex to vertex translation
typedef map_v2v::iterator				map_v2v_it;



struct  XRLC_LIGHT_API DataVertex	: public base_Vertex
{
public:
	//vecAdj		m_adjacents;
	typedef		DataFace			DataFaceType;

	IC	BOOL	 similar			( Tvertex<DataVertex> &V, float eps );

virtual	void		read	(INetReader	&r );
virtual	void		write	(IWriter	&w )const;

	DataVertex				(){};
	virtual		~DataVertex				(){};
};

typedef	 Tface<DataVertex>  Face;


struct XRLC_LIGHT_API DataFace	: public base_Face
{
public:

	//Vertex*					v[3];			// vertices
	Fvector					N;				// face normal

	svector<_TCF,2>			tc;				// TC

	void*					pDeflector;		// does the face has LM-UV map?
	CLightmap*				lmap_layer;
	u32						sm_group;
	virtual Fvector2*		getTC0			( ) { return tc[0].uv; }


	BOOL		RenderEqualTo		( Face *F );

	void		AddChannel			( Fvector2 &p1, Fvector2 &p2, Fvector2 &p3 ); 
	BOOL		hasImplicitLighting	();

	virtual	void		read	(INetReader	&r );
	virtual	void		write	(IWriter	&w )const;

	DataFace(){};
	virtual ~DataFace(){};
};


//struct Vertex;
//struct DataVertex;
//struct Face;
class Material;
class Edge;

// Typedefs
namespace detail
{
	typedef xr_vector<Vertex>::iterator	dummy_compiler_treatment;
} // namespace detail






#include		"xrUVpoint.h"

#include		"xrFaceInline.h"


extern XRLC_LIGHT_API bool						g_bUnregister;

#pragma pack(pop)

extern "C" XRLC_LIGHT_API	void start_unwarp_recursion	();
extern "C" XRLC_LIGHT_API	void destroy_vertex			( Vertex* &v, bool unregister );

							void destroy_face			( Face* &v, bool unregister );


typedef  vector_serialize< t_read<Vertex, get_id_self_index<Vertex> > >			tread_vertices;
typedef  vector_serialize< t_write<Vertex, get_id_self_index<Vertex> > >		twrite_vertices	;
typedef  vector_serialize< t_read<Face, get_id_self_index<Face> >  >			tread_faces;
typedef  vector_serialize< t_write<Face, get_id_self_index<Face> > >			twrite_faces;

extern	twrite_faces		*write_faces		;
extern	tread_faces			*read_faces			;
extern	tread_vertices		*read_vertices		;
extern	twrite_vertices		*write_vertices		;