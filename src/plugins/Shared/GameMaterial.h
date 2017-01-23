#ifndef __GAME_MATERIAL__H
#define __GAME_MATERIAL__H

#include "shaders.h"
#include "samplers.h"
#include "iparamm2.h"
#include "texmaps.h"
#include "XRayMtlRes.h"

#define XRAYMTL_CLASS_ID	Class_ID(0x35e04198, 0x2293156c)

// XRayMtl flags values
#define NEWSTDMTL_ADD_TRANSP	(1<<0)
#define NEWSTDMTL_FALLOFF_OUT	(1<<1)
#define NEWSTDMTL_WIRE		  	(1<<2)
#define NEWSTDMTL_2SIDE			(1<<3)
#define NEWSTDMTL_SOFTEN		(1<<4)
#define NEWSTDMTL_FILT_TRANSP 	(1<<5)
#define NEWSTDMTL_WIRE_UNITS	(1<<6)
#define NEWSTDMTL_LOCK_AD		(1<<8)
#define NEWSTDMTL_LOCK_DS		(1<<9)
#define NEWSTDMTL_UNUSED1		(1<<10)
#define NEWSTDMTL_LOCK_ADTEX	(1<<11)
#define NEWSTDMTL_FACEMAP		(1<<12)
#define NEWSTDMTL_OLDSPEC		(1<<13)
#define NEWSTDMTL_SSAMP_ON		(1<<14)
#define NEWSTDMTL_COLOR_SI		(1<<15)
#define NEWSTDMTL_FACETED		(1<<16)

#define NEWSTDMTL_ROLLUP0_OPEN  (1<<27)	// shader
#define NEWSTDMTL_ROLLUP1_OPEN  (1<<28)	// basic
#define NEWSTDMTL_ROLLUP2_OPEN  (1<<29)	// extra
#define NEWSTDMTL_ROLLUP3_OPEN  (1<<30)	// maps
#define NEWSTDMTL_ROLLUP4_OPEN  (1<<26)	// sampling
#define NEWSTDMTL_ROLLUP5_OPEN  (1<<25)	// dynamics
#define NEWSTDMTL_ROLLUP6_OPEN  (1<<24)	// effects

// only needed if the constant shader is included in shaders
#define  CONSTClassID (STDSHADERS_CLASS_ID+1)

#define NEWSTDMTL_ROLLUP_FLAGS (NEWSTDMTL_ROLLUP0_OPEN|NEWSTDMTL_ROLLUP1_OPEN|NEWSTDMTL_ROLLUP2_OPEN|NEWSTDMTL_ROLLUP3_OPEN \
	|NEWSTDMTL_ROLLUP4_OPEN|NEWSTDMTL_ROLLUP5_OPEN|NEWSTDMTL_ROLLUP6_OPEN)

class XRayMtlDlg;


// IDs for all the ParamBlocks and their parameters.  One block UI per rollout.
enum { std2_shader, std2_extended, std2_sampling, std_maps, std2_dynamics, std2_xray };  // pblock IDs
enum{
	std2_eshader_type,
	std2_cshader_type,
	std2_eshader_by_name,	// virtual param for accessing shader type by name
	std2_cshader_by_name,	// virtual param for accessing shader type by name
	std2_gamemtl_type,
	std2_gamemtl_by_name	// virtual param for accessing shader type by name
};
// std2_shader param IDs
enum 
{ 
	std2_shader_type, std2_wire, std2_two_sided, std2_face_map, std2_faceted,
	std2_shader_by_name  // virtual param for accessing shader type by name
};
// std2_extended param IDs
enum 
{ 
	std2_opacity_type, std2_opacity, std2_filter_color, std2_ep_filter_map,
	std2_falloff_type, std2_falloff_amnt, 
	std2_ior,
	std2_wire_size, std2_wire_units,
	std2_apply_refl_dimming, std2_dim_lvl, std2_refl_lvl,
};

