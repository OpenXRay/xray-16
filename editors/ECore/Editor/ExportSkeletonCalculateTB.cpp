
//----------------------------------------------------
// file: ExportSkeleton.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#ifndef	_EDITOR
//
#include "../../xrEngine/defines.h"
//
#include "../../xrCore/xrCore.h"

#include "../../../Layers/xrRender/hwcaps.h"
#include "../../../Layers/xrRender/hw.h"
#include "../../../xrEngine/pure.h"
class CGameFont;
#include "..\..\include\xrRender\drawutils.h"
#include "..\..\Layers\xrRender\xrD3dDefs.h"
#include "..\..\Layers\xrRender\shader.h"
#include "..\..\Layers\xrRender\R_Backend.h"

#include "..\..\xrEngine\fmesh.h"
#include "..\..\xrEngine\_d3d_extensions.h"
#include "..\..\xrEngine\properties.h"

//


DEFINE_VECTOR(FVF::L,FLvertexVec,FLvertexIt);
DEFINE_VECTOR(FVF::TL,FTLvertexVec,FTLvertexIt);
DEFINE_VECTOR(FVF::LIT,FLITvertexVec,FLITvertexIt);
DEFINE_VECTOR(shared_str,RStrVec,RStrVecIt);

#endif


#include "ExportSkeleton.h"

