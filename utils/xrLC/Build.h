// Build.h: interface for the CBuild class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BUILD_H__C7D43EBA_1C15_4FF4_A3ED_E89F6D759F58__INCLUDED_)
#define AFX_BUILD_H__C7D43EBA_1C15_4FF4_A3ED_E89F6D759F58__INCLUDED_
#pragma once

//#include "../../editors/LevelEditor/Engine/communicate.h"

#include "../../xrcore/fs.h"
//.#include "../../xrEngine/xrLevel.h"

#include "../shader_xrlc.h"
//#include "xrMU_Model.h"

struct		 STextureParams;
#pragma comment(lib,"dxt.lib")
extern "C" bool __declspec(dllimport) __stdcall DXTCompress(LPCSTR out_name, u8* raw_data, u8* normal_map, u32 w, u32 h, u32 pitch, STextureParams* fmt, u32 depth);

//#include "../xrLC_Light/xrLC_GlobalData.h"
//#include "../xrLC_Light/xrface.h"
#include "../xrLC_Light/b_build_texture.h"
#include "../xrLC_Light/xrfacedefs.h"

class xrLC_GlobalData;
class xrMU_Model;
class xrMU_Reference;
extern "C" XRLC_LIGHT_API xrLC_GlobalData*	lc_global_data();
//////////////////////////////////////////////////////////////////////////
// tesselator callbacks



typedef	int		tesscb_estimator	( const Face* F );	// -1 = none, 0,1,2 = edge-number
typedef void	tesscb_face			(Face*		F);	// new face
typedef void	tesscb_vertex		(Vertex*	V);	// new vertex

class  base_lighting;
class  INetReader;
//////////////////////////////////////////////////////////////////////////
class CBuild  
{
public:
	CMemoryWriter					&err_invalid();
	CMemoryWriter					&err_tjunction();
	CMemoryWriter					&err_multiedge();
	void							err_save	();

	Fbox							scene_bb;
	xr_vector<b_shader>				shader_render;
	xr_vector<b_shader>				shader_compile;
    xr_vector<b_light_dynamic>		L_dynamic;
	xr_vector<b_glow>				glows;
	xr_vector<b_portal>				portals;
	xr_vector<b_lod>				lods;
	string_path						path;
	xr_vector<LPCSTR>				g_Shaders;


	xr_vector<b_material>			&materials()	;
	xr_vector<b_BuildTexture>		&textures()	;
	base_lighting					&L_static();
	xr_vector<xrMU_Model*>			&mu_models();
	xr_vector<xrMU_Reference*>		&mu_refs();

	Shader_xrLC_LIB					&shaders();


	void	mem_Compact				();
	void	mem_CompactSubdivs		();
public:
	void	Load					(const b_params& P, const IReader&  fs);
	void	Run						(LPCSTR path);
	void	StartMu					();
	void	RunAfterLight			( IWriter* fs	);
	void	Tesselate				();
	void	PreOptimize				();
	void	CorrectTJunctions		();

	void	xrPhase_AdaptiveHT		();
	void	u_Tesselate				(tesscb_estimator* E, tesscb_face* F, tesscb_vertex* V);
	void	u_SmoothVertColors		(int count);

	void	CalcNormals				();
	void	MU_ModelsCalculateNormals();
	void	xrPhase_TangentBasis	();

	void	BuildCForm				();
	void	BuildPortals			(IWriter &fs);
	void	BuildRapid				(BOOL bSave);
	void	xrPhase_Radiosity		();
		
	void	IsolateVertices			(BOOL bProgress);
	void	xrPhase_ResolveMaterials();
	void	xrPhase_UVmap			();
	void	xrPhase_Subdivide		();
	void	ImplicitLighting		();
	void	Light_prepare			();
	void	Light					();
	void	LMapsLocal				();
	void	LMaps					();
	//void	Light_R2				();
	void	LightVertex				();
	void	xrPhase_MergeLM			();
	void	xrPhase_MergeGeometry	();

	void	Flex2OGF				();
	void	BuildSectors			();

	void	SaveLights				(IWriter &fs);
	void	SaveTREE				(IWriter &fs);
	void	SaveSectors				(IWriter &fs);

	void	validate_splits			();
	bool	IsOGFContainersEmpty	();
	void	CheckBeforeSave			( u32 stage );
	void	TempSave				( u32 stage );
	void	read					( INetReader &r );
	void	write					( IWriter	&w ) const ;

	CBuild	();
	~CBuild	();
};

extern CBuild*		pBuild;			;
extern vec2Face		g_XSplit		;

#endif // !defined(AFX_BUILD_H__C7D43EBA_1C15_4FF4_A3ED_E89F6D759F58__INCLUDED_)