// std2_sampling param IDs
enum 
{ 
	std2_ssampler, std2_ssampler_qual, std2_ssampler_enable, 
	std2_ssampler_adapt_on, std2_ssampler_adapt_threshold, std2_ssampler_advanced,
	std2_ssampler_subsample_tex_on, std2_ssampler_by_name, 
	std2_ssampler_param0, std2_ssampler_param1,
};
// std_maps param IDs
enum 
{
	std2_map_enables, std2_maps, std2_map_amnts, std2_mp_ad_texlock, 
};
// std2_dynamics param IDs
enum 
{
	std2_bounce, std2_static_friction, std2_sliding_friction,
};


// paramblock2 block and parameter IDs for the standard shaders
// NB these are duplicated in shaders/stdShaders.cpp...
enum { shdr_params, };
// shdr_params param IDs
enum 
{ 
	shdr_ambient, shdr_diffuse, shdr_specular,
	shdr_ad_texlock, shdr_ad_lock, shdr_ds_lock, 
	shdr_use_self_illum_color, shdr_self_illum_amnt, shdr_self_illum_color, 
	shdr_spec_lvl, shdr_glossiness, shdr_soften,
};

extern TCHAR *GetString(int id);

#define NUM_REFS		9

// refs
#define OLD_PBLOCK_REF	0		// reference number assignments
#define TEXMAPS_REF		1
#define SHADER_REF		2
#define SHADER_PB_REF	3
#define EXTENDED_PB_REF	4
#define SAMPLING_PB_REF	5
#define MAPS_PB_REF		6
#define DYNAMICS_PB_REF	7
#define SAMPLER_REF		8
#define XRAY_PB_REF		9

// sub anims
#define NUM_SUB_ANIMS	5
//#define OLD_PARAMS_SUB		0
#define TEXMAPS_SUB			0
#define SHADER_SUB			1
#define EXTRA_PB_SUB		2
#define SAMPLING_PB_SUB		3
#define DYNAMICS_PB_SUB		4


class RefmsgKillCounter {
private:
	friend class KillRefmsg;
	LONG	counter;

public:
	RefmsgKillCounter() : counter(-1) {}

	bool DistributeRefmsg() { return counter < 0; }
};

class KillRefmsg {
private:
	LONG&	counter;

public:
	KillRefmsg(RefmsgKillCounter& c) : counter(c.counter) { ++counter; }
	~KillRefmsg() { --counter; }
};

class XRayMtl: public StdMat2, public IReshading {
	// Animatable parameters
public:
	// current UI if open
	static ShaderParamDlg*	pShaderDlg;
	static IAutoMParamDlg*	masterDlg;
	static IAutoMParamDlg*	texmapDlg;
	static IAutoMParamDlg*	extendedDlg;
	static IAutoMParamDlg*	samplingDlg;
	static IAutoMParamDlg*	xrayDlg;
	static HWND				curHwmEdit;
	static IMtlParams*		curImp;
	static Tab<ClassDesc*>	shaderList;
	static Tab<ClassDesc*>	samplerList;
	static LPSTRVec			EShaders;
	static LPSTRVec			CShaders;
	static LPSTRVec			GameMtls;

	IParamBlock *old_pblock;    // ref 0, for old version loading
	Texmaps* maps;				// ref 1
	Interval ivalid;
	ReshadeRequirements mReshadeRQ; // mjm - 06.02.00
	ReshadeRequirements mInRQ;		// ca - 12/7/00
	ULONG flags;
	int shaderId;
	Shader *pShader;			// ref 2
	// new PB2 paramblocks, one per rollout
	IParamBlock2 *pb_xray;		// 
	IParamBlock2 *pb_shader;	// ref 3, 4, ...
	IParamBlock2 *pb_extended;	
	IParamBlock2 *pb_sampling;	
	IParamBlock2 *pb_maps;	
	IParamBlock2 *pb_dynamics;	

	// xray
	int		eshaderId;
	int		cshaderId;
	int		gamemtlId;

	Color filter;
	float opacity;	
	float opfall;
	float wireSize;
	float ioRefract;
	float dimIntens;
	float dimMult;
	BOOL dimReflect;

	// sampling 
	int samplerId;
	Sampler* pixelSampler;	// ref 8

