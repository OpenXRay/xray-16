/*
	Functions below is necessary for g-buffer optimization.
	Vanilla buffer:
	RT0 - D3DFMT_A16B16G16R16F, xyz - view-space position, w - material
	RT1 - D3DFMT_A16B16G16R16F, xyz - view_space normal, w - hemi
	RT2 - D3DFMT_A8R8G8B8/D3DFMT_A16B16G16R16F*, xyz - albedo color, w - gloss
	
	Buffer for optimizing:
	RT0 - D3DFMT_R16F, single channel - view-space linear depth
	RT1 - D3DFMT_A16B16G16R16F, xy - normal, compressed by spheremap transform, z - material, w - hemi
	RT2 - D3DFMT_A8R8G8B8/D3DFMT_A16B16G16R16F*, xyz - albedo color, w - gloss

	Buffer for using with reflections:
	RT0 - D3DFMT_G16R16F, x - view-space linear depth, y - reflections power
	RT1 - D3DFMT_A16B16G16R16F, xy - normal, compressed by spheremap transform, z - material, w - hemi
	RT2 - D3DFMT_A8R8G8B8/D3DFMT_A16B16G16R16F*, xyz - albedo color, w - gloss
	
* - depending on videoadapter hardware capabilities
	New buffer is smaller, than old one. Despite there are more ALU instructions for decoding & encoding position and normal I observe higher FPS.
	Also, there is a space for storing velocity map now.
*/
#ifndef OGSE_GBUFFER_H
#define OGSE_GBUFFER_H
uniform float4 ogse_pos_decompression; // x - fFOV, y - fAspect, z - Zf/(Zf-Zn), w - Zn*tan(fFov/2)
uniform float4x4 m_InvP;
#if /*defined(USE_GBUFFER_OPTIMIZATION) || */defined(USE_WET_SURFACES)
struct GB
{
	float4	_rt1	:	COLOR0;
	float4	_rt2	:	COLOR1;
	float4	_rt3	:	COLOR2;
};
	float2	encode_normal			(float3 _N)	{return (_N.xy/sqrt(_N.z*8+8) + 0.5);}
