#include	"stdafx.h"

#include	"NvMender2002/nvMeshMender.h"


int find_same_vertex( const xr_vector<u32>& m, const Fvector2	Ftc, const xr_vector<float>& v_tc )
{
	// Search
	for (u32 it=0; it<m.size(); it++)
	{
		u32		m_id		= m[it];
		const float*	tc	= &*v_tc.begin()+m_id*3;
		if (!fsimilar(tc[0],Ftc.x))	
			continue;
		if (!fsimilar(tc[1],Ftc.y))	
			continue;
		return int(m_id);
	}
	return -1;
}

u32 add_vertex(	 const	Vertex		&V,
				 const	Fvector2	&Ftc,
				 xr_vector<float>	&v_position,
			     xr_vector<float>	&v_normal,	
			     xr_vector<float>	&v_tc//,
			    // xr_vector<int>		&v_indices
			     )
{
	u32		m_id	= v_position.size()/3;

	v_position.push_back(V.P.x);	
	v_position.push_back(V.P.y);	
	v_position.push_back(V.P.z);

	v_normal.push_back(V.N.x);
	v_normal.push_back(V.N.y);
	v_normal.push_back(V.N.z);

	v_tc.push_back(Ftc.x);
	v_tc.push_back(Ftc.y);
	v_tc.push_back(0);

	//v_indices.push_back(m_id);
	return m_id;
}


void	fill_mender_input( xr_vector<float>	&v_position,
						   xr_vector<float>	&v_normal,	
						   xr_vector<float>	&v_tc,
						   xr_vector<int>	&v_indices
						   )
{
		// ************************************* Build vectors + expand TC if nessesary
	Status						("Building inputs...");
	std::sort					(g_vertices.begin(),g_vertices.end());
	xr_vector<xr_vector<u32> >	remap;
	remap.resize				(g_vertices.size());
	for (u32 f=0; f<g_faces.size(); f++)
	{
		Progress	(float(f)/float(g_faces.size()));
		Face*		F	= g_faces[f];
		for (u32 v=0; v<3; v++)
		{
			Vertex			*V	= F->v[v];	
			u32				ID	= u32(std::lower_bound(g_vertices.begin(),g_vertices.end(),V)-g_vertices.begin());
			xr_vector<u32>	&m	= remap[ID];
			Fvector2		Ftc = F->tc.front().uv[v];

			int vertex_index	= find_same_vertex( m, Ftc, v_tc );
			
			// Register new if not found
			if ( vertex_index == (-1) )
			{
				vertex_index =	add_vertex(*V, Ftc, v_position, v_normal, v_tc );
				remap[ID].push_back( vertex_index );
			}
	
			v_indices	.push_back		(vertex_index);

		}
	}
	remap.clear	();
}

void retrive_data_from_mender_otput( const	xr_vector<float>	&o_tc,		
									 const 	xr_vector<float>	&o_tangent,	
									 const	xr_vector<float>	&o_binormal,
									 const 	xr_vector<int>		&o_indices	)

{
	// ************************************* Retreive data
	Status						("Retreiving basis...");
	for (u32 f=0; f<g_faces.size(); f++)
	{
		Face*	F				= g_faces[f];
		for (u32 v=0; v<3; v++)
		{
			u32	id							=	o_indices	[f*3+v];	// vertex index
			R_ASSERT						(id*3 < o_tc.size());
			F->tc.front().uv[v].set			(o_tc		[id*3+0],	o_tc		[id*3+1]);
			F->basis_tangent[v].set			(o_tangent	[id*3+0],	o_tangent	[id*3+1],	o_tangent	[id*3+2]);
			F->basis_binormal[v].set		(o_binormal	[id*3+0],	o_binormal	[id*3+1],	o_binormal	[id*3+2]);
		}
	}
}

