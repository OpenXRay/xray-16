#include	"stdafx.h"

#include	"MeshMenderLayerOGF.h"


static		xr_vector< MeshMender::Vertex > mender_in_out_verts;
static		xr_vector< unsigned int >		mender_in_out_indices;
static		xr_vector< unsigned int >		mender_mapping_out_to_in_vert;
void		remove_isolated_verts( vecOGF_V &vertices, vecOGF_F &faces );
// Calculate T&B
void OGF::CalculateTB()
{
	remove_isolated_verts( data.vertices, data.faces );
		// ************************************* Declare inputs
	Status						( "Declarator..." );
	u32 v_count_reserve			= iFloor( float( data.vertices.size() )*1.33f );
	u32 i_count_reserve			= 3*data.faces.size();


	mender_mapping_out_to_in_vert	.clear( );

	mender_mapping_out_to_in_vert	.reserve( v_count_reserve );
	mender_in_out_verts				.reserve( v_count_reserve );
	mender_in_out_indices			.reserve( i_count_reserve );

	mender_in_out_verts				.clear( );
	mender_in_out_indices			.clear( );
	fill_mender_input( data.vertices, data.faces, mender_in_out_verts, mender_in_out_indices );

	u32			v_was	= data.vertices.size();
	u32			v_become= mender_in_out_verts.size();
	clMsg		("duplication: was[%d] / become[%d] - %2.1f%%",v_was,v_become,100.f*float(v_become-v_was)/float(v_was));

	// ************************************* Perform mungle
	Status			("Calculating basis...");
	
	MeshMender	mender	;
	if (	!mender.Mend		(
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

	retrive_data_from_mender_otput( data.vertices, data.faces, mender_in_out_verts, mender_in_out_indices, mender_mapping_out_to_in_vert  );
	remove_isolated_verts( data.vertices, data.faces );

	mender_in_out_verts				.clear( );
	mender_in_out_indices			.clear( );
	mender_mapping_out_to_in_vert	.clear( );
}