/*	GB		make_gbuffer	(float3 _P, float3 _N, float3 _D, float4 params)	// params: x - hemi, y - mat, z - gloss, w - refl
	{
		GB _out;
		_out._rt1	= float4(_P.z, params.yyy);
		_out._rt2	= float4(encode_normal(_N), params.wx);
		_out._rt3	= float4(_D.xyz, params.z);
		return _out;
	}
	float	get_material			(float2 tc)	{return tex2D(s_position, tc).y;}
	float3	get_xyz_position		(float2 tc)
	{
		float3 view;
		view.z = tex2D(s_position, tc).x;
		view.xy = screen_to_proj(tc, view.z).xy;
		view.xy *= float2(ogse_c_screen.z / ogse_c_screen.y, ogse_c_screen.z) * view.z;
//		view.xy *= (float2(1,1) + float2(ogse_c_screen.z / ogse_c_screen.y, ogse_c_screen.z) * view.z);// saturate(view.z - 0.2)* ogse_c_screen.w/( ogse_c_screen.w-0.2));
		return view;
	}
	float3	get_xyz_position_fast	(float2 tc)
	{
		float3 view;
		view.z = tex2Dlod(s_position, float4(tc,0,0)).x;
		view.xy = screen_to_proj(tc, view.z).xy;
		view.xy *= float2(ogse_c_screen.z / ogse_c_screen.y, ogse_c_screen.z) * view.z;
//		view.xy *= (float2(1,1) + float2(ogse_c_screen.z / ogse_c_screen.y, ogse_c_screen.z) * view.z);// saturate(view.z - 0.2)* ogse_c_screen.w/( ogse_c_screen.w-0.2));
		return view;
	}
	float3	get_xyz_position_proj	(float4 tc)
	{
		float3 view;
		view.z = tex2Dproj(s_position, tc).x;
		view.xy = screen_to_proj(tc.xy/tc.w, view.z).xy;
		view.xy *= float2(ogse_c_screen.z / ogse_c_screen.y, ogse_c_screen.z) * view.z;
//		view.xy *= (float2(1,1) + float2(ogse_c_screen.z / ogse_c_screen.y, ogse_c_screen.z) * view.z);// saturate(view.z - 0.2)* ogse_c_screen.w/( ogse_c_screen.w-0.2));
		return view;
	}
	float4	get_full_position		(float2 tc)
	{
		float4 pos;
		pos.xyz = get_xyz_position(tc);
		pos.w = get_material(tc);
		return pos;
	}
	float4	get_full_position_proj	(float4 tc)
	{
		float4 pos;
		pos.xyz = get_xyz_position_proj(tc);
		pos.w = get_material(tc);
		return pos;
	}
	float	get_depth				(float2 tc)	{return tex2D(s_position, tc).x;}
	float	get_depth_fast			(float2 tc)	{return tex2Dlod(s_position, float4(tc, 0, 0)).x;}
	float	get_depth_proj			(float4 tc)	{return tex2Dproj(s_position, tc).x;}*/
	GB		make_gbuffer	(float3 _P, float3 _N, float3 _D, float4 params)	// params: x - hemi, y - mat, z - gloss, w - refl
	{
		GB _out;
		_out._rt1	= float4(_P.xyz, params.y);
		_out._rt2	= float4(encode_normal(_N), params.wx);
		_out._rt3	= float4(_D.xyz, params.z);
		return _out;
	}
	float4	get_full_position		(float2 tc)	{return tex2D(s_position, tc);}
	float4	get_full_position_proj	(float4 tc)	{return tex2Dproj(s_position, tc);}
	float3	get_xyz_position		(float2 tc)	{return tex2D(s_position, tc).xyz;}
	float3	get_xyz_position_fast	(float2 tc)	{return tex2Dlod(s_position, float4(tc,0,0)).xyz;}
	float3	get_xyz_position_proj	(float4 tc)	{return tex2Dproj(s_position, tc).xyz;}
	float	get_material			(float2 tc)	{return tex2D(s_position, tc).w;}
	float	get_hemi				(float2 tc)	{return tex2D(s_normal, tc).w;}
	float	get_gloss				(float2 tc)	{return tex2D(s_diffuse, tc).w;}
	float	get_depth				(float2 tc)	{return tex2D(s_position, tc).z;}
	float	get_depth_fast			(float2 tc)	{return tex2Dlod(s_position, float4(tc,0,0)).z;}
	float	get_depth_proj			(float4 tc)	{return tex2Dproj(s_position, tc).z;}
	
	// NORMALS COMPRESSION
	float4	decode_normal			(float4 _N)
	{
		float2 enc = _N.xy;

		float2 fenc = enc*4-2;
		float f = dot(fenc,fenc);
		float g = sqrt(1-f/4);

		float4 N = _N;
   
		N.xy = fenc*g;
		N.z = 1-f/2;
		return N;
	}
	float4	get_full_normal			(float2 tc)	{return decode_normal(tex2D(s_normal, tc));}
	float4	get_full_normal_fast	(float2 tc)	{return decode_normal(tex2Dlod(s_normal, float4(tc,0,0)));}
	float4	get_full_normal_proj	(float4 tc)	{return decode_normal(tex2Dproj(s_normal, tc));}
	float3	get_xyz_normal			(float2 tc)	
	{
		float4 _N = get_full_normal(tc);
		return _N.xyz;
	}
	float3	get_xyz_normal_fast		(float2 tc)	
	{
		float4 _N = get_full_normal_fast(tc);
		return _N.xyz;
	}
	float3	get_xyz_normal_proj		(float4 tc)	
	{
		float4 _N = get_full_normal_proj(tc);
		return _N.xyz;
	}
	float	get_refl_power			(float2 tc)	{return tex2D(s_normal, tc).z;}
