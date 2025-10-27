#ifndef	common_samplers_h_included
#define	common_samplers_h_included

//////////////////////////////////////////////////////////////////////////////////////////
// Geometry phase / deferring               	//

sampler 	smp_nofilter;   //	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT 
sampler 	smp_rtlinear;	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR 
sampler 	smp_linear;		//	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
sampler 	smp_base;		//	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC

Texture2D 	s_base;		//	smp_base
#ifdef USE_MSAA
	#ifndef SM_5
		Texture2DMS<float4,MSAA_SAMPLES> s_generic;
	#else
		Texture2DMS<float4>	s_generic;
	#endif
#else
Texture2D   s_generic;
#endif
Texture2D 	s_bump;             	//
Texture2D 	s_bumpX;                //
Texture2D 	s_detail;               //
Texture2D 	s_detailBump;           //	
Texture2D 	s_detailBumpX;          //	Error for bump detail
Texture2D 	s_hemi;             	//

Texture2D 	s_mask;             	//

Texture2D 	s_dt_r;                	//
Texture2D 	s_dt_g;                	//
Texture2D 	s_dt_b;                	//
Texture2D 	s_dt_a;                	//

Texture2D 	s_dn_r;                	//
Texture2D 	s_dn_g;                	//
Texture2D 	s_dn_b;                	//
Texture2D 	s_dn_a;                	//

//////////////////////////////////////////////////////////////////////////////////////////
// Lighting/shadowing phase                     //

sampler 	smp_material;

#ifdef USE_MSAA
	#ifndef SM_5
		Texture2DMS<float4,MSAA_SAMPLES>	s_position;	// rgb.a = diffuse.gloss
		Texture2DMS<float4,MSAA_SAMPLES>	s_diffuse;	// rgb.a = diffuse.gloss
		Texture2DMS<float4,MSAA_SAMPLES>	s_accumulator;      	// rgb.a = diffuse.specular		
	#else
		Texture2DMS<float4>	s_position;	// rgb.a = diffuse.gloss
		Texture2DMS<float4>	s_diffuse;	// rgb.a = diffuse.gloss
		Texture2DMS<float4>	s_accumulator;      	// rgb.a = diffuse.specular		
	#endif
#else
	Texture2D	s_position;	//	smp_nofilter or Load
	Texture2D	s_normal;	//	smp_nofilter or Load
	Texture2D	s_diffuse;	// rgb.a = diffuse.gloss
	Texture2D	s_accumulator;      	// rgb.a = diffuse.specular
#endif

Texture2D	s_lmap;		// 2D/???cube projector lightmap
Texture3D	s_material;	//	smp_material

//////////////////////////////////////////////////////////////////////////////////////////
// Combine phase                                //

Texture2D	s_bloom;	//
Texture2D	s_image;	// used in various post-processing
Texture2D	s_tonemap;	// actually MidleGray / exp(Lw + eps)


//Other
Texture2D s_blur_2;
Texture2D s_blur_4;
Texture2D s_blur_8;

Texture2D s_bloom_new;
//Texture2D s_motion_vector;

Texture2D s_lens_dirt; //shaders\\lens_dirt
Texture2D s_noise_1; //shaders\\noise\\noise_tex
Texture2D s_lut_atlas; //shaders\\lut_atlas

#endif	//	#ifndef	common_samplers_h_included