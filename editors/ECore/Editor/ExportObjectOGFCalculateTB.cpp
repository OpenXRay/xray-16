#include "stdafx.h"
#pragma hdrstop

#include "ExportObjectOGF.h"

//--------------------------------------------------------------------------------
#include "../../common/nvMender2003/nvmeshmender.h"
#include "../../common/NvMender2003/nvMeshMender.h"
#include "../../common/NvMender2003/mender_input_output.h"
#include "../../common/NvMender2003/remove_isolated_verts.h"
//--------------------------------------------------------------------------------
IC void	set_vertex( MeshMender::Vertex &out_vertex, const SOGFVert& in_vertex )
{
			cv_vector( out_vertex.pos,		in_vertex.P );
			cv_vector( out_vertex.normal,	in_vertex.N );
			out_vertex.s		= in_vertex.UV.x;
			out_vertex.t		= in_vertex.UV.y;
			//out_vertex.tangent;
			//out_vertex.binormal;
}

IC void	set_vertex( SOGFVert& out_vertex,  const SOGFVert& in_old_vertex, const MeshMender::Vertex &in_vertex )
{
			out_vertex = in_old_vertex;

			cv_vector( out_vertex.P, in_vertex.pos );//?
			cv_vector( out_vertex.N, in_vertex.normal );//?

			out_vertex.UV.x	= in_vertex.s;
			out_vertex.UV.y	= in_vertex.t;
			Fvector tangent; Fvector binormal;
			out_vertex.T.set( cv_vector( tangent, in_vertex.tangent ) );
			out_vertex.B.set( cv_vector( binormal, in_vertex.binormal ) );
}


IC WORD	&face_vertex( SOGFFace &F, u32 vertex_index )
{
	VERIFY( vertex_index < 3 );
	return F.v[vertex_index];
}

IC const WORD &face_vertex( const SOGFFace &F, u32 vertex_index )
{
	VERIFY( vertex_index < 3 );
	return F.v[vertex_index];
}

