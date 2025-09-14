#ifndef	SKIN_H
#define SKIN_H

#include "common.h"
//RoH & SM+
struct 	v_model_skinned_0
{
	float4 	P	; // POSITION;	// (float,float,float,1) - quantized	// short4
	float3	N	; // NORMAL;	// normal				// DWORD
	float3	T	; // TANGENT;	// tangent				// DWORD
	float3	B	; // BINORMAL;	// binormal				// DWORD
	float2	tc	; // TEXCOORD0;// (u,v)				// short2
};
struct 	v_model_skinned_1   		// 24 bytes
{
	float4 	P	; // POSITION;	// (float,float,float,1) - quantized	// short4
	float4	N	; // NORMAL;	// (nx,ny,nz,index)			// DWORD
	float3	T	; // TANGENT;	// tangent				// DWORD
	float3	B	; // BINORMAL;	// binormal				// DWORD
	float2	tc	; // TEXCOORD0;// (u,v)				// short2
};
struct 	v_model_skinned_2		// 28 bytes
{
	float4 	P	; // POSITION;	// (float,float,float,1) - quantized	// short4
	float4 	N	; // NORMAL;	// (nx,ny,nz,weight)	// DWORD
	float3	T	; // TANGENT;	// tangent				// DWORD
	float3	B	; // BINORMAL;	// binormal				// DWORD
	float4 	tc	; // TEXCOORD0;	// (u,v, w=m-index0, z=m-index1)  	// short4
};

struct 	v_model_skinned_3		// 28 bytes
{
	float4 	P	; // POSITION;	// (float,float,float,1) - quantized	// short4
	float4 	N	; // NORMAL;	// (nx,ny,nz,weight0)			// DWORD
	float4	T	; // TANGENT;	// (tx,ty,tz,weight1)				// DWORD
	float4	B	; // BINORMAL;	// (bx,by,bz,m-index2)				// DWORD
	float4 	tc	; // TEXCOORD0;	// (u,v, w=m-index0, z=m-index1)  	// short4
};

struct 	v_model_skinned_4		// 28 bytes
{
	float4 	P	; // POSITION;	// (float,float,float,1) - quantized	// short4
	float4 	N	; // NORMAL;	// (nx,ny,nz,weight0)			// DWORD
	float4	T	; // TANGENT;	// (tx,ty,tz,weight1)				// DWORD
	float4	B	; // BINORMAL;	// (bx,by,bz,weight2)				// DWORD
	float2 	tc	; // TEXCOORD0;// (u,v)  					// short2
	float4 	ind	; // TEXCOORD1;// (x=m-index0, y=m-index1, z=m-index2, w=m-index3)  	// DWORD
};

//////////////////////////////////////////////////////////////////////////////////////////

float4 	u_position	(float4 v)	{ return float4(v.xyz, 1.0);	}	// -12..+12 //--#SM+#--

//////////////////////////////////////////////////////////////////////////////////////////
//uniform float4 	sbones_array	[256-22] : register(vs,c22);
//tbuffer	SkeletonBones
//{
uniform float4 	sbones_array	[256-22];
//}

float3 	skinning_dir 	(float3 dir, float4 m0, float4 m1, float4 m2)
{
	float3 	U 	= unpack_normal	(dir);
	return 	float3	
		(
			dot	(m0.xyz, U),
			dot	(m1.xyz, U),
			dot	(m2.xyz, U)
		);
}
float4 	skinning_pos 	(float4 pos, float4 m0, float4 m1, float4 m2)
{
	float4 	P	= u_position(pos); // -12..+12 //--#SM+#--
	return 	float4
		(
			dot	(m0, P),
			dot	(m1, P),
			dot	(m2, P),
			1
		);
}

