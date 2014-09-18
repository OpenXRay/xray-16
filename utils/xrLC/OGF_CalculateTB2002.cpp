#include	"stdafx.h"

#include	"NvMender2002/nvMeshMender.h"

void fill_mender_input( const	vecOGF_V	&vertices,
						const	vecOGF_F	&faces,
						xr_vector<float>	&i_position,
						xr_vector<float>	&i_normal,
						xr_vector<float>	&i_tc,
						xr_vector<float>	&i_color,
						xr_vector<int>		&i_indices  )
{
	// fill inputs (verts&indices)
	for (citOGF_V vert_it=vertices.begin(); vert_it!=vertices.end(); vert_it++){
		const OGF_Vertex	&iV = *vert_it;
		i_position.push_back(iV.P.x);		i_position.push_back(iV.P.y);		i_position.push_back(iV.P.z);
		i_normal.push_back	(iV.N.x);  		i_normal.push_back	(iV.N.y);		i_normal.push_back	(iV.N.z);
		i_color.push_back	(iV.Color._x);	i_color.push_back	(iV.Color._y);	i_color.push_back	(iV.Color._z);
		i_tc.push_back		(iV.UV[0].x);	i_tc.push_back		(iV.UV[0].y);	i_tc.push_back		(0);
	}
	for (citOGF_F face_it=faces.begin(); face_it!=faces.end(); face_it++){
		const OGF_Face	&iF = *face_it;
		i_indices.push_back	(iF.v[0]);
		i_indices.push_back	(iF.v[1]);
		i_indices.push_back	(iF.v[2]);
	}
}
void retrive_data_from_mender_otput( vecOGF_V						&vertices,
									 vecOGF_F						&faces, 
									 const xr_vector<float>			&o_position,
									 const xr_vector<float>			&o_normal,
							  		 const xr_vector<float>			&o_tc,
							  		 const xr_vector<float>			&o_tangent,
							  		 const xr_vector<float>			&o_binormal,
							  		 const xr_vector<float>			&o_color,
							  		 const xr_vector<int>			&o_indices )
{
	// retriving data
	u32 v_cnt		= o_position.size();
	v_cnt			/= 3;
	u32 o_idx		= 0;
	for (itOGF_F face_it=faces.begin(); face_it!=faces.end(); face_it++){
		OGF_Face	&iF = *face_it;
		iF.v[0]		= o_indices[o_idx++];
		iF.v[1]		= o_indices[o_idx++];
		iF.v[2]		= o_indices[o_idx++];
	}
	vertices.clear	(); vertices.resize(v_cnt);
	for (u32 v_idx=0; v_idx!=v_cnt; v_idx++){
		OGF_Vertex	&iV = vertices[v_idx];
		iV.P.set	(o_position[v_idx*3+0],	o_position[v_idx*3+1],	o_position[v_idx*3+2]);
		iV.N.set	(o_normal[v_idx*3+0],	o_normal[v_idx*3+1],	o_normal[v_idx*3+2]);
		iV.T.set	(o_tangent[v_idx*3+0],	o_tangent[v_idx*3+1],	o_tangent[v_idx*3+2]);
		iV.B.set	(o_binormal[v_idx*3+0],	o_binormal[v_idx*3+1],	o_binormal[v_idx*3+2]);
		iV.UV.resize(1);
		iV.UV[0].set(o_tc[v_idx*3+0],		o_tc[v_idx*3+1]);
		iV.Color._set(o_color[v_idx*3+0],	o_color[v_idx*3+1],		o_color[v_idx*3+2]);
	}
}
// Calculate T&B
void OGF::CalculateTB()
{
	u32 v_count_reserve			= 3*iFloor(float(data.vertices.size())*1.33f);
	u32 i_count_reserve			= 3*data.faces.size();

	// Declare inputs
	xr_vector<NVMeshMender::VertexAttribute> 			input;
	input.push_back	(NVMeshMender::VertexAttribute());	// pos
	input.push_back	(NVMeshMender::VertexAttribute());	// norm
	input.push_back	(NVMeshMender::VertexAttribute());	// tex0
	input.push_back	(NVMeshMender::VertexAttribute());	// color
	input.push_back	(NVMeshMender::VertexAttribute());	// *** faces

	input[0].Name_= "position";	xr_vector<float>&	i_position	= input[0].floatVector_;	i_position.reserve	(v_count_reserve);
	input[1].Name_= "normal";	xr_vector<float>&	i_normal	= input[1].floatVector_;	i_normal.reserve	(v_count_reserve);
	input[2].Name_= "tex0";		xr_vector<float>&	i_tc		= input[2].floatVector_;	i_tc.reserve		(v_count_reserve);
	input[3].Name_= "clr";		xr_vector<float>&	i_color		= input[3].floatVector_;	i_normal.reserve	(v_count_reserve);
	input[4].Name_= "indices";	xr_vector<int>&		i_indices	= input[4].intVector_;		i_indices.reserve	(i_count_reserve);

	// Declare outputs
	xr_vector<NVMeshMender::VertexAttribute> 			output;
	output.push_back(NVMeshMender::VertexAttribute());	// position, needed for mender
	output.push_back(NVMeshMender::VertexAttribute());	// normal
	output.push_back(NVMeshMender::VertexAttribute());	// tangent
	output.push_back(NVMeshMender::VertexAttribute());	// binormal
	output.push_back(NVMeshMender::VertexAttribute());	// tex0
	output.push_back(NVMeshMender::VertexAttribute());	// color
	output.push_back(NVMeshMender::VertexAttribute());	// *** faces

	output[0].Name_= "position";
	output[1].Name_= "normal";
	output[2].Name_= "tangent";	
	output[3].Name_= "binormal";
	output[4].Name_= "tex0";	
	output[5].Name_= "clr";	
	output[6].Name_= "indices";	


	fill_mender_input( data.vertices, data.faces, i_position, i_normal, i_tc, i_color, i_indices  );

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
	xr_vector<float>&	o_position	= output[0].floatVector_;	R_ASSERT(output[0].Name_=="position");
	xr_vector<float>&	o_normal	= output[1].floatVector_;	R_ASSERT(output[1].Name_=="normal");
	xr_vector<float>&	o_tangent	= output[2].floatVector_;	R_ASSERT(output[2].Name_=="tangent");
	xr_vector<float>&	o_binormal	= output[3].floatVector_;	R_ASSERT(output[3].Name_=="binormal");
	xr_vector<float>&	o_tc		= output[4].floatVector_;	R_ASSERT(output[4].Name_=="tex0");
	xr_vector<float>&	o_color		= output[5].floatVector_;	R_ASSERT(output[5].Name_=="clr");
	xr_vector<int>&		o_indices	= output[6].intVector_;		R_ASSERT(output[6].Name_=="indices");

	// verify
	R_ASSERT		(3*data.faces.size()	== o_indices.size());
	u32 v_cnt		= o_position.size();
	R_ASSERT		(0==v_cnt%3);
	R_ASSERT		(v_cnt == o_normal.size());
	R_ASSERT		(v_cnt == o_tangent.size());
	R_ASSERT		(v_cnt == o_binormal.size());
	R_ASSERT		(v_cnt == o_tc.size());
	R_ASSERT		(v_cnt == o_color.size());


	retrive_data_from_mender_otput( data.vertices, data.faces, o_position, o_normal, o_tc, o_tangent,  o_binormal, o_color, o_indices );
}

