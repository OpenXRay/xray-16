#include	"stdafx.h"
#include	"build.h"

#include	"MeshMenderLayerOrdinaryStatic.h"
#include	"../xrLC_Light/xrLC_GlobalData.h"

static u32 find_same_vertex( const xr_vector<u32> &m, const Fvector2	&Ftc, const xr_vector< MeshMender::Vertex > &theVerts )
{
	// Search
	for ( u32 it=0; it<m.size(); it++ )
	{
		u32	m_id	= m[it];
		float tc[2]		= { theVerts[ m_id ].s, theVerts[ m_id ].t }; 
		if ( !fsimilar(tc[0],Ftc.x ) )	
			continue;
		if ( !fsimilar( tc[1], Ftc.y ) )	
			continue;
		return m_id;
	}
	return u32(-1);
}

static u32 add_vertex(	 const	Vertex						&V,
				 const	Fvector2					&Ftc,
				 xr_vector< MeshMender::Vertex >	&theVerts
			   )
{
	MeshMender::Vertex new_vertex;
	set_vertex( new_vertex, V, Ftc );
	theVerts.push_back( new_vertex );
	return theVerts.size() - 1;
}

static void	add_face(	const Face& F, 
					xr_vector< MeshMender::Vertex >& theVerts,
					xr_vector< unsigned int >& theIndices,
					xr_vector<xr_vector<u32> >	&remap )
{
	for (u32 v=0; v<3; v++)
	{
		const Vertex	*V	= F.v[v];	
		u32				ID	= u32(std::lower_bound(lc_global_data()->g_vertices().begin(),lc_global_data()->g_vertices().end(),V)-lc_global_data()->g_vertices().begin());
		xr_vector<u32>	&m	= remap[ID];
		Fvector2		Ftc = F.tc.front().uv[v];

		u32 vertex_index	= find_same_vertex( m, Ftc, theVerts );
		
		// Register new if not found
		if ( vertex_index == u32(-1) )
		{
			vertex_index =	add_vertex( *V, Ftc, theVerts );
			remap[ID].push_back( vertex_index );
		}
		
		theIndices	.push_back		(vertex_index);
	}
}

static void	fill_mender_input( xr_vector< MeshMender::Vertex >& theVerts, xr_vector< unsigned int >& theIndices )
{
		// ************************************* Build vectors + expand TC if nessesary
	Status						("Building inputs...");
	std::sort					(lc_global_data()->g_vertices().begin(),lc_global_data()->g_vertices().end());
	xr_vector<xr_vector<u32> >	remap;
	remap.resize				(lc_global_data()->g_vertices().size());
	for (u32 f=0; f<lc_global_data()->g_faces().size(); f++)
	{
		Progress	(float(f)/float(lc_global_data()->g_faces().size()));
		Face*	F	= lc_global_data()->g_faces()[f];
		add_face( *F, theVerts, theIndices, remap );
	}
	remap.clear	();
}

static void retrive_data_from_mender_otput( const	 xr_vector< MeshMender::Vertex >& theVerts, const xr_vector< unsigned int >& theIndices )

{
	// ************************************* Retreive data
	Status						("Retreiving basis...");
	for (u32 f=0; f<lc_global_data()->g_faces().size(); f++)
	{
		Face*	F						=	lc_global_data()->g_faces()[f];
		u32	id0							=	theIndices	[f*3+0];	// vertex index
		u32	id1							=	theIndices	[f*3+1];	// vertex index
		u32	id2							=	theIndices	[f*3+2];	// vertex index
		R_ASSERT						( id0 < theVerts.size() );
		R_ASSERT						( id1 < theVerts.size() );
		R_ASSERT						( id2 < theVerts.size() );
		MeshMender::Vertex verts[3] =	{ theVerts[id0], theVerts[id1], theVerts[id2] };
		set_face( *F, verts );
	}
}
static		xr_vector< MeshMender::Vertex > mender_in_out_verts;
static		xr_vector< unsigned int >		mender_in_out_indices;
static		xr_vector< unsigned int >		mender_mapping_out_to_in_vert;

void CBuild::xrPhase_TangentBasis()
{
	// ************************************* Declare inputs
	Status						("Declarator...");
	u32 v_count_reserve			= iFloor(float(lc_global_data()->g_vertices().size())*1.33f);
	u32 i_count_reserve			= 3*lc_global_data()->g_faces().size();
	
	mender_in_out_verts				.clear( );
	mender_in_out_indices			.clear( );
	mender_mapping_out_to_in_vert	.clear( );


	mender_in_out_verts				.reserve( v_count_reserve );
	mender_in_out_indices			.reserve( i_count_reserve );
	mender_mapping_out_to_in_vert	.reserve( v_count_reserve );


	fill_mender_input( mender_in_out_verts, mender_in_out_indices );

	u32			v_was	= lc_global_data()->g_vertices().size();
	u32			v_become= mender_in_out_verts.size();
	clMsg		("duplication: was[%d] / become[%d] - %2.1f%%",v_was,v_become,100.f*float(v_become-v_was)/float(v_was));

	// ************************************* Perform mungle
	Status			("Calculating basis...");
	
	MeshMender	mender	;

	if ( !mender.Mend		(
		  mender_in_out_verts,
		  mender_in_out_indices,
		  mender_mapping_out_to_in_vert,
		  1,
		  0.5,
		  0.5,
		  0.0f,
		  MeshMender::DONT_CALCULATE_NORMALS,
		  MeshMender::RESPECT_SPLITS,
		  MeshMender::DONT_FIX_CYLINDRICAL
		)
	)
	{
		Debug.fatal	(DEBUG_INFO, "NVMeshMender failed " );
		//Debug.fatal	(DEBUG_INFO,"NVMeshMender failed (%s)",mender.GetLastError().c_str());
	}

	// ************************************* Bind declarators
	// bind

	retrive_data_from_mender_otput( mender_in_out_verts, mender_in_out_indices );
	mender_in_out_verts				.clear( );
	mender_in_out_indices			.clear( );
	mender_mapping_out_to_in_vert	.clear( );
}