	// composite of shader/mtl channel types
	int channelTypes[ STD2_NMAX_TEXMAPS ];
	int stdIDToChannel[ N_ID_CHANNELS ];

	// experiment: override filter
	BOOL	filterOverrideOn;
	float	filterSz;

	// Kill REFMSG_CHANGE messages. This counter is used to
	// prevent these messages when things really aren't changing.
	// Use the class KillRefmsg
	RefmsgKillCounter	killRefmsg;

	void SetFlag(ULONG f, ULONG val);
	void EnableMap(int i, BOOL onoff);
	BOOL IsMapEnabled(int i) { return (*maps)[i].mapOn; }
	BOOL KeyAtTime(int id,TimeValue t) { return (id == OPACITY_PARAM) ? pb_extended->KeyFrameAtTime(std2_opacity, t) : FALSE; }
	BOOL AmtKeyAtTime(int i, TimeValue t);
	int  GetMapState( int indx ); //returns 0 = no map, 1 = disable, 2 = mapon
	TSTR  GetMapName( int indx ); 
	void SyncADTexLock( BOOL lockOn );

	// from StdMat
	// these set Approximate colors into the plug in shader
	BOOL IsSelfIllumColorOn();
	void SetSelfIllumColorOn( BOOL on );
	void SetSelfIllumColor(Color c, TimeValue t);		
	void SetAmbient(Color c, TimeValue t);		
	void SetDiffuse(Color c, TimeValue t);		
	void SetSpecular(Color c, TimeValue t);
	void SetShininess(float v, TimeValue t);		
	void SetShinStr(float v, TimeValue t);		
	void SetSelfIllum(float v, TimeValue t);	
	void SetSoften(BOOL onoff) { SetFlag(NEWSTDMTL_SOFTEN,onoff); }

	void SetTexmapAmt(int imap, float amt, TimeValue t);
	void LockAmbDiffTex(BOOL onoff) { SetFlag(NEWSTDMTL_LOCK_ADTEX,onoff); }

	void SetWire(BOOL onoff){ pb_shader->SetValue(std2_wire,0, (onoff!=0) ); }//SetFlag(NEWSTDMTL_WIRE,onoff); }
	void SetWireSize(float s, TimeValue t);
	void SetWireUnits(BOOL onoff) { pb_extended->SetValue(std2_wire_units,0, (onoff!=0) ); } //SetFlag(NEWSTDMTL_WIRE_UNITS,onoff); }

	void SetFaceMap(BOOL onoff) { pb_shader->SetValue(std2_face_map,0, (onoff!=0) ); } //SetFlag(NEWSTDMTL_FACEMAP,onoff); }
	void SetTwoSided(BOOL onoff) { pb_shader->SetValue(std2_two_sided,0, (onoff!=0) ); } //SetFlag(NEWSTDMTL_2SIDE,onoff); }
	void SetFalloffOut(BOOL outOn) { pb_extended->SetValue(std2_falloff_type,0, (outOn!=0) ); } //SetFlag(NEWSTDMTL_FALLOFF_OUT,onoff); }
	void SetTransparencyType(int type);

	void SetFilter(Color c, TimeValue t);
	void SetOpacity(float v, TimeValue t);		
	void SetOpacFalloff(float v, TimeValue t);		
	void SetIOR(float v, TimeValue t);
	void SetDimIntens(float v, TimeValue t);
	void SetDimMult(float v, TimeValue t);

	int GetFlag(ULONG f) { return (flags&f)?1:0; }

	// >>>Shaders

	// these 3 internal only
	void SetShaderIndx( long shaderId, BOOL update=TRUE );
	long GetShaderIndx(){ return shaderId; }
	void SetShader( Shader* pNewShader );
	void ShuffleTexMaps( Shader* newShader, Shader* oldShader );
	void ShuffleShaderParams( Shader* newShader, Shader* oldShader );

