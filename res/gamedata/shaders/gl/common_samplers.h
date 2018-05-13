#ifndef	common_samplers_h_included
#define	common_samplers_h_included

#define Texture2D	uniform sampler2D
#define Texture3D	uniform sampler3D
#define Texture2DMS uniform sampler2DMS
#define TextureCube uniform samplerCube
#define Texture2DShadow uniform sampler2DShadow

//////////////////////////////////////////////////////////////////////////////////////////
// Geometry phase / deferring               	//

//sampler 	smp_nofilter;   //	Use D3DTADDRESS_CLAMP,	D3DTEXF_POINT,			D3DTEXF_NONE,	D3DTEXF_POINT 
//sampler 	smp_rtlinear;	//	Use D3DTADDRESS_CLAMP,	D3DTEXF_LINEAR,			D3DTEXF_NONE,	D3DTEXF_LINEAR 
//sampler 	smp_linear;		//	Use	D3DTADDRESS_WRAP,	D3DTEXF_LINEAR,			D3DTEXF_LINEAR,	D3DTEXF_LINEAR
//sampler 	smp_base;		//	Use D3DTADDRESS_WRAP,	D3DTEXF_ANISOTROPIC, 	D3DTEXF_LINEAR,	D3DTEXF_ANISOTROPIC

Texture2D 	s_base;		//	smp_base
#ifdef USE_MSAA
Texture2DMS	s_generic;	//	smp_generic
#else
Texture2D   s_generic;
#endif
Texture2D 	s_bump;             	//
Texture2D 	s_bumpX;                //
Texture2D 	s_detail;               //
Texture2D 	s_detailBump;           //	
Texture2D 	s_detailBumpX;          //	Error for bump detail
//Texture2D 	s_bumpD;                //
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

//sampler 	smp_material;

//uniform sampler2D       s_depth;                //
#ifdef USE_MSAA
Texture2DMS	s_position;	//	smp_nofilter or Load
Texture2DMS	s_normal;	//	smp_nofilter or Load
#else
Texture2D	s_position;	//	smp_nofilter or Load
Texture2D	s_normal;	//	smp_nofilter or Load
#endif
Texture2D	s_lmap;		// 2D/???cube projector lightmap
Texture3D	s_material;	//	smp_material
//uniform sampler1D       s_attenuate;        	//


//////////////////////////////////////////////////////////////////////////////////////////
// Combine phase                                //
#ifdef USE_MSAA
Texture2DMS	s_diffuse;	// rgb.a = diffuse.gloss
Texture2DMS	s_accumulator;      	// rgb.a = diffuse.specular
#else
Texture2D	s_diffuse;	// rgb.a = diffuse.gloss
Texture2D	s_accumulator;      	// rgb.a = diffuse.specular
#endif
//uniform sampler2D       s_generic;              //
Texture2D	s_bloom;	//
Texture2D	s_image;	// used in various post-processing
Texture2D	s_tonemap;	// actually MidleGray / exp(Lw + eps)


#endif	//	#ifndef	common_samplers_h_included