v_model skinning_0	(v_model_skinned_0	v)
{
	//	Swizzle for D3DCOLOUR format
	v.N			= v.N.zyx;
	v.T			= v.T.zyx;
	v.B			= v.B.zyx;

	// skinning
	v_model 	o;
	o.P 		= u_position(v.P); // -12..+12 //--#SM+#--
	o.N 		= unpack_normal(v.N);
	o.T 		= unpack_normal(v.T);
	o.B 		= unpack_normal(v.B);
	o.tc 		= v.tc; // -16..+16 //--#SM+#--
	return o;
}
v_model skinning_1 	(v_model_skinned_1	v)
{
	//	Swizzle for D3DCOLOUR format
	v.N.xyz		= v.N.zyx;
	v.T.xyz		= v.T.zyx;
	v.B.xyz		= v.B.zyx;

	// matrices
	int 	mid = int(v.N.w * 255 + 0.3);
	float4  m0 	= sbones_array[mid+0];
	float4  m1 	= sbones_array[mid+1];
	float4  m2 	= sbones_array[mid+2];

	// skinning
	v_model 	o;
	o.P 		= skinning_pos(v.P, m0,m1,m2 );
	o.N 		= skinning_dir(v.N.xyz, m0,m1,m2 );
	o.T 		= skinning_dir(v.T.xyz, m0,m1,m2 );
	o.B 		= skinning_dir(v.B.xyz, m0,m1,m2 );
	o.tc 		= v.tc;		// -16..+16 //--#SM+#--
	return o;
}
v_model skinning_2 	(v_model_skinned_2	v)
{
	//	Swizzle for D3DCOLOUR format
	v.N.xyz		= v.N.zyx;
	v.T.xyz		= v.T.zyx;
	v.B.xyz		= v.B.zyx;

	// matrices
	int 	id_0 	= int(v.tc.z);
	float4  m0_0 	= sbones_array[id_0+0];
	float4  m1_0 	= sbones_array[id_0+1];
	float4  m2_0 	= sbones_array[id_0+2];
	int 	id_1 	= int(v.tc.w);
	float4  m0_1 	= sbones_array[id_1+0];
	float4  m1_1 	= sbones_array[id_1+1];
	float4  m2_1 	= sbones_array[id_1+2];

	// lerp
	float 	w 	= v.N.w;
	float4  m0 	= lerp(m0_0,m0_1,w);
	float4  m1 	= lerp(m1_0,m1_1,w);
	float4  m2 	= lerp(m2_0,m2_1,w);

	// skinning
	v_model 	o;
	o.P 		= skinning_pos(v.P, m0,m1,m2 );
	o.N 		= skinning_dir(v.N.xyz, m0,m1,m2 );
	o.T 		= skinning_dir(v.T.xyz, m0,m1,m2 );
	o.B 		= skinning_dir(v.B.xyz, m0,m1,m2 );
	o.tc 		= v.tc.xy;	// -16..+16 //--#SM+#--
	return o;
}
v_model skinning_3 	(v_model_skinned_3	v)
{
	//	Swizzle for D3DCOLOUR format
	v.N.xyz		= v.N.zyx;
	v.T.xyz		= v.T.zyx;
	v.B.xyz		= v.B.zyx;

	// matrices
	int 	id_0 	= int(v.tc.z);
	float4  m0_0 	= sbones_array[id_0+0];
	float4  m1_0 	= sbones_array[id_0+1];
	float4  m2_0 	= sbones_array[id_0+2];
	int 	id_1 	= int(v.tc.w);
	float4  m0_1 	= sbones_array[id_1+0];
	float4  m1_1 	= sbones_array[id_1+1];
	float4  m2_1 	= sbones_array[id_1+2];
	int 	id_2 	= int(v.B.w*255+0.3);
	float4  m0_2 	= sbones_array[id_2+0];
	float4  m1_2 	= sbones_array[id_2+1];
	float4  m2_2 	= sbones_array[id_2+2];

	// lerp
	float 	w0 	= v.N.w;
	float 	w1 	= v.T.w;
	float 	w2 	= 1-w0-w1;
	float4  m0 	= m0_0*w0;
	float4  m1 	= m1_0*w0;
	float4  m2 	= m2_0*w0;

			m0 	+= m0_1*w1;
			m1 	+= m1_1*w1;
			m2 	+= m2_1*w1;

			m0 	+= m0_2*w2;
			m1 	+= m1_2*w2;
			m2 	+= m2_2*w2;

	// skinning
	v_model 	o;
	o.P 		= skinning_pos(v.P, m0,m1,m2 );
	o.N 		= skinning_dir(v.N.xyz, m0,m1,m2 );
	o.T 		= skinning_dir(v.T.xyz, m0,m1,m2 );
	o.B 		= skinning_dir(v.B.xyz, m0,m1,m2 );
	o.tc 		= v.tc.xy;	// -16..+16 //--#SM+#--
#ifdef SKIN_COLOR
	o.rgb_tint	= float3	(2,0,0)	;
	if (id_0==id_1)	o.rgb_tint	= float3(1,2,0);
#endif
	return o;
}
v_model skinning_4 	(v_model_skinned_4	v)
{
	//	Swizzle for D3DCOLOUR format
	v.N.xyz		= v.N.zyx;
	v.T.xyz		= v.T.zyx;
	v.B.xyz		= v.B.zyx;
	v.ind.xyz	= v.ind.zyx;

	// matrices
	int		id[4];
	mat3x4	m[4];	//	[bone index][matrix row or column???]

	for (int i=0; i<4; ++i)
	{		
		id[i] = int(v.ind[i]*255+0.3);

		for (int j=0; j<3; ++j)
			m[i][j] = sbones_array[id[i]+j];
	}

	// lerp
	float	w[4];
	w[0] 	= v.N.w;
	w[1] 	= v.T.w;
	w[2] 	= v.B.w;
	w[3]	= 1-w[0]-w[1]-w[2];

	float4  m0 	= m[0][0]*w[0];
	float4  m1 	= m[0][1]*w[0];
	float4  m2 	= m[0][2]*w[0];

	for (int i=1; i<4; ++i)
	{
		m0 	+= m[i][0]*w[i];
		m1 	+= m[i][1]*w[i];
		m2 	+= m[i][2]*w[i];
	}

	// skinning
	v_model 	o;
	o.P 		= skinning_pos(v.P, m0,m1,m2 );
	o.N 		= skinning_dir(v.N.xyz, m0,m1,m2 );
	o.T 		= skinning_dir(v.T.xyz, m0,m1,m2 );
	o.B 		= skinning_dir(v.B.xyz, m0,m1,m2 );
	o.tc 		= v.tc;	// -16..+16 //--#SM+#--

	return o;
}

#endif