	Shader* GetShader(){ return pShader; }
	void SwitchShader(Shader* pNewShader, BOOL loadDlg = FALSE);
	void SwitchShader(ClassDesc* pNewCD);
	BOOL SwitchShader(Class_ID shaderId);
	int FindShader( Class_ID& findId, ClassDesc** ppCD=NULL );
	BOOL IsShaderInUI() { return pb_shader && pb_shader->GetMap() && pShader && pShader->GetParamDlg(); }

	static void XRayMtl::LoadShaderList();
	static int XRayMtl::NumShaders();
	static ClassDesc* XRayMtl::GetShaderCD(int i);
	static void XRayMtl::LoadSamplerList();
	static int XRayMtl::NumSamplers();
	static ClassDesc* XRayMtl::GetSamplerCD(int i);

	void SetEShaderIndx(long shaderId);
	void SetCShaderIndx(long shaderId);
	void SetGameMtlIndx(long shaderId);
	long GetEShaderIndx(){ return eshaderId; }
	long GetCShaderIndx(){ return cshaderId; }
	long GetGameMtlIndx(){ return gamemtlId; }
	LPCSTR GetEShaderName(){ return GetEShader(eshaderId); }
	LPCSTR GetCShaderName(){ return GetCShader(cshaderId); }
	LPCSTR GetGameMtlName(){ return GetGameMtl(gamemtlId); }

	static void		XRayMtl::LoadXRayShaderList		();
	static void		XRayMtl::UnloadXRayShaderList	();
	virtual TCHAR*	GetEShader	(DWORD i);
	virtual TCHAR*	GetCShader	(DWORD i);
	virtual TCHAR*	GetGameMtl	(DWORD i);
	virtual int		FindEShader	(LPCSTR name);
	virtual int		FindCShader	(LPCSTR name);
	virtual int		FindGameMtl	(LPCSTR name);

	BOOL IsFaceted(){ return GetFlag(NEWSTDMTL_FACETED); }
	void SetFaceted( BOOL on ){	pb_shader->SetValue(std2_faceted,0, (on!=0) ); }

	// These utilitys provide R2.5 shaders, ONLY used for Translators
	// Does not & will not work for plug-in shaders
	void SetShading(int s);
	int GetShading();

	// from Mtl
	Color GetAmbient(int mtlNum=0, BOOL backFace=FALSE);		
	Color GetDiffuse(int mtlNum=0, BOOL backFace=FALSE);		
	Color GetSpecular(int mtlNum=0, BOOL backFace=FALSE);
	float GetShininess(int mtlNum=0, BOOL backFace=FALSE);	
	float GetShinStr(int mtlNum=0, BOOL backFace=FALSE) ;
	float GetXParency(int mtlNum=0, BOOL backFace=FALSE);
	float WireSize(int mtlNum=0, BOOL backFace=FALSE) { return wireSize; }

	// >>>> Self Illumination 
	float GetSelfIllum(int mtlNum, BOOL backFace) ;
	BOOL  GetSelfIllumColorOn(int mtlNum, BOOL backFace);
	Color GetSelfIllumColor(int mtlNum, BOOL backFace);

	// >>>> sampling
	void SetSamplingOn( BOOL on )
	{	
		pb_sampling->SetValue(std2_ssampler_enable, 0, on!=0 );
	}	
	BOOL GetSamplingOn()
	{	Interval iv; 
	BOOL on;
	pb_sampling->GetValue(std2_ssampler_enable, 0, on, iv );
	return on;
	}	
	void SetSamplingQuality( float quality )
	{	 
		pb_sampling->SetValue(std2_ssampler_qual, 0, quality );
	}	
	float GetSamplingQuality()
	{	Interval iv; 
	float q;
	pb_sampling->GetValue(std2_ssampler_qual, 0, q, iv );
	return q;
	}

	void SwitchSampler(ClassDesc* pNewCD);
	void SwitchSampler(Sampler* pNewSampler);
	BOOL SwitchSampler(Class_ID samplerId);
	int FindSampler( Class_ID findId, ClassDesc** pNewCD=NULL );
	Sampler * GetPixelSampler(int mtlNum=0, BOOL backFace=FALSE){  return pixelSampler; }	