//--------------------------------------------------------------------------------------------
void CObjectOGFCollectorPacked::CalculateTB()
{
    /*
	u32 v_count_reserve			= 3*iFloor(float(m_Verts.size())*1.33f);
	u32 i_count_reserve			= 3*m_Faces.size();

	// Declare inputs
	xr_vector<NVMeshMender::VertexAttribute> 			input;
	input.push_back	(NVMeshMender::VertexAttribute());	// pos
	input.push_back	(NVMeshMender::VertexAttribute());	// norm
	input.push_back	(NVMeshMender::VertexAttribute());	// tex0
	input.push_back	(NVMeshMender::VertexAttribute());	// *** faces

	input[0].Name_= "position";	xr_vector<float>&	i_position	= input[0].floatVector_;	i_position.reserve	(v_count_reserve);
	input[1].Name_= "normal";	xr_vector<float>&	i_normal	= input[1].floatVector_;	i_normal.reserve	(v_count_reserve);
	input[2].Name_= "tex0";		xr_vector<float>&	i_tc		= input[2].floatVector_;	i_tc.reserve		(v_count_reserve);
	input[3].Name_= "indices";	xr_vector<int>&		i_indices	= input[3].intVector_;		i_indices.reserve	(i_count_reserve);

	// Declare outputs
	xr_vector<NVMeshMender::VertexAttribute> 			output;
	output.push_back(NVMeshMender::VertexAttribute());	// position, needed for mender
	output.push_back(NVMeshMender::VertexAttribute());	// normal
	output.push_back(NVMeshMender::VertexAttribute());	// tangent
	output.push_back(NVMeshMender::VertexAttribute());	// binormal
	output.push_back(NVMeshMender::VertexAttribute());	// tex0
	output.push_back(NVMeshMender::VertexAttribute());	// *** faces

	output[0].Name_= "position";
	output[1].Name_= "normal";
	output[2].Name_= "tangent";	
	output[3].Name_= "binormal";
	output[4].Name_= "tex0";	
	output[5].Name_= "indices";	

    // fill inputs (verts&indices)
    for (OGFVertIt vert_it=m_Verts.begin(); vert_it!=m_Verts.end(); vert_it++){
        SOGFVert	&iV = *vert_it;
        i_position.push_back(iV.P.x);	i_position.push_back(iV.P.y);	i_position.push_back(iV.P.z);
        i_normal.push_back	(iV.N.x);  	i_normal.push_back	(iV.N.y);	i_normal.push_back	(iV.N.z);
        i_tc.push_back		(iV.UV.x);	i_tc.push_back		(iV.UV.y);	i_tc.push_back		(0);
    }
    for (OGFFaceIt face_it=m_Faces.begin(); face_it!=m_Faces.end(); face_it++){
        SOGFFace	&iF = *face_it;
		i_indices.push_back	(iF.v[0]);
		i_indices.push_back	(iF.v[1]);
		i_indices.push_back	(iF.v[2]);
    }
    
	// Perform munge
	NVMeshMender mender;
	if (!mender.Munge(
		input,										// input attributes
		output,										// outputs attributes
		deg2rad(75.f),								// tangent space smooth angle
		0,											// no texture matrix applied to my texture coordinates
		NVMeshMender::FixTangents,					// fix degenerate bases & texture mirroring
		NVMeshMender::DontFixCylindricalTexGen,		// handle cylindrically mapped textures via vertex duplication
		NVMeshMender::DontWeightNormalsByFaceSize	// weigh vertex normals by the triangle's size
		))
	{
		Debug.fatal	(DEBUG_INFO,"NVMeshMender failed (%s)",mender.GetLastError().c_str());
	}

	// Bind declarators
	// bind
	output[0].Name_= "position";
	output[1].Name_= "normal";
	output[2].Name_= "tangent";	
	output[3].Name_= "binormal";
	output[4].Name_= "tex0";	
	output[5].Name_= "indices";	

	xr_vector<float>&	o_position	= output[0].floatVector_;	R_ASSERT(output[0].Name_=="position");
	xr_vector<float>&	o_normal	= output[1].floatVector_;	R_ASSERT(output[1].Name_=="normal");
	xr_vector<float>&	o_tangent	= output[2].floatVector_;	R_ASSERT(output[2].Name_=="tangent");
	xr_vector<float>&	o_binormal	= output[3].floatVector_;	R_ASSERT(output[3].Name_=="binormal");
	xr_vector<float>&	o_tc		= output[4].floatVector_;	R_ASSERT(output[4].Name_=="tex0");
	xr_vector<int>&		o_indices	= output[5].intVector_;		R_ASSERT(output[5].Name_=="indices");

	// verify
	R_ASSERT		(3*m_Faces.size()	== o_indices.size());
    u32 v_cnt		= o_position.size();
    R_ASSERT		(0==v_cnt%3);
    R_ASSERT		(v_cnt == o_normal.size());
    R_ASSERT		(v_cnt == o_tangent.size());
    R_ASSERT		(v_cnt == o_binormal.size());
    R_ASSERT		(v_cnt == o_tc.size());
    v_cnt			/= 3;

    // retriving data
    u32 o_idx		= 0;
    for (face_it=m_Faces.begin(); face_it!=m_Faces.end(); face_it++){
        SOGFFace	&iF = *face_it;
        iF.v[0]		= (u16)o_indices[o_idx++];
        iF.v[1]		= (u16)o_indices[o_idx++];
        iF.v[2]		= (u16)o_indices[o_idx++];
    }
    m_Verts.clear	(); m_Verts.resize(v_cnt);
    for (u32 v_idx=0; v_idx!=v_cnt; v_idx++){
        SOGFVert	&iV = m_Verts[v_idx];
        iV.P.set	(o_position[v_idx*3+0],	o_position[v_idx*3+1],	o_position[v_idx*3+2]);
        iV.N.set	(o_normal[v_idx*3+0],	o_normal[v_idx*3+1],	o_normal[v_idx*3+2]);
        iV.T.set	(o_tangent[v_idx*3+0],	o_tangent[v_idx*3+1],	o_tangent[v_idx*3+2]);
        iV.B.set	(o_binormal[v_idx*3+0],	o_binormal[v_idx*3+1],	o_binormal[v_idx*3+2]);
        iV.UV.set	(o_tc[v_idx*3+0],		o_tc[v_idx*3+1]);
    }
  */


  	xr_vector<MeshMender::Vertex>	mender_in_out_verts;
	xr_vector< unsigned int >		mender_in_out_indices;
	xr_vector< unsigned int >		mender_mapping_out_to_in_vert;

	fill_mender_input( m_Verts, m_Faces, mender_in_out_verts, mender_in_out_indices );

	MeshMender	mender	;
	if
	( 
		!mender.Mend		
		(
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
		Debug.fatal	( DEBUG_INFO, "NVMeshMender failed " );
		//Debug.fatal	(DEBUG_INFO,"NVMeshMender failed (%s)",mender.GetLastError().c_str());
	}
	
	retrive_data_from_mender_otput( m_Verts, m_Faces, mender_in_out_verts, mender_in_out_indices, mender_mapping_out_to_in_vert  );
	t_remove_isolated_verts( m_Verts, m_Faces );

	mender_in_out_verts				.clear( );
	mender_in_out_indices			.clear( );
	mender_mapping_out_to_in_vert	.clear( );

    OptimizeTextureCoordinates();



}