/*	
	float	get_material			(float2 tc)	{return tex2D(s_position, tc).y;}
	float3	get_xyz_position		(float2 tc)
	{
		float3 view;
		view.z = tex2D(s_position, tc).x;
		float4 proj = screen_to_proj(tc, view.z);
		view.xyz = float3(proj.x * ogse_c_screen.z / ogse_c_screen.y, proj.y * ogse_c_screen.z, 1.0) * proj.z;
		return view;
	}
	float3	get_xyz_position_fast	(float2 tc)
	{
		float3 view;
		view.z = tex2Dlod(s_position, float4(tc,0,0)).x;
		float4 proj = screen_to_proj(tc, view.z);
		view.xyz = float3(proj.x * ogse_c_screen.z / ogse_c_screen.y, proj.y * ogse_c_screen.z, 1.0) * proj.z;
		return view;
	}
	float3	get_xyz_position_proj	(float4 tc)
	{
		float3 view;
		view.z = tex2Dproj(s_position, tc).x;
		float4 proj = screen_to_proj(tc.xy/tc.w, view.z);
		view.xyz = float3(proj.x * ogse_c_screen.z / ogse_c_screen.y, proj.y * ogse_c_screen.z, 1.0) * proj.z;
		return view;
	}
	float4	get_full_position		(float2 tc)
	{
		float4 pos;
		pos.xyz = get_xyz_position(tc);
		pos.w = get_material(tc);
		return pos;
	}
	float4	get_full_position_proj	(float4 tc)
	{
		float4 pos;
		pos.xyz = get_xyz_position_proj(tc);
		pos.w = get_material(tc);
		return pos;
	}
	float	get_gloss				(float2 tc)	{return tex2D(s_diffuse, tc).w;}
	float	get_hemi				(float2 tc)	{return tex2D(s_normal, tc).w;}
	float	get_depth				(float2 tc)	{return tex2D(s_position, tc).x;}
	float	get_depth_fast			(float2 tc)	{return tex2Dlod(s_position, float4(tc, 0, 0)).x;}
	float	get_depth_proj			(float4 tc)	{return tex2Dproj(s_position, tc).x;}
	#ifdef USE_WET_SURFACES
		float	get_refl_power			(float2 tc)	{return tex2D(s_position, tc).z;}
		float4	encode_pos				(float3 _P, float _M, float _R)	{return float4(_P.z, _M, _R, _R);}
	#else
		float	get_refl_power			(float2 tc)	{return 0;}
		float4	encode_pos				(float3 _P, float _M, float _R)	{return float4(_P.z, _M, _M, _M);}
	#endif*/
#else
struct GB
{
	float4	_rt1	:	COLOR0;
	float4	_rt2	:	COLOR1;
	float4	_rt3	:	COLOR2;
};
	GB		make_gbuffer	(float3 _P, float3 _N, float3 _D, float4 params)	// params.x - hemi, y - mat, z - gloss, w - refl
	{
		GB _out;
		_out._rt1	= float4(_P.xyz, params.y);
		_out._rt2	= float4(_N.xyz, params.x);
		_out._rt3	= float4(_D.xyz, params.z);
		return _out;
	}
	float4	get_full_position		(float2 tc)	{return tex2D(s_position, tc);}
	float4	get_full_position_proj	(float4 tc)	{return tex2Dproj(s_position, tc);}
	float3	get_xyz_position		(float2 tc)	{return tex2D(s_position, tc).xyz;}
	float3	get_xyz_position_fast	(float2 tc)	{return tex2Dlod(s_position, float4(tc,0,0)).xyz;}
	float3	get_xyz_position_proj	(float4 tc)	{return tex2Dproj(s_position, tc).xyz;}
	float	get_material			(float2 tc)	{return tex2D(s_position, tc).w;}
	float	get_hemi				(float2 tc)	{return tex2D(s_normal, tc).w;}
	float	get_gloss				(float2 tc)	{return tex2D(s_diffuse, tc).w;}
	float	get_depth				(float2 tc)	{return tex2D(s_position, tc).z;}
	float	get_depth_fast			(float2 tc)	{return tex2Dlod(s_position, float4(tc,0,0)).z;}
	float	get_depth_proj			(float4 tc)	{return tex2Dproj(s_position, tc).z;}
		
	float4	get_full_normal			(float2 tc)	{return tex2D(s_normal, tc);}
	float4	get_full_normal_proj	(float4 tc)	{return tex2Dproj(s_normal, tc);}
	float3	get_xyz_normal			(float2 tc)	{return tex2D(s_normal, tc).xyz;}
	float3	get_xyz_normal_fast		(float2 tc)	{return tex2Dlod(s_normal, float4(tc,0,0)).xyz;}
	float3	get_xyz_normal_proj		(float4 tc)	{return tex2Dproj(s_normal, tc).xyz;}
	float	get_refl_power			(float2 tc)	{return 0;}
#endif
#endif