	// these 2 internal only
	void SetSamplerIndx( long indx, BOOL update=TRUE );
	long  GetSamplerIndx(){ return samplerId; }
	void SetPixelSampler( Sampler * sampler );

	// Dynamics properties
	float GetDynamicsProperty(TimeValue t, int mtlNum, int propID);
	void SetDynamicsProperty(TimeValue t, int mtlNum, int propID, float value);

	// from StdMat
	BOOL GetSoften() { return GetFlag(NEWSTDMTL_SOFTEN); }
	BOOL GetFaceMap() { return GetFlag(NEWSTDMTL_FACEMAP); }
	BOOL GetTwoSided() { return GetFlag(NEWSTDMTL_2SIDE); }
	BOOL GetWire() { return GetFlag(NEWSTDMTL_WIRE); }
	BOOL GetWireUnits() { return GetFlag(NEWSTDMTL_WIRE_UNITS); }
	BOOL GetFalloffOut() { return GetFlag(NEWSTDMTL_FALLOFF_OUT); }  // 1: out, 0: in
	BOOL GetAmbDiffTexLock(){ return GetFlag(NEWSTDMTL_LOCK_ADTEX); } 
	int GetTransparencyType() {
		return (flags&NEWSTDMTL_FILT_TRANSP)?TRANSP_FILTER:
		flags&NEWSTDMTL_ADD_TRANSP ? TRANSP_ADDITIVE: TRANSP_SUBTRACTIVE;
	}
	Color GetFilter(TimeValue t);

	// these are stubs till i figure out scripting
	Color GetAmbient(TimeValue t);		
	Color GetDiffuse(TimeValue t);		
	Color GetSpecular(TimeValue t);
	float GetShininess( TimeValue t);		
	float GetShinStr(TimeValue t);	
	float GetSelfIllum(TimeValue t);
	BOOL  GetSelfIllumColorOn();
	Color GetSelfIllumColor(TimeValue t); 

	float GetOpacity( TimeValue t);		
	float GetOpacFalloff(TimeValue t);		
	float GetWireSize(TimeValue t);
	float GetIOR( TimeValue t);
	float GetDimIntens( TimeValue t);
	float GetDimMult( TimeValue t);
	float GetSoftenLevel( TimeValue t);
	BOOL MapEnabled(int i);
	float GetTexmapAmt(int imap, TimeValue t);

	// internal
	float GetOpacity() { return opacity; }		
	float GetOpacFalloff() { return opfall; }		
	float GetTexmapAmt(int imap);
	Color GetFilter();
	float GetIOR() { return ioRefract; }

	XRayMtl(BOOL loading = FALSE);
	~XRayMtl() {
		DiscardTexHandles();
	}
	BOOL ParamWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
	ParamDlg* CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
	BOOL SetDlgThing(ParamDlg* dlg);
	void UpdateTexmaps();
	void UpdateMapButtons();
	void UpdateExtendedMapButtons();
	void UpdateMtlDisplay();
	void UpdateLockADTex( BOOL passOn );
	void UpdateExtraParams( ULONG stdParams );
	void UpdateSamplingParams();


	Color TranspColor(ShadeContext& sc, float opac, Color& diff);
	void Shade(ShadeContext& sc);
	float EvalDisplacement(ShadeContext& sc); 
	Interval DisplacementValidity(TimeValue t); 
	void Update(TimeValue t, Interval& validr);
	void Reset();
	void OldVerFix(int loadVer);
	void BumpFix();
	Interval Validity(TimeValue t);
	void NotifyChanged();