#include "../engine/nvMeshMender.h"
/*
void CExportSkeleton::SSplit::CalculateTB()
{
	u32 v_count_reserve			= 3*iFloor(float(m_Verts.size())*1.33f);
	u32 i_count_reserve			= 3*m_Faces.size();

	// Declare inputs
	xr_vector<NVMeshMender::VertexAttribute> 			input;
	input.push_back	(NVMeshMender::VertexAttribute());	// pos
	input.push_back	(NVMeshMender::VertexAttribute());	// norm
	input.push_back	(NVMeshMender::VertexAttribute());	// tex0
	input.push_back	(NVMeshMender::VertexAttribute());	// c_w0_b0
	input.push_back	(NVMeshMender::VertexAttribute());	// w1_w2_w3
	input.push_back	(NVMeshMender::VertexAttribute());	// b1_b2_b3
	input.push_back	(NVMeshMender::VertexAttribute());	// *** faces

    int i_id=-1;
	input[++i_id].Name_= "position";	xr_vector<float>&	i_position	= input[i_id].floatVector_;	i_position.reserve	(v_count_reserve);
	input[++i_id].Name_= "normal";		xr_vector<float>&	i_normal	= input[i_id].floatVector_;	i_normal.reserve	(v_count_reserve);
	input[++i_id].Name_= "tex0";		xr_vector<float>&	i_tc		= input[i_id].floatVector_;	i_tc.reserve		(v_count_reserve);
	input[++i_id].Name_= "c_w0_b0";  	xr_vector<float>&	i_c_w0_b0	= input[i_id].floatVector_;	i_c_w0_b0.reserve	(v_count_reserve);
	input[++i_id].Name_= "w1_w2_w3";	xr_vector<float>&	i_w1_w2_w3	= input[i_id].floatVector_;	i_w1_w2_w3.reserve	(v_count_reserve);
	input[++i_id].Name_= "b1_b2_b3";	xr_vector<float>&	i_b1_b2_b3	= input[i_id].floatVector_;	i_b1_b2_b3.reserve	(v_count_reserve);
	input[++i_id].Name_= "indices";		xr_vector<int>&		i_indices	= input[i_id].intVector_;	i_indices.reserve	(i_count_reserve);

	// Declare outputs
	xr_vector<NVMeshMender::VertexAttribute> 			output;
	output.push_back(NVMeshMender::VertexAttribute());	// position, needed for mender
	output.push_back(NVMeshMender::VertexAttribute());	// normal
	output.push_back(NVMeshMender::VertexAttribute());	// tangent
	output.push_back(NVMeshMender::VertexAttribute());	// binormal
	output.push_back(NVMeshMender::VertexAttribute());	// tex0
	output.push_back(NVMeshMender::VertexAttribute());	// w0_w1_w2
	output.push_back(NVMeshMender::VertexAttribute());	// b0_b1_b2
	output.push_back(NVMeshMender::VertexAttribute());	// w3_b3
	output.push_back(NVMeshMender::VertexAttribute());	// *** faces

    int o_id=-1;
	output[++o_id].Name_= "position";	xr_vector<float>&	o_position	= output[o_id].floatVector_;
	output[++o_id].Name_= "normal";		xr_vector<float>&	o_normal	= output[o_id].floatVector_;
	output[++o_id].Name_= "tangent";	xr_vector<float>&	o_tangent	= output[o_id].floatVector_;
	output[++o_id].Name_= "binormal";	xr_vector<float>&	o_binormal	= output[o_id].floatVector_;
	output[++o_id].Name_= "tex0";		xr_vector<float>&	o_tc		= output[o_id].floatVector_;
	output[++o_id].Name_= "c_w0_b0";	xr_vector<float>&	o_c_w0_b0	= output[o_id].floatVector_;
	output[++o_id].Name_= "w1_w2_w3";	xr_vector<float>&	o_w1_w2_w3	= output[o_id].floatVector_;
	output[++o_id].Name_= "b1_b2_b3";	xr_vector<float>&	o_b1_b2_b3	= output[o_id].floatVector_;
	output[++o_id].Name_= "indices";	xr_vector<int>&		o_indices	= output[o_id].intVector_;

    // fill inputs (verts&indices)
    for (SkelVertIt vert_it=m_Verts.begin(); vert_it!=m_Verts.end(); vert_it++){
        SSkelVert	&iV = *vert_it;
        i_position.push_back(iV.offs.x);	    	    i_position.push_back(iV.offs.y);	   			i_position.push_back(iV.offs.z);
        i_normal.push_back	(iV.norm.x);  	    	    i_normal.push_back	(iV.norm.y);	    		i_normal.push_back	(iV.norm.z);
        i_tc.push_back		(iV.uv.x);	    		    i_tc.push_back		(iV.uv.y);	    			i_tc.push_back		(0);
        u32 sz				= iV.bones.size();
        i_c_w0_b0.push_back	(*(float*)&sz);				i_c_w0_b0.push_back(iV.bones[0].w);   			i_c_w0_b0.push_back	(*(float*)&iV.bones[0].id);
        u32 pt_w			= i_w1_w2_w3.size();
        u32 pt_b			= i_b1_b2_b3.size();
        i_w1_w2_w3.push_back(0.f);	i_w1_w2_w3.push_back(0.f);	i_w1_w2_w3.push_back(0.f);
        i_b1_b2_b3.push_back(0.f);	i_b1_b2_b3.push_back(0.f);	i_b1_b2_b3.push_back(0.f);
        if (iV.bones.size()>1){
            i_w1_w2_w3[pt_w+0]			= iV.bones[1].w;
            i_b1_b2_b3[pt_b+0]			= (*(float*)&iV.bones[1].id);	
	        if (iV.bones.size()>2){
    	        i_w1_w2_w3[pt_w+1] 		= iV.bones[2].w;
        	    i_b1_b2_b3[pt_b+1] 		= (*(float*)&iV.bones[2].id);	
		        if (iV.bones.size()>3){
        		    i_w1_w2_w3[pt_w+2]	= iV.bones[3].w;
		            i_b1_b2_b3[pt_b+2] 	= (*(float*)&iV.bones[3].id);
                }
            }
        }
    }
    for (SkelFaceIt face_it=m_Faces.begin(); face_it!=m_Faces.end(); face_it++){
        SSkelFace	&iF = *face_it;
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

	// verify
	R_ASSERT		(3*m_Faces.size()	== o_indices.size());
    u32 v_cnt		= o_position.size();
    R_ASSERT		(0==v_cnt%3);
    R_ASSERT		(v_cnt == o_normal.size());
    R_ASSERT		(v_cnt == o_tangent.size());
    R_ASSERT		(v_cnt == o_binormal.size());
    R_ASSERT		(v_cnt == o_tc.size());
    R_ASSERT		(v_cnt == o_w1_w2_w3.size());
    R_ASSERT		(v_cnt == o_b1_b2_b3.size());
    R_ASSERT		(v_cnt == o_c_w0_b0.size());
    v_cnt			/= 3;

    // retriving data
    u32 o_idx		= 0;
    for (face_it=m_Faces.begin(); face_it!=m_Faces.end(); face_it++){
        SSkelFace	&iF = *face_it;
        iF.v[0]		= (u16)o_indices[o_idx++];
        iF.v[1]		= (u16)o_indices[o_idx++];
        iF.v[2]		= (u16)o_indices[o_idx++];
    }
    m_Verts.clear	(); m_Verts.resize(v_cnt);
    for (u32 v_idx=0; v_idx!=v_cnt; v_idx++){
        SSkelVert	&oV = m_Verts[v_idx];
        oV.offs.set	(o_position[v_idx*3+0],	o_position[v_idx*3+1],	o_position[v_idx*3+2]);
        oV.norm.set	(o_normal[v_idx*3+0],	o_normal[v_idx*3+1],	o_normal[v_idx*3+2]);
        oV.tang.set	(o_tangent[v_idx*3+0],	o_tangent[v_idx*3+1],	o_tangent[v_idx*3+2]);
        oV.binorm.set(o_binormal[v_idx*3+0],o_binormal[v_idx*3+1],	o_binormal[v_idx*3+2]);
        oV.uv.set	(o_tc[v_idx*3+0],		o_tc[v_idx*3+1]);
        oV.bones.resize	(*(u32*)&o_c_w0_b0 [v_idx*3+0]);

        oV.bones[0].w	= o_c_w0_b0 [v_idx*3+1];oV.bones[0].id		= (u16)(*(u32*)&o_c_w0_b0 	[v_idx*3+2]);
        if (oV.bones.size()>1){
	        oV.bones[1].w	= o_w1_w2_w3[v_idx*3+0];
            oV.bones[1].id	= (u16)(*(u32*)&o_b1_b2_b3	[v_idx*3+0]);
            if (oV.bones.size()>2){
                oV.bones[2].w	= o_w1_w2_w3[v_idx*3+1];
                oV.bones[2].id	= (u16)(*(u32*)&o_b1_b2_b3	[v_idx*3+1]);
                if (oV.bones.size()>3){
                    oV.bones[3].w	= o_w1_w2_w3[v_idx*3+2];
                    oV.bones[3].id	= (u16)(*(u32*)&o_b1_b2_b3	[v_idx*3+2]);
                }
            }
        }
    }

 
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////////// 

/////////////////////////////////////////////////////////////////////////////////////////////////////
#include "../../common/nvMender2003/nvmeshmender.h"
#include "../../common/NvMender2003/nvMeshMender.h"
#include "../../common/NvMender2003/mender_input_output.h"
#include "../../common/NvMender2003/remove_isolated_verts.h"

void 	CExportSkeleton::SSplit::OptimizeTextureCoordinates()
{
	// Optimize texture coordinates
    // 1. Calc bounds
    Fvector2 	Tdelta;
    Fvector2 	Tmin,Tmax;
    Tmin.set	(flt_max,flt_max);
    Tmax.set	(flt_min,flt_min);

	u32	v_cnt	= m_Verts.size();

    for ( u32 v_idx=0; v_idx!=v_cnt; v_idx++ ){
        SSkelVert	&iV = m_Verts[v_idx];
        Tmin.min	(iV.uv);
        Tmax.max	(iV.uv);
    }
    Tdelta.x 	= floorf((Tmax.x-Tmin.x)/2+Tmin.x);
    Tdelta.y 	= floorf((Tmax.y-Tmin.y)/2+Tmin.y);

    Fvector2	Tsize;
    Tsize.sub	(Tmax,Tmin);
    if ((Tsize.x>32)||(Tsize.y>32))
    	Msg		("#!Surface [T:'%s', S:'%s'] has UV tiled more than 32 times.",*m_Texture,*m_Shader);
    {
      // 2. Recalc UV mapping
      for ( u32 v_idx=0; v_idx!=v_cnt; v_idx++ ){
          SSkelVert	&iV = m_Verts[v_idx];
          iV.uv.sub	(Tdelta);
      }
    }
}

IC void	set_vertex( MeshMender::Vertex &out_vertex, const SSkelVert& in_vertex )
{
			cv_vector( out_vertex.pos,		in_vertex.offs );
			cv_vector( out_vertex.normal,	in_vertex.norm );
			out_vertex.s		= in_vertex.uv.x;
			out_vertex.t		= in_vertex.uv.y;
			//out_vertex.tangent;
			//out_vertex.binormal;
}

IC void	set_vertex( SSkelVert& out_vertex,  const SSkelVert& in_old_vertex, const MeshMender::Vertex &in_vertex )
{
			out_vertex = in_old_vertex;

			cv_vector( out_vertex.offs, in_vertex.pos );//?
			cv_vector( out_vertex.norm, in_vertex.normal );//?

			out_vertex.uv.x	= in_vertex.s;
			out_vertex.uv.y	= in_vertex.t;
			Fvector tangent; Fvector binormal;
			out_vertex.tang.set( cv_vector( tangent, in_vertex.tangent ) );
			out_vertex.binorm.set( cv_vector( binormal, in_vertex.binormal ) );
}


IC u16	&face_vertex( SSkelFace &F, u32 vertex_index )
{
	VERIFY( vertex_index < 3 );
	return F.v[vertex_index];
}

IC const u16 &face_vertex( const SSkelFace &F, u32 vertex_index )
{
	VERIFY( vertex_index < 3 );
	return F.v[vertex_index];
}





void 	CExportSkeleton::SSplit::CalculateTB	()
{
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