void CBuild::xrPhase_TangentBasis()
{
	// ************************************* Correct single-degenerates
	/*
	float	teps			= .5f / 4096.f;		// half pixel from 4096 texture (0.0001220703125)
	for (u32 f=0; f<g_faces.size(); f++)
	{
		Progress	(float(f)/float(g_faces.size()));
		Face*		F		= g_faces[f];
		Fvector2&	tc0		= F->tc.front().uv[0];
		Fvector2&	tc1		= F->tc.front().uv[1];
		Fvector2&	tc2		= F->tc.front().uv[2];
		float		e01		= tc0.distance_to(tc1);	
		float		e12		= tc1.distance_to(tc2);
		float		e20		= tc2.distance_to(tc0);
		if (e01<teps)
		{
			Fvector2	d0,d1,d,r;
			d0.sub		(tc0,tc2);	d0.norm	();
			d1.sub		(tc1,tc2);	d1.norm	();
			d.averageA	(d0,d1);	d.norm	();
			r.cross		(d);		r.mul	(teps);	// right
			tc1.add		(r);		tc0.sub	(r);	// correction
		} else if (e12<teps) {
			Fvector2	d1,d2,d,r;
			d0.sub		(tc0,tc2);	d0.norm	();
			d1.sub		(tc1,tc2);	d1.norm	();
			d.averageA	(d0,d1);	d.norm	();
			r.cross		(d);		r.mul	(teps);	// right
			tc1.add		(r);		tc0.sub	(r);	// correction
		}
	}
	*/

	// ************************************* Declare inputs
	Status						("Declarator...");
	u32 v_count_reserve			= iFloor(float(g_vertices.size())*1.33f);
	u32 i_count_reserve			= 3*g_faces.size();

	xr_vector<NVMeshMender::VertexAttribute> input,output;
	input.push_back	(NVMeshMender::VertexAttribute());	// pos
	input.push_back	(NVMeshMender::VertexAttribute());	// norm
	input.push_back	(NVMeshMender::VertexAttribute());	// tex0
	input.push_back	(NVMeshMender::VertexAttribute());	// *** faces

	input[0].Name_= "position";	xr_vector<float>&	v_position	= input[0].floatVector_;	v_position.reserve	(v_count_reserve);
	input[1].Name_= "normal";	xr_vector<float>&	v_normal	= input[1].floatVector_;	v_normal.reserve	(v_count_reserve);
	input[2].Name_= "tex0";		xr_vector<float>&	v_tc		= input[2].floatVector_;	v_tc.reserve		(v_count_reserve);
	input[3].Name_= "indices";	xr_vector<int>&		v_indices	= input[3].intVector_;		v_indices.reserve	(i_count_reserve);

	output.push_back(NVMeshMender::VertexAttribute());	// tex0
	output.push_back(NVMeshMender::VertexAttribute());	// tangent
	output.push_back(NVMeshMender::VertexAttribute());	// binormal
	output.push_back(NVMeshMender::VertexAttribute());	// *** faces
	output.push_back(NVMeshMender::VertexAttribute());	// position, needed for mender

	output[0].Name_= "tex0";	
	output[1].Name_= "tangent";	
	output[2].Name_= "binormal";
	output[3].Name_= "indices";	
	output[4].Name_= "position";

	fill_mender_input( v_position, v_normal, v_tc, v_indices );

	u32			v_was	= g_vertices.size();
	u32			v_become= v_position.size()/3;
	clMsg		("duplication: was[%d] / become[%d] - %2.1f%%",v_was,v_become,100.f*float(v_become-v_was)/float(v_was));

	// ************************************* Perform mungle
	Status			("Calculating basis...");
	NVMeshMender	mender	;
	if (!mender.Munge		(
		input,										// input attributes
		output,										// outputs attributes
		0,//deg2rad(61.f),								// tangent space smooth angle
		//deg2rad(61.f)
		0,											// no texture matrix applied to my texture coordinates
		NVMeshMender::FixTangents,					// fix degenerate bases & texture mirroring
		NVMeshMender::DontFixCylindricalTexGen,		// handle cylindrically mapped textures via vertex duplication
		NVMeshMender::DontWeightNormalsByFaceSize	// weigh vertex normals by the triangle's size
		))
	{
		Debug.fatal	(DEBUG_INFO,"NVMeshMender failed (%s)",mender.GetLastError().c_str());
	}

	// ************************************* Bind declarators
	// bind
	xr_vector<float>&	o_tc		= output[0].floatVector_;	R_ASSERT(output[0].Name_=="tex0");
	xr_vector<float>&	o_tangent	= output[1].floatVector_;	R_ASSERT(output[1].Name_=="tangent");
	xr_vector<float>&	o_binormal	= output[2].floatVector_;	R_ASSERT(output[2].Name_=="binormal");
	xr_vector<int>&		o_indices	= output[3].intVector_;		R_ASSERT(output[3].Name_=="indices");

	// verify
	R_ASSERT		(3*g_faces.size()	== o_indices.size());
	R_ASSERT		(o_tc.size()		== o_tangent.size());
	R_ASSERT		(o_tangent.size()	== o_binormal.size());
	R_ASSERT		(o_tc.size()		>= v_tc.size());

	retrive_data_from_mender_otput( o_tc, o_tangent, o_binormal, o_indices );
}