	// Requirements
	ULONG Requirements(int subMtlNum);
	void MappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq);

	// Methods to access texture maps of material
	int NumSubTexmaps() { return STD2_NMAX_TEXMAPS; }
	Texmap* GetSubTexmap(int i) { return (*maps)[i].map; }
	int MapSlotType(int i);
	void SetSubTexmap(int i, Texmap *m);
	TSTR GetSubTexmapSlotName(int i);
	int SubTexmapOn(int i) { return  MAPACTIVE(i); } 
	long StdIDToChannel( long id ){ return stdIDToChannel[id]; }

	Class_ID ClassID();
	SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CLASS_NAME); }  

	void DeleteThis();

	int NumSubs() { return NUM_SUB_ANIMS; }  
	Animatable* SubAnim(int i);
	TSTR SubAnimName(int i);
	int SubNumToRefNum(int subNum);

	// JBW: add direct ParamBlock access
	int	NumParamBlocks() { return 5; }
	IParamBlock2* GetParamBlock(int i);
	IParamBlock2* GetParamBlockByID(BlockID id);

	// From ref
	int NumRefs() { return NUM_REFS; }
	RefTargetHandle GetReference(int i);
	void SetReference(int i, RefTargetHandle rtarg);

	RefTargetHandle Clone(RemapDir &remap = NoRemap());
	RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
		PartID& partID, RefMessage message );

	// IO
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// begin - ke/mjm - 03.16.00 - merge reshading code
	BOOL SupportsRenderElements(){ return TRUE; }
	//		BOOL SupportsReShading(ShadeContext& sc);
	void UpdateReshadeRequirements(RefTargetHandle hTarget, PartID partID); // mjm - 06.02.00
	ReshadeRequirements GetReshadeRequirements() { return mReshadeRQ; } // mjm - 06.02.00
	void PreShade(ShadeContext& sc, IReshadeFragment* pFrag);
	void PostShade(ShadeContext& sc, IReshadeFragment* pFrag, int& nextTexIndex, IllumParams* ip);
	// end - ke/mjm - 03.16.00 - merge reshading code

	// begin - dds- 04.27.00 - multiple map display support
#define NTEXHANDLES 2
	TexHandle *texHandle[NTEXHANDLES];
	short useSubForTex[NTEXHANDLES];
	short texOpsType[NTEXHANDLES];
	int numTexHandlesUsed;
	Interval texHandleValid;
	void SetTexOps(Material *mtl, int i, int type);
	void DiscardTexHandles();
	BOOL SupportTexDisplay() { return TRUE; }
	BOOL SupportsMultiMapsInViewport() { return TRUE; }
	void ActivateTexDisplay(BOOL onoff);
	void SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb);
	// end - dds- 04.27.00 - multiple map display support

	// --- Material evaluation - from Mtl ---
	bool IsOutputConst( ShadeContext& sc, int stdID );
	bool EvalColorStdChannel( ShadeContext& sc, int stdID, Color& outClr );
	bool EvalMonoStdChannel( ShadeContext& sc, int stdID, float& outVal );

	void* GetInterface(ULONG id);

	float GetReflectionDim(float diffIllumIntensity ){
		if (dimReflect)
			return ((1.0f-dimIntens)*diffIllumIntensity*dimMult + dimIntens);
		else 
			return 1.0f;
	}
	Color TranspColor( float opac, Color filt, Color diff );

	float GetEffOpacity(ShadeContext& sc, float opac){
		if ( opac != 1.0f || opfall != 0.0f) {
			if (opfall != 0.0f) {	
				Point3 N = (flags & NEWSTDMTL_FACETED) ? sc.GNormal() : sc.Normal();
				float d = _abs( DotProd( N, sc.V() ) );
				if (flags & NEWSTDMTL_FALLOFF_OUT) d = 1.0f-d;
				return opac * (1.0f - opfall * d);
			} else return opac;
		} else return 1.0f;
	}

private:
	// --- Material evaluation ---
	bool EvalChannel( ShadeContext& sc, int channelID, Color& outClr);
	bool EvalBump( ShadeContext& sc, Color& outClr );
	bool EvalReflection( ShadeContext& sc, Color& outClr );
	bool EvalReflection( ShadeContext& sc, float& outVal );
	bool EvalRefraction( ShadeContext& sc, Color& outClr );
	bool EvalRefraction( ShadeContext& sc, float& outVal );
	bool EvalDisplacement( ShadeContext& sc, float& outVal );
	bool ShouldEvalSubTexmap( ShadeContext& sc, int id );

};

Mtl* CreateXRayMtl();

#endif // __GAME_MATERIAL__H
