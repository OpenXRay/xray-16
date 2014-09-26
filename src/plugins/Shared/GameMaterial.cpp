#include "stdafx.h"
#pragma hdrstop

#include "GameMaterial.h"
#include "BlenderListLoader.h"

#include "macrorec.h"

class XRayMtlClassDesc:public ClassDesc2 {
public:
	int 			IsPublic		()				{ return 1; }
	void *			Create			(BOOL loading)	{ return xr_new<XRayMtl>(loading); }
	const TCHAR *	ClassName		()				{ return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID	()				{ return MATERIAL_CLASS_ID; }
	Class_ID 		ClassID			()				{ return XRAYMTL_CLASS_ID; }
	const TCHAR* 	Category		()				{ return GetString(IDS_CATEGORY);  }
	// JBW: new descriptor data accessors added.  Note that the 
	//      internal name is hardwired since it must not be localized.
	const TCHAR*	InternalName	()				{ return _T("S.T.A.L.K.E.R.Mtl"); }		// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance		()				{ return hInstance; }			// returns owning module handle
};

static XRayMtlClassDesc stdmtl2CD;
ClassDesc2* GetXRayMtlDesc() { return &stdmtl2CD;  }

ShaderParamDlg* XRayMtl::pShaderDlg;	
IAutoMParamDlg* XRayMtl::xrayDlg;
IAutoMParamDlg* XRayMtl::masterDlg;
IAutoMParamDlg* XRayMtl::texmapDlg;
IAutoMParamDlg* XRayMtl::extendedDlg;
IAutoMParamDlg* XRayMtl::samplingDlg;
HWND			XRayMtl::curHwmEdit;
IMtlParams*	    XRayMtl::curImp;
Tab<ClassDesc*> XRayMtl::shaderList;
Tab<ClassDesc*> XRayMtl::samplerList;
LPSTRVec		XRayMtl::EShaders;
LPSTRVec		XRayMtl::CShaders;
LPSTRVec		XRayMtl::GameMtls;

/////////////////////////////////////////////////////////////////////////
// utility function
IC float Min( float a, float b ) { return (a < b) ? a : b; }
IC float Min( float a, float b, float c ) { return (a < b) ? Min(a,c) : Min(b,c); }
IC float Min( Color& c ){ return Min( c.r, c.g, c.b ); }
IC float Max( float a, float b ) { return (a < b) ? b : a; }
IC float Max( float a, float b, float c ) { return (a < b) ? Max( b, c ) : Max(a,c); }
IC float Max( Color& c ){ return Max( c.r, c.g, c.b ); }

IC float LBound( float x, float min = 0.0f ){ return x < min ? min : x; }
IC Color LBound( Color& c, float min = 0.0f )
{ return Color( LBound(c.r, min), LBound(c.g, min), LBound(c.b, min) ); }
IC float UBound( float x, float max = 1.0f ){ return x > max ? max : x; }
IC Color UBound( Color& c, float max = 1.0f )
{ return Color( UBound(c.r,max), UBound(c.g,max), UBound(c.b,max) ); }

IC float Bound( float x, float min = 0.0f, float max = 1.0f ){ return x < min? min:( x > max? max : x); }
IC Color Bound( Color& c, float min = 0.0f, float max = 1.0f ){
	return Color( Bound(c.r,min,max), Bound(c.r,min,max), Bound(c.r,min,max) );
}
IC void BoundInPlace( AColor& c , float min = 0.0f, float max = 1.0f ){
	if( c.r > max ) c.r = max; else if( c.r < min ) c.r = min;
	if( c.g > max ) c.g = max; else if( c.g < min ) c.g = min;
	if( c.b > max ) c.b = max; else if( c.b < min ) c.b = min;
	if( c.a > max ) c.a = max; else if( c.a < min ) c.a = min;
}
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
//	Material Texture Channels
//
#define MTL_NTEXMAPS	4


// channels ids needed by shader
#define _BUMP		0
#define _REFLECT	1
#define _REFRACT	2
#define _DISPLACE	3

// channel names
static int mtlChannelNameIDS[] = {
	IDS_DS_BU, IDS_DS_RL, IDS_DS_RR, IDS_DS_DP, 
};	

// what channel corresponds to the stdMat ID's
static int mtlStdIDToChannel[N_ID_CHANNELS] = { -1, -1, -1, -1, -1,	-1,   -1, -1, 0, 1, 2, 3  };

// internal non-local parsable channel map names
static TCHAR* mtlChannelInternalNames[STD2_NMAX_TEXMAPS] = {
	_T("bumpMap"), _T("reflectionMap"), _T("refractionMap"), _T("displacementMap"),
		_T(""),	_T(""),	_T(""),	_T(""),

		_T(""),	_T(""),	_T(""),	_T(""),
		_T(""),	_T(""),	_T(""),	_T(""),

		_T(""),	_T(""),	_T(""),	_T(""),
		_T(""),	_T(""),	_T(""),	_T(""),
};	

// sized for nmax textures
static int mtlChannelType[STD2_NMAX_TEXMAPS] = {
	BUMP_CHANNEL, REFL_CHANNEL, REFR_CHANNEL, DISP_CHANNEL,	
		UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,

		UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,
		UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,
		UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,
		UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL, UNSUPPORTED_CHANNEL,
};	


static IReshading::ReshadeRequirements chooseReshade[3][3] = {
	{ IReshading::RR_None,
		IReshading::RR_NeedPreshade,
		IReshading::RR_NeedReshade  },		// RR_None
	{ IReshading::RR_NeedPreshade,
	IReshading::RR_NeedPreshade,
	IReshading::RR_NeedPreshade },		// RR_NeedPreshade
	{ IReshading::RR_NeedReshade,
	IReshading::RR_NeedPreshade,
	IReshading::RR_NeedReshade  },		// RR_NeedReshade
};

#define NO_UPDATE	-2

// parameter setter callback, reflect any ParamBlock-mediated param setting in instance data members.
// Since standard2 keeps many parameters as instance data members, these setter callbacks
// are implemented to reduce changes to existing code 

class ShaderPBAccessor : public PBAccessor
{
public:
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		XRayMtl* m = (XRayMtl*)owner;
		switch (id)
		{
			// use item data to unscramble sorted lists
		case std2_shader_type: {
			ClassDesc* pCD = XRayMtl::GetShaderCD(v.i);
			if (pCD && m->GetShaderIndx() != NO_UPDATE )
			{
				m->SwitchShader(pCD); 
				m->shaderId = v.i; 
			}
							   } break;
		case std2_shader_by_name: {
			for (int i = 0; i < XRayMtl::NumShaders(); i++)
			{
				ClassDesc* pCD = XRayMtl::GetShaderCD(i);
				if (_tcsicmp(pCD->ClassName(), v.s) == 0)
				{
					m->pb_shader->SetValue(std2_shader_type, 0, i);
					break;
				}
			}
								  } break;
		case std2_wire:
			m->SetFlag(NEWSTDMTL_WIRE, v.i); break;		
		case std2_two_sided:
			m->SetFlag(NEWSTDMTL_2SIDE, v.i); break;		
		case std2_face_map:
			m->SetFlag(NEWSTDMTL_FACEMAP, v.i); break;
		case std2_faceted:
			m->SetFlag(NEWSTDMTL_FACETED, v.i); break;
		}
	}

	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval& valid)    // get into v
	{
		XRayMtl* m = (XRayMtl*)owner;
		switch (id)
		{
		case std2_shader_by_name: {
			ClassDesc* pCD = XRayMtl::GetShaderCD(m->shaderId);
			if (pCD)
				v.s = (TCHAR*)pCD->ClassName();
								  } break;
		}
	}
};
static ShaderPBAccessor shaderPBAccessor;

// shader rollout dialog proc
class ShaderDlgProc : public ParamMap2UserDlgProc 
{
public:
	BOOL DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) 
		{
		case WM_INITDIALOG:
			//  load the shader dropdown
			HWND hwndShader = GetDlgItem(hWnd, IDC_SHADER);
			SendMessage(hwndShader, CB_RESETCONTENT, 0L, 0L);
			for (int i = 0; i < XRayMtl::NumShaders(); i++) {
				ClassDesc* pClassD = XRayMtl::GetShaderCD(i);
				int n = SendMessage(hwndShader, CB_ADDSTRING, 0L, (LPARAM)(pClassD->ClassName()) );
				SendMessage(hwndShader, CB_SETITEMDATA, n, (LPARAM)pClassD );
			}
			return TRUE;
		}
		return FALSE;
	}
	void DeleteThis() { }
};


static ShaderDlgProc shaderDlgProc;

// shader parameters
static ParamBlockDesc2 std2_shader_blk ( std2_shader, _T("newShaderParameters"),  0, &stdmtl2CD, P_AUTO_CONSTRUCT + P_AUTO_UI, SHADER_PB_REF, 
										//rollout
										IDD_DMTL_SHADER4, IDS_KE_SHADER, 0, 0, &shaderDlgProc, 
										// params
										std2_shader_type, _T("shaderType"), TYPE_INT, 				0, 		IDS_JW_SHADERTYPE, 	
										p_default, 		3, 
										p_ui, 			TYPE_INTLISTBOX, IDC_SHADER, 0,
										p_accessor,		&shaderPBAccessor,
										end, 
										std2_wire, 		_T("wire"), 		TYPE_BOOL, 				0, 		IDS_DS_WIREFRAME, 	
										p_default, 		FALSE, 
										p_ui, 			TYPE_SINGLECHEKBOX, IDC_WIRE, 
										p_accessor,		&shaderPBAccessor,
										end, 
										std2_two_sided, 	_T("twoSided"), TYPE_BOOL, 				0, 		IDS_JW_TWOSIDED, 	
										p_default, 		FALSE, 
										p_ui, 			TYPE_SINGLECHEKBOX, IDC_2SIDE, 
										p_accessor,		&shaderPBAccessor,
										end, 
										std2_face_map, 	_T("faceMap"), 		TYPE_BOOL, 				0, 		IDS_JW_FACEMAP, 	
										p_default, 		FALSE, 
										p_ui, 			TYPE_SINGLECHEKBOX, IDC_FACE_MAP, 
										p_accessor,		&shaderPBAccessor,
										end, 
										std2_faceted, 		_T("faceted"), 		TYPE_BOOL, 			0, 		IDS_KE_FACETED, 	
										p_default, 		FALSE, 
										p_ui, 			TYPE_SINGLECHEKBOX, IDC_FACETED, 
										p_accessor,		&shaderPBAccessor,
										end,
										std2_shader_by_name, _T("shaderByName"), TYPE_STRING, 		0, 		IDS_JW_SHADERBYNAME, 	
										p_accessor,		&shaderPBAccessor,
										end,
										end
										);

////////////////////////
// Extended Rollout
class ExtendedPBAccessor : public PBAccessor
{
public:
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		XRayMtl* m = (XRayMtl*)owner;
		IParamMap2* map = m->pb_extended ? m->pb_extended->GetMap() : NULL;

		switch (id)
		{
		case std2_opacity_type:
			m->SetFlag(NEWSTDMTL_ADD_TRANSP, v.i == 2);
			m->SetFlag(NEWSTDMTL_FILT_TRANSP, v.i == 0);
			if (map != NULL)
			{
				map->Show(std2_filter_color, v.i == 0);
				map->Show(std2_ep_filter_map, v.i == 0);
				m->UpdateTexmaps();
				m->UpdateLockADTex(FALSE);
			}
			break;
		case std2_opacity:
			m->opacity = v.f; 
			if (m->pShader->GetParamDlg())
				m->pShader->GetParamDlg()->UpdateOpacity(); 
			break;
		case std2_filter_color:
			m->filter = *v.p; break;
		case std2_falloff_type:
			m->SetFlag(NEWSTDMTL_FALLOFF_OUT, v.i == 1); break;
		case std2_falloff_amnt:
			m->opfall = v.f; break;
		case std2_ior:
			m->ioRefract = v.f; break;

		case std2_wire_size:
			m->wireSize = v.f; break;
		case std2_wire_units:
			m->SetFlag(NEWSTDMTL_WIRE_UNITS, v.i == 1); break;

		case std2_apply_refl_dimming:
			m->dimReflect = v.i; break;
		case std2_dim_lvl:
			m->dimIntens = v.f; break;
		case std2_refl_lvl:
			m->dimMult = v.f; break;
		}
	}
};

static ExtendedPBAccessor extendedPBAccessor;

// extra rollout dialog proc
class ExtraDlgProc : public ParamMap2UserDlgProc 
{
public:
	BOOL DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) 
		{
		case WM_INITDIALOG:{
			XRayMtl* m = (XRayMtl*)map->GetParamBlock()->GetOwner();
			m->UpdateExtraParams( m->GetShader()->SupportStdParams() );
			return TRUE;
						   }
		}
		return FALSE;
	}
	void DeleteThis() { }
};

static ExtraDlgProc extraDlgProc;

// extended parameters
static ParamBlockDesc2 std2_extended_blk ( std2_extended, _T("extendedParameters"),  0, &stdmtl2CD, P_AUTO_CONSTRUCT + P_AUTO_UI, EXTENDED_PB_REF, 
										  //rollout
										  IDD_DMTL_EXTRA6, IDS_DS_EXTRA, 0, APPENDROLL_CLOSED, &extraDlgProc, 
										  // params
										  std2_opacity_type,  _T("opacityType"), 	TYPE_INT, 		0, 				IDS_JW_OPACITYTYPE,
										  p_default, 		0, 
										  p_range, 		0, 2, 
										  p_ui, 			TYPE_RADIO, 	3, IDC_TR_SUB2, IDC_TR_SUB, IDC_TR_ADD, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_opacity,		_T("opacity"), 	TYPE_PCNT_FRAC, 	P_ANIMATABLE, 	IDS_DS_OPACITY, 
										  p_default, 		0.0, 
										  p_range, 		0.0, 100.0,   // UI us in the shader rollout
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_filter_color, 	 _T("filterColor"), TYPE_RGBA, 		P_ANIMATABLE, 	IDS_DS_FILTER, 	
										  p_default, 		Color(0, 0, 0), 
										  p_ui, 			TYPE_COLORSWATCH, IDC_FILTER_CS, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_ep_filter_map, _T("filterMap"), 	TYPE_TEXMAP, 	P_SUBTEX + P_NO_AUTO_LABELS, IDS_JW_FILTERMAP, 
										  p_subtexno, 	ID_FI, 
										  p_ui, 			TYPE_TEXMAPBUTTON, IDC_MAPON_FI, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_falloff_type,  _T("opacityFallOffType"), 	TYPE_INT, 	0, 			IDS_JW_FALLOFFTYPE, 	 
										  p_default, 		0, 
										  p_range, 		0, 1, 
										  p_ui, 			TYPE_RADIO, 	2, IDC_TF_IN, IDC_TF_OUT, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_falloff_amnt, _T("opacityFallOff"), TYPE_PCNT_FRAC, 	P_ANIMATABLE, 	IDS_DS_FALLOFF, 
										  p_default, 		0.0, 
										  p_range, 		0.0, 100.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_TF_EDIT, IDC_TF_SPIN, 0.1, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_ior, 			_T("ior"), 			TYPE_FLOAT, 	P_ANIMATABLE, 	IDS_DS_IOR, 
										  p_default, 		1.5, 
										  p_range, 		0.0, 10.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_IOR_EDIT, IDC_IOR_SPIN, 0.01, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_wire_size, 		_T("wireSize"), TYPE_FLOAT, 	P_ANIMATABLE, 	IDS_DS_WIRESZ, 
										  p_default, 		1.0, 
										  p_range, 		0.0, 100.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_WIRE_EDIT, IDC_WIRE_SPIN, 1.0, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_wire_units, 	_T("wireUnits"), 	TYPE_INT, 		0, 				IDS_JW_WIREUNITS, 	 
										  p_default, 		0, 
										  p_range, 		0, 1, 
										  p_ui, 			TYPE_RADIO, 	2, IDC_PIXELS, IDC_UNITS, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_apply_refl_dimming, _T("applyReflectionDimming"), 	TYPE_BOOL, 	0, 	IDS_JW_APPLYREFDIM, 	
										  p_default, 		FALSE, 
										  p_ui, 			TYPE_SINGLECHEKBOX, IDC_DIM_REFL, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_dim_lvl, 		_T("dimLevel"), 		TYPE_FLOAT, 	P_ANIMATABLE, 	IDS_DS_DIMLEV, 
										  p_default, 		0.0, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIM_AMT, IDC_DIM_AMTSPIN, 0.01, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  std2_refl_lvl, 		_T("reflectionLevel"), 	TYPE_FLOAT, 	P_ANIMATABLE, 	IDS_DS_DIMMULT, 
										  p_default, 		1.0, 
										  p_range, 		0.1, 10.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_DIM_MULT, IDC_DIM_MULTSPIN, 0.01, 
										  p_accessor,		&extendedPBAccessor,
										  end, 
										  end
										  );

///////////////////////////////////////////////////////////////////////////////////
// Sampling Rollout
//
class SamplingPBAccessor : public PBAccessor
{
public:
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		XRayMtl* m = (XRayMtl*)owner;
		macroRecorder->Disable();
		switch (id)
		{
		case std2_ssampler: {
			ClassDesc* pCD = XRayMtl::GetSamplerCD(v.i);
			if (pCD && m->GetSamplerIndx() != NO_UPDATE )
			{
				m->SwitchSampler(pCD); 
				//					m->samplerId = v.i;
			}
							} break;
		case std2_ssampler_by_name: {
			for (int i = 0; i < XRayMtl::NumSamplers(); i++)
			{
				ClassDesc* pCD = XRayMtl::GetSamplerCD(i);
				if (_tcsicmp(pCD->ClassName(), v.s) == 0)
				{
					m->pb_sampling->SetValue(std2_ssampler, 0, i);
					break;
				}
			}
									} break;
		case std2_ssampler_qual:
			m->GetPixelSampler()->SetQuality( v.f ); break;
		case std2_ssampler_enable:
			m->GetPixelSampler()->SetEnable( v.i ); 
			m->SetFlag(NEWSTDMTL_SSAMP_ON, v.i); break;		

		case std2_ssampler_adapt_threshold:
			m->GetPixelSampler()->SetAdaptiveThreshold( v.f ); break;
		case std2_ssampler_adapt_on:{
			m->GetPixelSampler()->SetAdaptiveOn( v.i );
			IParamMap2* map = m->pb_sampling ? m->pb_sampling->GetMap() : NULL;
			if ( map ) {
				map->Enable(std2_ssampler_adapt_threshold, v.i );
			}
									} break;
		case std2_ssampler_subsample_tex_on:
			m->GetPixelSampler()->SetTextureSuperSampleOn( v.i ); break;
		case std2_ssampler_advanced: {
			IParamMap2* map = m->pb_sampling ? m->pb_sampling->GetMap() : NULL;
			HWND hwnd = ( map ) ? map->GetHWnd() : NULL;
			m->GetPixelSampler()->ExecuteParamDialog( hwnd, m );
									 } break;
		case std2_ssampler_param0:
			m->GetPixelSampler()->SetOptionalParam( 0, v.f ); break;
		case std2_ssampler_param1:
			m->GetPixelSampler()->SetOptionalParam( 1, v.f ); break;

		}
		macroRecorder->Enable();
	}
	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval& valid)    // get into v
	{
		XRayMtl* m = (XRayMtl*)owner;
		switch (id)
		{
		case std2_ssampler_by_name: {
			ClassDesc* pCD = XRayMtl::GetSamplerCD(m->samplerId);
			if (pCD)
				v.s = (TCHAR*)pCD->ClassName();
									} break;
		}
	}
};


static SamplingPBAccessor samplingPBAccessor;

// sampling rollout dialog proc

class SamplingDlgProc : public ParamMap2UserDlgProc 
{
public:
	BOOL DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) 
		{
		case WM_INITDIALOG:
			//  load the sampler dropdown
			HWND hwndSamplerCombo = GetDlgItem(hWnd, IDC_PIX_SAMPLER);
			SendMessage(hwndSamplerCombo, CB_RESETCONTENT, 0L, 0L);
			for (int i = 0; i < XRayMtl::NumSamplers(); i++) {
				ClassDesc* pClassD = XRayMtl::GetSamplerCD(i);
				int n = SendMessage(hwndSamplerCombo, CB_ADDSTRING, 0L, (LPARAM)(pClassD->ClassName()) );
				SendMessage(hwndSamplerCombo, CB_SETITEMDATA, n, (LPARAM)pClassD );
			}
			XRayMtl* m = (XRayMtl*)map->GetParamBlock()->GetOwner();
			//					Class_ID sampId = m->GetPixelSampler()->ClassID();

			// JohnW: Are these still necessary? removed setindx 3/23/99 ke
			macroRecorder->Disable();
			//					m->SetSamplerIndx( m->FindSampler( sampId ) );
			m->UpdateSamplingParams();
			macroRecorder->Enable();
			return TRUE;
		}
		return FALSE;
	}
	void DeleteThis() { }
};

static SamplingDlgProc samplingDlgProc;

// supersampling parameters
static ParamBlockDesc2 std2_sampling_blk ( std2_sampling, _T("samplingParameters"), 0, &stdmtl2CD, P_AUTO_CONSTRUCT + P_AUTO_UI, SAMPLING_PB_REF, 
										  //rollout
										  IDD_DMTL_SAMPLING3, IDS_KE_SAMPLING, 0, APPENDROLL_CLOSED, &samplingDlgProc, 
										  // params
										  std2_ssampler, _T("sampler"), TYPE_INT, 0,	IDS_JW_PIXELSAMPLER, 	
										  p_default, 		0, 
										  p_ui, 			TYPE_INTLISTBOX, IDC_PIX_SAMPLER, 0, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_qual, _T("samplerQuality"), TYPE_FLOAT, P_ANIMATABLE, IDS_JW_SAMPLERQUAL, 
										  p_default, 		0.5, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SAMPLEQUALITY_EDIT, IDC_SAMPLEQUALITY_SPIN, 0.01, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_enable, _T("samplerEnable"), TYPE_BOOL, P_ANIMATABLE, IDS_JW_SAMPLERENABLE, 	
										  p_default, 		FALSE, 
										  p_ui, 			TYPE_SINGLECHEKBOX, IDC_SUPER_SAMP, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_adapt_threshold, _T("samplerAdaptThreshold"), TYPE_FLOAT, 0, IDS_KE_SAMPLERADAPTTHRESH, 
										  p_default, 		0.1, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_THRESHOLD_EDIT, IDC_THRESHOLD_SPIN, 0.001, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_adapt_on, _T("samplerAdaptOn"), TYPE_BOOL, 0, IDS_KE_SAMPLERADAPTON, 	
										  p_default, 		TRUE, 
										  p_ui, 			TYPE_SINGLECHEKBOX, IDC_ADAPT_ON, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_subsample_tex_on, _T("subSampleTextureOn"), TYPE_BOOL, 0, IDS_KE_SUBSAMPLE_TEX_ON, 	
										  p_default, 		TRUE, 
										  p_ui, 			TYPE_SINGLECHEKBOX, IDC_SAMPLE_TEX, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_advanced, _T("samplerAdvancedOptions"), TYPE_BOOL, 0, IDS_KE_SAMPLERADVANCED, 	
										  p_default, 		TRUE, 
										  p_ui, 			TYPE_SINGLECHEKBOX, IDC_ADVANCED_BUTTON, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_by_name, _T("samplerByName"), TYPE_STRING, 		0, 		IDS_JW_SAMPLERBYNAME, 	
										  p_accessor,		&samplingPBAccessor,
										  end,
										  std2_ssampler_param0, _T("UserParam0"), TYPE_FLOAT, 0, IDS_KE_SAMPLER_PARAM0, 
										  p_default, 		0.0, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PARAM0_EDIT, IDC_PARAM0_SPIN, 0.01, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  std2_ssampler_param1, _T("UserParam1"), TYPE_FLOAT, 0, IDS_KE_SAMPLER_PARAM1, 
										  p_default, 		0.0, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_PARAM1_EDIT, IDC_PARAM1_SPIN, 0.01, 
										  p_accessor,		&samplingPBAccessor,
										  end, 
										  end
										  );

class XRayPBAccessor : public PBAccessor
{
public:
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		XRayMtl* m = (XRayMtl*)owner;
		switch (id)
		{
		case std2_eshader_type:
			m->eshaderId = (m->GetEShader(v.i))?v.i:-1;
			break;
		case std2_cshader_type:
			m->cshaderId = (m->GetCShader(v.i))?v.i:-1;
			break;
		case std2_gamemtl_type:
			m->gamemtlId = (m->GetGameMtl(v.i))?v.i:-1;
			break;
		case std2_eshader_by_name: {
			int idx = m->FindEShader(v.s);
			if (idx>=0) m->pb_xray->SetValue(std2_eshader_type, 0, idx);
								   }break;
		case std2_cshader_by_name: {
			int idx = m->FindCShader(v.s);
			if (idx>=0) m->pb_xray->SetValue(std2_cshader_type, 0, idx);
								   }break;
		case std2_gamemtl_by_name: {
			int idx = m->FindGameMtl(v.s);
			if (idx>=0) m->pb_xray->SetValue(std2_gamemtl_type, 0, idx);
								   }break;
		}
	}

	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval& valid)    // get into v
	{
		XRayMtl* m = (XRayMtl*)owner;
		switch (id)
		{
		case std2_eshader_by_name:{
			v.s = m->GetEShader(m->eshaderId);
								  }break;
		case std2_cshader_by_name:{
			v.s = m->GetCShader(m->cshaderId);
								  }break;
		case std2_gamemtl_by_name:{
			v.s = m->GetGameMtl(m->gamemtlId);
								  }break;
		}
	}
};

static XRayPBAccessor xrayPBAccessor;

// shader rollout dialog proc
class XRayDlgProc : public ParamMap2UserDlgProc 
{
public:
	BOOL DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) 
		{
		case WM_INITDIALOG:{
			XRayMtl* m = (XRayMtl*)map->GetParamBlock()->GetOwner();
			//  load the shader dropdown
			{
				HWND hwndEShader = GetDlgItem(hWnd, IDC_ESHADER);
				SendMessage(hwndEShader, CB_RESETCONTENT, 0L, 0L);
				for (DWORD i = 0; i < XRayMtl::EShaders.size(); i++)
				{
					int n = SendMessage(hwndEShader, CB_ADDSTRING, 0L, (LPARAM)m->GetEShader(i));
					SendMessage(hwndEShader, CB_SETITEMDATA, n, (LPARAM)i);
				}
			}
			{
				HWND hwndEShader = GetDlgItem(hWnd, IDC_CSHADER);
				SendMessage(hwndEShader, CB_RESETCONTENT, 0L, 0L);
				for (DWORD i = 0; i < XRayMtl::CShaders.size(); i++)
				{
					int n = SendMessage(hwndEShader, CB_ADDSTRING, 0L, (LPARAM)m->GetCShader(i));
					SendMessage(hwndEShader, CB_SETITEMDATA, n, (LPARAM)i);
				}
			}
			{
				HWND hwndEShader = GetDlgItem(hWnd, IDC_GAMEMTL);
				SendMessage(hwndEShader, CB_RESETCONTENT, 0L, 0L);
				for (DWORD i = 0; i < XRayMtl::GameMtls.size(); i++)
				{
					int n = SendMessage(hwndEShader, CB_ADDSTRING, 0L, (LPARAM)m->GetGameMtl(i));
					SendMessage(hwndEShader, CB_SETITEMDATA, n, (LPARAM)i);
				}
			}
		}return TRUE;
		}
		return FALSE;
	}
	void DeleteThis() { }
};


static XRayDlgProc xrayDlgProc;

// shader parameters
static ParamBlockDesc2 std2_xray_blk ( std2_xray, _T("Parameters"),  0, &stdmtl2CD, P_AUTO_CONSTRUCT + P_AUTO_UI, XRAY_PB_REF, 
									  //rollout
									  IDD_DMTL_XRAY, IDS_KE_XRAY, 0, APPENDROLL_CLOSED, &xrayDlgProc, 
									  // params
									  std2_eshader_type, _T("EShaderType"),		TYPE_INT, 		0,		IDS_JW_ESHADERTYPE,
									  p_default, 		0, 
									  p_ui, 			TYPE_INTLISTBOX, IDC_ESHADER, 0,
									  p_accessor,		&xrayPBAccessor,
									  end, 
									  std2_cshader_type, _T("CShaderType"),		TYPE_INT, 		0,		IDS_JW_CSHADERTYPE,
									  p_default, 		0, 
									  p_ui, 			TYPE_INTLISTBOX, IDC_CSHADER, 0,
									  p_accessor,		&xrayPBAccessor,
									  end, 
									  std2_gamemtl_type, _T("GameMtlType"),		TYPE_INT, 		0,		IDS_JW_GAMEMTLTYPE,
									  p_default, 		0, 
									  p_ui, 			TYPE_INTLISTBOX, IDC_GAMEMTL, 0,
									  p_accessor,		&xrayPBAccessor,
									  end, 
									  std2_eshader_by_name, _T("EShaderByName"),TYPE_STRING,	0,		IDS_JW_ESHADERBYNAME,
									  p_accessor,		&xrayPBAccessor,
									  end, 
									  std2_cshader_by_name, _T("CShaderByName"),TYPE_STRING, 	0,		IDS_JW_CSHADERBYNAME,
									  p_accessor,		&xrayPBAccessor,
									  end, 
									  std2_gamemtl_by_name, _T("GameMtlByName"),TYPE_STRING, 	0,		IDS_JW_GAMEMTLBYNAME,
									  p_accessor,		&xrayPBAccessor,
									  end, 
									  end
									  );

//////////////////////////////////////////end, sampling
static HIMAGELIST hLockButtons = NULL;

// mjm - begin - 5.10.99
class ResourceDelete
{
public:
	ResourceDelete() {}
	~ResourceDelete() { if (hLockButtons) ImageList_Destroy(hLockButtons); }
};

static ResourceDelete theResourceDelete;
// mjm - end

static void SetupPadLockButton(HWND hWnd, int id, BOOL check) 
{
	ICustButton *iBut;
	iBut = GetICustButton(GetDlgItem(hWnd, id));
	iBut->SetImage(hLockButtons, 2, 2, 2, 2, 16, 15);
	iBut->SetType(CBT_CHECK);
	ReleaseICustButton(iBut);
}

static void LoadXRayMtlResources()
{
	static BOOL loaded=FALSE;
	if (loaded) return;
	loaded = TRUE;	
	HBITMAP hBitmap, hMask;
	hLockButtons = ImageList_Create(16, 15, TRUE, 2, 0);
	hBitmap = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DMTL_BUTTONS));
	hMask   = LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DMTL_MASKBUTTONS));
	ImageList_Add(hLockButtons, hBitmap, hMask);
	DeleteObject(hBitmap);
	DeleteObject(hMask);
}

class StdMapsDlgProc : public ParamMap2UserDlgProc 
{
public:
	BOOL DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		XRayMtl* m = (XRayMtl*)map->GetParamBlock()->GetOwner();
		switch (msg) 
		{
		case WM_INITDIALOG:
			// conditionally set map amount spinner ranges and padlock button images
			LoadXRayMtlResources();
			int bumpChan = m->stdIDToChannel[ ID_BU ];
			int dispChan = m->stdIDToChannel[ ID_DP ];
			int slevChan = m->stdIDToChannel[ ID_SS ];

			for (int i = 0; i < STD2_NMAX_TEXMAPS; i++) 
			{
				if (i == bumpChan || i == dispChan || i == slevChan)
					map->SetRange(std2_map_amnts, -999, 999, i);
				else 
					map->SetRange(std2_map_amnts, 0, 100, i);
			}

			SetupPadLockButton(hWnd, IDC_LOCK_TEX01, TRUE);
			macroRecorder->Disable();
			{
				KillRefmsg kill( m->killRefmsg );
				m->UpdateLockADTex(TRUE);
			}
			macroRecorder->Enable();
			return TRUE;
		}
		return FALSE;
	}

	void DeleteThis() { }
};

static StdMapsDlgProc stdMapsDlgProc;

static BOOL IsMultipleInstanced(XRayMtl *m, Texmap *t) {
	int cnt=0;
	for (int i = 0; i < STD2_NMAX_TEXMAPS; i++) {
		if (m->maps->txmap[i].map == t)			
			cnt++;
	}
	return (cnt>1)?TRUE:FALSE;
}

#define BUMP_DEF_AMT .30f

class MapsPBAccessor : public PBAccessor
{
public:
	// the texture maps are stored, as before, in the Texmaps structure, so the parameters in the 
	// pb_map block act as 'views' onto this structure.  Any setting of map info (say from the scripter)
	// is reflected here in the Texmaps structure, and any getting of map info is extracted here from
	// this structure.

	// I hope to be able to host the texmap stuff entirely in the paramblock when I get assignable subanim
	// numbers supported in PB2's (so that the map, map enable and map amount Tab<>s are number in interleaved row order)
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		XRayMtl* m = (XRayMtl*)owner;
		switch (id)
		{
		case std2_map_enables:
			m->maps->txmap[tabIndex].mapOn = v.i;
			m->UpdateMapButtons();
			m->UpdateExtendedMapButtons();
			break;

		case std2_maps:
			{
				Texmap* tex = (Texmap*)v.r;
				m->maps->ReplaceReference(2*tabIndex+1, tex);
				if (tex != NULL) {
					macroRecorder->Disable();
					m->EnableMap(tabIndex, TRUE);
					macroRecorder->Enable();
					if (m->maps->txmap[tabIndex].amtCtrl == NULL) {			
						m->maps->ReplaceReference(2*tabIndex, NewDefaultFloatController());
						m->maps->txmap[tabIndex].amtCtrl->SetValue(TimeValue(0), &m->maps->txmap[tabIndex].amount);
					}
				} else {
					if (m->maps->txmap[tabIndex].amtCtrl != NULL)			
						m->maps->DeleteReference(2*tabIndex);
					macroRecorder->Disable();
					m->SetTexmapAmt(tabIndex, ((tabIndex == m->stdIDToChannel[ID_BU]) ? BUMP_DEF_AMT : 1.0f), TimeValue(0));
					m->EnableMap(tabIndex, FALSE);
					macroRecorder->Enable();
				}
				if (tex && (tabIndex == m->stdIDToChannel[ID_RL]|| tabIndex == m->stdIDToChannel[ID_RR]) ){
					if (!IsMultipleInstanced(m,tex)) {  //DS 4/26/99: keep settings for instanced map
						UVGen* uvg0 = tex->GetTheUVGen();
						if (uvg0 && uvg0->IsStdUVGen()) {
							StdUVGen *uvg = (StdUVGen*)uvg0;
							uvg->InitSlotType(MAPSLOT_ENVIRON);
							uvg->SetCoordMapping(UVMAP_SPHERE_ENV);
						}
					}
				}
				m->UpdateMapButtons();
				m->UpdateExtendedMapButtons();

				break;
			}

		case std2_map_amnts:
			{
				// Kill the refmessage from the map, since it will
				// be fixed later by the ref message for std2_map_amnts.
				KillRefmsg kill(m->killRefmsg);
				if (m->maps->txmap[tabIndex].amtCtrl) 
					m->maps->txmap[tabIndex].amtCtrl->SetValue(t, &v.f);
				m->maps->txmap[tabIndex].amount = v.f;
				if (tabIndex == m->stdIDToChannel[ID_BU])
					m->mInRQ = chooseReshade[m->mInRQ][IReshading::RR_NeedPreshade];
			}
			break;

		case std2_mp_ad_texlock:
			m->SetFlag(NEWSTDMTL_LOCK_ADTEX, v.i);
			m->UpdateLockADTex(TRUE);
			//			 removed to avoid multiple viewport redraws on change shader
			//				m->UpdateMtlDisplay();
			break;
		}
	}

	void Get(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t, Interval& valid)    // set from v
	{
		XRayMtl* m = (XRayMtl*)owner;
		switch (id)
		{
		case std2_map_enables:
			v.i = m->maps->txmap[tabIndex].mapOn; break;
		case std2_maps:
			v.r = m->maps->txmap[tabIndex].map; break;
		case std2_map_amnts:
			if (m->maps->txmap[tabIndex].amtCtrl) 
				m->maps->txmap[tabIndex].amtCtrl->GetValue(t, &v.f, valid);
			else
				v.f = m->maps->txmap[tabIndex].amount;
			break;
		}
	}

	// this allows 'virtual' parameters to refelect key state if in spinners and the underlying
	// source parameter is animatable (map amount in this case)
	BOOL KeyFrameAtTime(ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		if (id == std2_map_amnts)
			return ((XRayMtl*)owner)->AmtKeyAtTime(tabIndex, t);
		else
			return FALSE;
	}

};
static MapsPBAccessor mapsPBAccessor;


// std_maps parameters
static ParamBlockDesc2 std_maps_blk ( std_maps, _T("maps"),  0, &stdmtl2CD, P_AUTO_CONSTRUCT + P_AUTO_UI, MAPS_PB_REF, 
									 //rollout
									 IDD_DMTL_TEXMAP4, IDS_DS_TEXMAP, 0, APPENDROLL_CLOSED, &stdMapsDlgProc, 
									 // params
									 std2_map_enables,	_T("mapEnables"), 	TYPE_BOOL_TAB, STD2_NMAX_TEXMAPS, 		0, 				IDS_JW_MAPENABLES, 	
									 p_default, 		FALSE, 
									 p_ui, 			TYPE_SINGLECHEKBOX, IDC_USEMAP_0, IDC_USEMAP_1, IDC_USEMAP_2, IDC_USEMAP_3, 
									 IDC_USEMAP_4, IDC_USEMAP_5, IDC_USEMAP_6, IDC_USEMAP_7,
									 IDC_USEMAP_8, IDC_USEMAP_9, IDC_USEMAP_10, IDC_USEMAP_11,
									 IDC_USEMAP_12, IDC_USEMAP_13, IDC_USEMAP_14, IDC_USEMAP_15,
									 IDC_USEMAP_16, IDC_USEMAP_17, IDC_USEMAP_18, IDC_USEMAP_19,
									 IDC_USEMAP_20, IDC_USEMAP_21, IDC_USEMAP_22, IDC_USEMAP_23,
									 p_accessor,		&mapsPBAccessor,
									 end, 
									 std2_maps, 			_T("maps"), 		TYPE_TEXMAP_TAB, STD2_NMAX_TEXMAPS,	P_NO_REF,		IDS_JW_MAPS, 
									 p_ui, 			TYPE_TEXMAPBUTTON,  IDC_MAP_0, IDC_MAP_1, IDC_MAP_2, IDC_MAP_3, 
									 IDC_MAP_4, IDC_MAP_5, IDC_MAP_6, IDC_MAP_7, 
									 IDC_MAP_8, IDC_MAP_9, IDC_MAP_10, IDC_MAP_11, 
									 IDC_MAP_12, IDC_MAP_13, IDC_MAP_14, IDC_MAP_15, 
									 IDC_MAP_16, IDC_MAP_17, IDC_MAP_18, IDC_MAP_19, 
									 IDC_MAP_20, IDC_MAP_21, IDC_MAP_22, IDC_MAP_23, 
									 p_accessor,		&mapsPBAccessor,
									 end, 
									 std2_map_amnts, 	_T("mapAmounts"), 	TYPE_PCNT_FRAC_TAB, STD2_NMAX_TEXMAPS, 0, 				IDS_JW_MAPAMOUNTS, 
									 p_default, 		1.0,   // default is given in internal units   JBW 10.8.99
									 p_range, 		0.0, 100.0, 
									 p_ui, 			TYPE_SPINNER, EDITTYPE_INT, IDC_AMTEDIT_0, IDC_AMTSPIN_0, IDC_AMTEDIT_1, IDC_AMTSPIN_1, 
									 IDC_AMTEDIT_2, IDC_AMTSPIN_2, IDC_AMTEDIT_3, IDC_AMTSPIN_3, 
									 IDC_AMTEDIT_4, IDC_AMTSPIN_4, IDC_AMTEDIT_5, IDC_AMTSPIN_5, 
									 IDC_AMTEDIT_6, IDC_AMTSPIN_6, IDC_AMTEDIT_7, IDC_AMTSPIN_7, 
									 IDC_AMTEDIT_8, IDC_AMTSPIN_8, IDC_AMTEDIT_9, IDC_AMTSPIN_9, 
									 IDC_AMTEDIT_10, IDC_AMTSPIN_10, IDC_AMTEDIT_11, IDC_AMTSPIN_11, 
									 IDC_AMTEDIT_12, IDC_AMTSPIN_12, IDC_AMTEDIT_13, IDC_AMTSPIN_13, 
									 IDC_AMTEDIT_14, IDC_AMTSPIN_14, IDC_AMTEDIT_15, IDC_AMTSPIN_15, 
									 IDC_AMTEDIT_16, IDC_AMTSPIN_16, IDC_AMTEDIT_17, IDC_AMTSPIN_17, 
									 IDC_AMTEDIT_18, IDC_AMTSPIN_18, IDC_AMTEDIT_19, IDC_AMTSPIN_19, 
									 IDC_AMTEDIT_20, IDC_AMTSPIN_20, IDC_AMTEDIT_21, IDC_AMTSPIN_21, 
									 IDC_AMTEDIT_22, IDC_AMTSPIN_22, IDC_AMTEDIT_23, IDC_AMTSPIN_23, 
									 1.0, 
									 p_accessor,		&mapsPBAccessor,
									 end, 
									 std2_mp_ad_texlock, _T("adTextureLock"), 	TYPE_BOOL, 			0,				IDS_JW_ADTEXLOCK, 	
									 p_default, 		TRUE, 
									 p_ui, 			TYPE_CHECKBUTTON, IDC_LOCK_TEX01, 
									 p_accessor,		&mapsPBAccessor,
									 end, 
									 end
									 );

#ifndef DESIGN_VER
// std2_dynamics parameters
static ParamBlockDesc2 std2_dynamics_blk ( std2_dynamics, _T("dynamicsParameters"),  0, &stdmtl2CD, P_AUTO_CONSTRUCT + P_AUTO_UI, DYNAMICS_PB_REF, 
										  //rollout
										  IDD_DMTL_DYNAM, IDS_DS_DYNAMICS, 0, APPENDROLL_CLOSED, NULL, 
										  // params
										  std2_bounce, 	_T("bounce"), 	TYPE_FLOAT, 	P_ANIMATABLE, 	IDS_DS_BOUNCE, 
										  p_default, 		1.0, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BOUNCE_EDIT, IDC_BOUNCE_SPIN, 0.01, 
										  end, 
										  std2_static_friction, 	_T("staticFriction"), 	TYPE_FLOAT, 	P_ANIMATABLE, 	IDS_DS_STATFRIC, 
										  p_default, 		0.0, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_STATFRIC_EDIT, IDC_STATFRIC_SPIN, 0.01, 
										  end, 
										  std2_sliding_friction, 	_T("slidingFriction"), 	TYPE_FLOAT, 	P_ANIMATABLE, 	IDS_DS_SLIDFRIC, 
										  p_default, 		0.0, 
										  p_range, 		0.0, 1.0, 
										  p_ui, 			TYPE_SPINNER, EDITTYPE_FLOAT, IDC_SLIDFRIC_EDIT, IDC_SLIDFRIC_SPIN, 0.01, 
										  end, 
										  end
										  );
#endif

// Old Parameter block indices, kept around for old-version loading
#define PB_AMBIENT_O		0
#define PB_DIFFUSE_O		1
#define PB_SPECULAR_O		2
#define PB_SHININESS_O 		3
#define PB_SHIN_STR_O		4
#define PB_SELFI_O			5
#define PB_OPAC_O			6
#define PB_OPFALL_O			7
#define PB_FILTER_O			8
#define PB_WIRESZ_O			9
#define PB_IOR_O 			10
#define PB_BOUNCE_O			11
#define PB_STATFRIC_O		12
#define PB_SLIDFRIC_O		13
#define PB_DIMLEV_O			14
#define PB_DIMMULT_O		15
#define PB_SOFTEN_O			16
#define PB_SELFILLUM_CLR_O	17

// Ver. 10 Parameter block indices
#define PB_OPAC			0
#define PB_OPFALL		1
#define PB_FILTER 		2
#define PB_WIRESZ 		3
#define PB_IOR	 		4
#define PB_BOUNCE 		5
#define PB_STATFRIC		6
#define PB_SLIDFRIC		7
#define PB_DIMLEV		8
#define PB_DIMMULT		9

#define CURRENT_NEWSTDMTL_VERSION 12
#define FINAL_PARAMBLOCK_v1_VERSION 10

#define NPARAMS 10
#define XRAYMTL_PBVERSION	1
#define NEWSTDMTL_PBVERSION	9

// conversion descriptors for old ParamBlocks to ParamBlock2s
// here we have two version descriptors, one for parameters going into 
// new pb_extended pblock, the other for params going into the
// new pb_dynamics block.  The -1 ID's below indicate no copy,
// so we use to update calls to distribute old params from one block
// into two new blocks

static ParamBlockDescID extVer10[] = {
	{ TYPE_FLOAT, NULL, TRUE, std2_opacity },		// opacity
	{ TYPE_FLOAT, NULL, TRUE, std2_falloff_amnt },	// opfalloff
	{ TYPE_RGBA,  NULL, TRUE, std2_filter_color },	// filter
	{ TYPE_FLOAT, NULL, TRUE, std2_wire_size },		// wireSize
	{ TYPE_FLOAT, NULL, TRUE, std2_ior },			// index of refraction
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// bounce
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// static friction
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// sliding friction
	{ TYPE_FLOAT, NULL, TRUE, std2_dim_lvl },		// reflect dim level
	{ TYPE_FLOAT, NULL, TRUE, std2_refl_lvl },		// reflect dim multiplier 
};

static ParamBlockDescID dynVer10[] = {
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// opacity
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// opfalloff
	{ TYPE_RGBA,  NULL, TRUE, -1 },					// filter
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// wireSize
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// index of refraction
	{ TYPE_FLOAT, NULL, TRUE, std2_bounce },		// bounce
	{ TYPE_FLOAT, NULL, TRUE, std2_static_friction }, // static friction
	{ TYPE_FLOAT, NULL, TRUE, std2_sliding_friction }, // sliding friction
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// reflect dim level
	{ TYPE_FLOAT, NULL, TRUE, -1 },					// reflect dim multiplier 
};

// v10 Param Block Descriptor
static ParamBlockDescID stdmtl2PB[ NPARAMS ] = {
	{ TYPE_FLOAT, NULL, TRUE, 7 },   // opacity
	{ TYPE_FLOAT, NULL, TRUE, 8 }, 	// opfalloff
	{ TYPE_RGBA,  NULL, TRUE, 9 },   // filter
	{ TYPE_FLOAT, NULL, TRUE, 10 },  // wireSize
	{ TYPE_FLOAT, NULL, TRUE, 11 },  // index of refraction
	{ TYPE_FLOAT, NULL, TRUE, 12 },  // bounce
	{ TYPE_FLOAT, NULL, TRUE, 13 },  // static friction
	{ TYPE_FLOAT, NULL, TRUE, 14 },  // sliding friction
	{ TYPE_FLOAT, NULL, TRUE, 15 },  // reflect dim level
	{ TYPE_FLOAT, NULL, TRUE, 16 },  // reflect dim multiplier 
}; 

#define NPARAMS_O 17

//Old (2.5 and before) Param Block Descriptor
static ParamBlockDescID stdmtlPB[ NPARAMS_O ] = {
	{ TYPE_RGBA, NULL, TRUE, 1 },    // ambient
	{ TYPE_RGBA, NULL, TRUE, 2 },    // diffuse
	{ TYPE_RGBA, NULL, TRUE, 3 },    // specular
	{ TYPE_FLOAT, NULL, TRUE, 4 },   // shininess
	{ TYPE_FLOAT, NULL, TRUE, 5 },   // shini_strength
	{ TYPE_FLOAT, NULL, TRUE, 6 },   // self-illum
	{ TYPE_FLOAT, NULL, TRUE, 7 },   // opacity
	{ TYPE_FLOAT, NULL, TRUE, 8 }, 	// opfalloff
	{ TYPE_RGBA,  NULL, TRUE, 9 },   // filter
	{ TYPE_FLOAT, NULL, TRUE, 10 },  // wireSize
	{ TYPE_FLOAT, NULL, TRUE, 11 },  // index of refraction
	{ TYPE_FLOAT, NULL, TRUE, 12 },  // bounce
	{ TYPE_FLOAT, NULL, TRUE, 13 },  // static friction
	{ TYPE_FLOAT, NULL, TRUE, 14 },  // sliding friction
	{ TYPE_FLOAT, NULL, TRUE, 15 },  // reflect dim level
	{ TYPE_FLOAT, NULL, TRUE, 16 },  // reflect dim multiplier 
	{ TYPE_FLOAT, NULL, TRUE, 17 }   // soften
}; 

// Descriptor for mapping old (2.5 and before) shader parameters into new stdShader PB2's
// Shader::ConvertParamBlock() uses this to extract old mtl shader params into new Shader
static ParamBlockDescID stdmtlPB2[ NPARAMS_O ] = {
	{ TYPE_RGBA, NULL, TRUE, shdr_ambient },			// ambient
	{ TYPE_RGBA, NULL, TRUE, shdr_diffuse },			// diffuse
	{ TYPE_RGBA, NULL, TRUE, shdr_specular },			// specular
	{ TYPE_FLOAT, NULL, TRUE, shdr_glossiness },		// shininess
	{ TYPE_FLOAT, NULL, TRUE, shdr_spec_lvl },			// shini_strength
	{ TYPE_FLOAT, NULL, TRUE, shdr_self_illum_amnt },   // self-illum
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// opacity
	{ TYPE_FLOAT, NULL, TRUE, -1 }, 					// opfalloff
	{ TYPE_RGBA,  NULL, TRUE, -1 },						// filter
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// wireSize
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// index of refraction
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// bounce
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// static friction
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// sliding friction
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// reflect dim level
	{ TYPE_FLOAT, NULL, TRUE, -1 },						// reflect dim multiplier 
	{ TYPE_FLOAT, NULL, TRUE, shdr_soften }				// soften
}; 
#define NUMOLDVER 1
static ParamVersionDesc oldXRayMtlVersions[ NUMOLDVER ] = {
	ParamVersionDesc(stdmtl2PB, NPARAMS, 0), 
};

#define NEWSTDMTL_NUMOLDVER 9

static ParamVersionDesc oldNewStdMtlVersions[ NEWSTDMTL_NUMOLDVER+1 ] = {
	ParamVersionDesc(stdmtlPB, 8, 0), 
		ParamVersionDesc(stdmtlPB, 9, 1), 
		ParamVersionDesc(stdmtlPB, 9, 2), 
		ParamVersionDesc(stdmtlPB, 10, 3), 
		ParamVersionDesc(stdmtlPB, 11, 4), 
		ParamVersionDesc(stdmtlPB, 14, 5), 
		ParamVersionDesc(stdmtlPB, 15, 6), 
		ParamVersionDesc(stdmtlPB, 15, 7), 
		ParamVersionDesc(stdmtlPB, 16, 8), 
		ParamVersionDesc(stdmtlPB, 17, 9)
};

static ParamVersionDesc curVersion(stdmtl2PB, NPARAMS, XRAYMTL_PBVERSION);
static ParamVersionDesc stdMtlVersion(stdmtlPB, NPARAMS_O, NEWSTDMTL_PBVERSION);

#define IDT_MYTIMER 1010
#define DRAGTHRESH 6
#define DITHER_WHEN_INACTIVE // avoids palette conflict probs

//-----------------------------------------------------------------------------
//  XRayMtl
//-----------------------------------------------------------------------------
/*
TCHAR* XRayMtl::get_string(int i)
{
	return strings[i];
}

int XRayMtl::n_strings()
{
	return strings.Count();
}
*/
void* XRayMtl::GetInterface(ULONG id)
{
	if( id == IID_IReshading )
		return (IReshading*)( this );
	//	else if ( id == IID_IValidityToken )
	//		return (IValidityToken*)( this );
	else
		return Mtl::GetInterface(id);
}


void XRayMtl::Reset() 
{
	ReplaceReference( TEXMAPS_REF, xr_new<Texmaps>((MtlBase*)this));	
	ivalid.SetEmpty();

	SetShaderIndx( FindShader( Class_ID(DEFAULT_SHADER_CLASS_ID,0) ));
	// back in 4.28, as it broke reset....fixed in shader.reset so only allocs pb if not there
	pShader->Reset(); // mjm - 4.22.99 - fix mem leak (pblock already created through SetShaderIndx() above.

	SetSamplerIndx( FindSampler( Class_ID(DEFAULT_SAMPLER_CLASS_ID,0) ) ); 

	// HEY!! this should all be done automatically in ParamBlock init...
	macroRecorder->Disable();  // don't want to see this parameter reset in macrorecorder
	// ensure all map-related parameters are set up
	UpdateMapButtons();	// calls updatetexmaps
	UpdateExtendedMapButtons();

	pb_shader->SetValue(std2_wire, 0, FALSE );
	pb_shader->SetValue(std2_two_sided, 0, FALSE );
	pb_shader->SetValue(std2_faceted, 0, FALSE );
	pb_shader->SetValue(std2_face_map, 0, FALSE );

	SetDimIntens( 0.0f, 0 );
	SetDimMult( 3.0f, 0 );

	pb_extended->SetValue(std2_opacity_type, 0, 0 );
	pb_extended->SetValue(std2_falloff_type, 0, 0 );
	pb_extended->SetValue(std2_wire_units, 0, 0 );
	pb_extended->SetValue(std2_apply_refl_dimming, 0, FALSE );
	SetOpacity(1.0f, 0);
	SetFilter(Color(.5f, .5f, .5f), 0);
	SetIOR(1.5f, 0);

	// sampling
	SetSamplingQuality( 0.5f );
	SetSamplingOn( FALSE );
	pb_sampling->SetValue(std2_ssampler_subsample_tex_on, 0, TRUE );
	pb_sampling->SetValue(std2_ssampler_adapt_threshold, 0, .1f );
	pb_sampling->SetValue(std2_ssampler_adapt_on, 0, TRUE );

	SetOpacFalloff(0.0f, 0);
	SetWireSize(1.0f, 0);
	int bumpChan = stdIDToChannel[ ID_BU ];
	SetTexmapAmt(bumpChan, BUMP_DEF_AMT, 0);
	SetDynamicsProperty(0, 0, DYN_BOUNCE, 1.0f);
	SetDynamicsProperty(0, 0, DYN_STATIC_FRICTION, 0.0f);
	SetDynamicsProperty(0, 0, DYN_SLIDING_FRICTION, 0.0f);

	SetEShaderIndx(FindEShader("default"));
	SetCShaderIndx(FindCShader("default"));
	SetGameMtlIndx(FindGameMtl("default"));

	macroRecorder->Enable();
}

XRayMtl::XRayMtl(BOOL loading) : mReshadeRQ(RR_None), mInRQ(RR_None) // mjm - 06.02.00
{
	texHandle[0] = texHandle[1] = NULL;
	texHandleValid.SetEmpty();

	pixelSampler = NULL;
	samplerId = -1;

	filterOverrideOn = FALSE;
	filterSz = 1.0f;
	ioRefract = 1.5f;

	old_pblock = NULL;
	pb_xray = pb_shader = pb_extended = pb_sampling = pb_maps = pb_dynamics = NULL;	
	maps = NULL;
	filter = Color(0.0f, 0.0f, 0.0f);
	pShader = NULL;
	shaderId = -1;
	flags = NEWSTDMTL_FILT_TRANSP | NEWSTDMTL_ROLLUP1_OPEN;

	dimReflect = FALSE;
	dimIntens = 0.0f;
	dimMult = 2.0f;
	wireSize = opacity = 1.0f;
	opfall = 0.0f;
	for ( int i = 0; i < 12; ++i )
		stdIDToChannel[i] = -1;

	for ( i = 0; i < STD2_NMAX_TEXMAPS; ++i )
		channelTypes[i] = UNSUPPORTED_CHANNEL;

	ivalid.SetEmpty();

/*
	if(strings.Count() == 0)
	{
		char path[MAX_PATH];
		sprintf(path, "%s/%s", GetCOREInterface()->GetDir(APP_PLUGCFG_DIR), "strings.txt");
		FILE* fstr = fopen(path, "r");
		if(fstr != NULL)
		{
			while(!feof(fstr))
			{
				TCHAR str[100];

				_fgetts(str, 100, fstr);
				TCHAR* res_str = _tcsdup(str);
				strings.Append(1, &res_str);
			}
			fclose(fstr);
		}
	}
*/
	if (!loading){
		// ask the ClassDesc to make the P_AUTO_CONSTRUCT paramblocks
		stdmtl2CD.MakeAutoParamBlocks(this); 
		Reset();
	}
}

RefTargetHandle XRayMtl::Clone(RemapDir &remap) {
	//DebugPrint(" Cloning NEWSTDMTL %d \n", ++numNewStdMtls);
	macroRecorder->Disable();
	XRayMtl *mnew = xr_new<XRayMtl>(TRUE);
	*((MtlBase*)mnew) = *((MtlBase*)this);  // copy superclass stuff
	mnew->ReplaceReference(TEXMAPS_REF,		remap.CloneRef(maps));
	mnew->ReplaceReference(SHADER_REF,		remap.CloneRef(pShader));	
	mnew->ReplaceReference(XRAY_PB_REF,		remap.CloneRef(pb_xray));
	mnew->ReplaceReference(SHADER_PB_REF,	remap.CloneRef(pb_shader));
	mnew->ReplaceReference(EXTENDED_PB_REF, remap.CloneRef(pb_extended));
	mnew->ReplaceReference(SAMPLING_PB_REF, remap.CloneRef(pb_sampling));
	mnew->ReplaceReference(MAPS_PB_REF,		remap.CloneRef(pb_maps));
#ifndef DESIGN_VER
	mnew->ReplaceReference(DYNAMICS_PB_REF, remap.CloneRef(pb_dynamics));
#endif
	mnew->ReplaceReference(SAMPLER_REF,		remap.CloneRef(pixelSampler));	

	mnew->ivalid.SetEmpty();	
	mnew->flags = flags;

	mnew->filterOverrideOn = filterOverrideOn;
	mnew->filterSz = filterSz;

	mnew->ioRefract = ioRefract;
	mnew->opacity = opacity;
	mnew->filter = filter;
	mnew->opfall = opfall;

	mnew->wireSize = wireSize;
	mnew->dimReflect = dimReflect;

	mnew->samplerId = samplerId;
	mnew->shaderId = shaderId;

	mnew->eshaderId = eshaderId;
	mnew->cshaderId = cshaderId;
	mnew->gamemtlId = gamemtlId;

	for ( int i = 0; i < 12; ++i )
		mnew->stdIDToChannel[i] = stdIDToChannel[i];

	for ( i = 0; i < STD2_NMAX_TEXMAPS; ++i )
		mnew->channelTypes[i] = channelTypes[i];
	macroRecorder->Enable();
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}

ParamDlg* XRayMtl::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) 
{
	KillRefmsg kill( killRefmsg );

	Interval v;
	macroRecorder->Disable();
	Update(imp->GetTime(), v);
	// save the creation context for possible later shader switching requiring a new CreateParamDialog
	curHwmEdit = hwMtlEdit;
	curImp = imp;
	// create the rollouts, first the main shader dialog as master
	//  note, we can't use PB2 AUTO_UI here, since the shader's dialog is to be installed in the
	//        middle of the rollouts
	masterDlg = stdmtl2CD.CreateParamDlg(std2_shader, hwMtlEdit, imp, this);
	// then have the selected shader make its own
	pShaderDlg = pShader->CreateParamDialog(NULL, hwMtlEdit, imp, this, 
		((flags & NEWSTDMTL_ROLLUP1_OPEN) ? 0 : APPENDROLL_CLOSED));
	masterDlg->AddDlg(pShaderDlg);
	// now make the rest (they are added to the master automatically)

	if( pShader->SupportStdParams() & STD_EXTRA_DLG )
		extendedDlg = stdmtl2CD.CreateParamDlg(std2_extended, hwMtlEdit, imp, this);
	else
		extendedDlg = NULL;

	samplingDlg		= stdmtl2CD.CreateParamDlg(std2_sampling,	hwMtlEdit, imp, this);
	texmapDlg		= stdmtl2CD.CreateParamDlg(std_maps,		hwMtlEdit, imp, this);
	xrayDlg			= stdmtl2CD.CreateParamDlg(std2_xray,		hwMtlEdit, imp, this);
	UpdateTexmaps();
	if( pb_dynamics )	// not in the VIZ version
		stdmtl2CD.CreateParamDlg(std2_dynamics, hwMtlEdit, imp, this);
	// restore any saved rollout state
	stdmtl2CD.RestoreRolloutState();
	UpdateMapButtons();
	UpdateExtendedMapButtons();

	macroRecorder->Enable();
	return masterDlg;	
}

BOOL XRayMtl::SetDlgThing(ParamDlg* dlg)
{
	KillRefmsg kill( killRefmsg );

	// set the appropriate 'thing' sub-object for each secondary dialog
	if (dlg == pShaderDlg)
	{
		// handle material switching in the shader dialog, incoming material is 'this'
		assert (SuperClassID() == MATERIAL_CLASS_ID);
		assert (ClassID() == XRAYMTL_CLASS_ID);
		XRayMtl* oldMtl = (XRayMtl*)pShaderDlg->GetThing();
		ULONG oldParams, newParams;
		Class_ID newClassId, oldClassId;

		if (oldMtl) {
			Shader* oldShader = oldMtl->GetShader();
			oldParams = oldShader->SupportStdParams();
			oldClassId = oldShader->ClassID();
		} else oldParams = 0;;

		newParams = pShader->SupportStdParams();
		newClassId = pShader->ClassID();

		if ( shaderId < 0 ) {
			//it's the standin shader
			int i = -1;
			//			DbgAssert(0);
			//			SetShaderIndx( FindShader( pShader->ClassID() ), FALSE );
		}

		// see if we need to change Shader basic params rollup
		if ( (oldClassId == newClassId) || 
			((newParams & STD_BASIC2_DLG) && (oldParams & STD_BASIC2_DLG)) ){
				assert( pShaderDlg );
				// update in case of partial stdParam support
				pShaderDlg->SetThings(this, pShader);
				pShader->SetParamDlg( pShaderDlg );				
				pShaderDlg->ReloadDialog(); 
				UpdateMapButtons();
			} else {
				// different shader in this Std2Mtl, toss out the old rollup
				HWND oldPanel = pShaderDlg->GetHWnd();
				masterDlg->DeleteDlg(pShaderDlg);
				pShaderDlg->DeleteThis();

				// make a new one
				pShaderDlg = pShader->CreateParamDialog(oldPanel, curHwmEdit, curImp, this, 
					(flags & NEWSTDMTL_ROLLUP1_OPEN ? 0 : APPENDROLL_CLOSED));
				pShader->SetParamDlg(pShaderDlg);				
				pShaderDlg->LoadDialog(TRUE); 
				masterDlg->AddDlg(pShaderDlg);

				UpdateMapButtons(); // calls update texmaps
			}
	}
	else if (dlg == texmapDlg)
	{
		texmapDlg->SetThing(this);	
		UpdateTexmaps();
		UpdateLockADTex(TRUE);
	}
	else if (dlg == extendedDlg)
	{
		extendedDlg->SetThing(this);	
		UpdateExtraParams( pShader->SupportStdParams() );
		UpdateExtendedMapButtons();				
		std2_extended_blk.SetSubTexNo(std2_ep_filter_map, stdIDToChannel[ID_FI]);
	}
	else if (dlg == samplingDlg)
	{
		samplingDlg->SetThing(this);	
		if ( samplerId < 0 ) {
			DbgAssert(0);
			//			SetSamplerIndx( FindSampler( pixelSampler->ClassID() ) );
		}
		UpdateSamplingParams();
	}
	else if ( pb_dynamics == NULL )
	{
		//maybe do something here for viz case of no pb_dynamics
		int i = 9;
	}else if ( pb_xray == NULL )
	{
		//maybe do something here for viz case of no pb_dynamics
		int i = 9;
	}
	else
		return FALSE;
	return TRUE;
}

// compare function for sorting Shader Tab<>
static int __cdecl classDescListCompare(const void *elem1, const void *elem2) 
{
	ClassDesc* s1 = *(ClassDesc**)elem1;
	ClassDesc* s2 = *(ClassDesc**)elem2;
	TSTR sn1 = s1->ClassName();  // need to snap name string, since both use GetString()
	TSTR sn2 = s2->ClassName();
	return _tcscmp(sn1.data(), sn2.data());
}

void XRayMtl::LoadShaderList()
{
	// loads static shader list with name-sorted Shader ClassDesc*'s
	shaderList.ZeroCount();
	SubClassList* scList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(SHADER_CLASS_ID);
	for (long i = 0, j = 0; i < scList->Count(ACC_ALL); ++i) {
		if ( (*scList)[ i ].IsPublic() ) {
			ClassDesc* pClassD = (*scList)[ i ].CD();
			shaderList.Append(1, &pClassD);
		}
	}
	shaderList.Sort(&classDescListCompare);
}

int XRayMtl::NumShaders()
{
	if (shaderList.Count() == 0)
		LoadShaderList();
	return shaderList.Count();
}

ClassDesc* XRayMtl::GetShaderCD(int i)
{
	if (shaderList.Count() == 0)
		LoadShaderList();
	return (i >= 0 && i < shaderList.Count()) ? shaderList[i] : NULL;
}

void XRayMtl::LoadSamplerList()
{
	// loads static sampler list with name-sorted Sampler ClassDesc*'s
	samplerList.ZeroCount();
	SubClassList* scList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(SAMPLER_CLASS_ID);
	for (long i = 0, j = 0; i < scList->Count(ACC_ALL); ++i) {
		if ( (*scList)[ i ].IsPublic() ) {
			ClassDesc* pClassD = (*scList)[ i ].CD();
			samplerList.Append(1, &pClassD);
		}
	}
	samplerList.Sort(&classDescListCompare);
}

int XRayMtl::NumSamplers()
{
	if (samplerList.Count() == 0)
		LoadSamplerList();
	return samplerList.Count();
}

ClassDesc* XRayMtl::GetSamplerCD(int i)
{
	if (samplerList.Count() == 0)
		LoadSamplerList();
	return (i >= 0 && i < samplerList.Count()) ? samplerList[i] : NULL;
}


//-------------------------------------------------------------------
// DS: This class is handy for implementing UNDO - 10/31/00
#if MAX_VERSION_MAJOR<=5
	#define  SRM_CLASS_ID 0xB8073421
	class SingleRefMaker: public ReferenceMaker {
	public:
		RefTargetHandle rtarget;
		SingleRefMaker() { rtarget = NULL; }
		~SingleRefMaker() { 
			DeleteAllRefsFromMe(); 
		}
		void SetRef(RefTargetHandle rt) {
			theHold.Suspend();
			ReplaceReference(0,rt);
			theHold.Resume();
		}
		RefTargetHandle GetRef() { return rtarget; }
		// ReferenceMaker 
		RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
			PartID& partID, RefMessage message ) { 
				if (message==REFMSG_TARGET_DELETED) {
					if (hTarget==rtarget) 
						rtarget = NULL;
				}
				return REF_SUCCEED; 
			}
			void DeleteThis() { xr_delete((SingleRefMaker*)this); }

			SClass_ID  SuperClassID() { return SRM_CLASS_ID; }
			// From ref
			int NumRefs() { return 1; }
			RefTargetHandle GetReference(int i) { return rtarget; }
			void SetReference(int i, RefTargetHandle rtarg) { rtarget = rtarg;			}
			BOOL CanTransferReference(int i) {return FALSE;}
	};
#endif

//-------------------------------------------------------------------
// This handles Undo/Redo of SwitchShader

class SwitchShaderRestore: public RestoreObj {
public:
	XRayMtl *mtl;
	SingleRefMaker oldShader;
	SingleRefMaker newShader;
	SwitchShaderRestore() { mtl = NULL; }
	SwitchShaderRestore( XRayMtl * m, Shader *olds, Shader *news) {
		mtl = m;
		oldShader.SetRef(olds);
		newShader.SetRef(news);
	}
	void Restore(int isUndo) { 
		mtl->SwitchShader((Shader *)oldShader.GetRef()); 
	}
	void Redo() { 
		mtl->SwitchShader((Shader *)newShader.GetRef()); 
	}
	TSTR Description() { return(TSTR(_T("SwitchShaderRestore"))); }
};


//--------------------------------------------------------------------------


void XRayMtl::SwitchShader(Shader* newShader, BOOL loadDlg )
{
	// switch to a different shader
	SuspendAnimate();
	Class_ID oldShaderId = pShader? pShader->ClassID(): Class_ID(0,0);
	Class_ID newShaderId = newShader->ClassID();
	Class_ID constShaderId( CONSTClassID, 0 );
	if ( newShaderId != oldShaderId ) 
	{
		if ( pb_extended ) {
			int n = pb_extended->IDtoIndex( std2_opacity );
			pb_extended->RemoveController( n, 0);
		}

		theHold.Suspend(); //-----------------------------------------------------

		if (newShaderId == constShaderId ) { // constant
			SetFlag( NEWSTDMTL_FACETED, TRUE );
			pb_shader->SetValue(std2_faceted, 0, TRUE );
		} else if ( oldShaderId == constShaderId ) {
			SetFlag( NEWSTDMTL_FACETED, FALSE );
			pb_shader->SetValue(std2_faceted, 0, FALSE );
		}

		Shader * oldShader = GetShader();
		ULONG	 oldParams = oldShader ? oldShader->SupportStdParams() : 0;
		ShaderParamDlg* oldShaderDlg = oldShader ? oldShader->GetParamDlg() : loadDlg? pShaderDlg:NULL;

		ULONG	 newParams = newShader->SupportStdParams();

		// copy any params in common
		if (  oldShader && newShader ){
			ShuffleShaderParams( newShader, oldShader );
			ShuffleTexMaps( newShader, oldShader );
		}

		theHold.Resume(); //-----------------------------------------------------

		if (theHold.Holding())
			theHold.Put(xr_new<SwitchShaderRestore>(this,oldShader,newShader));  // this will make a ref to oldShader

		theHold.Suspend(); //-----------------------------------------------------

		// moved from below, or it crashes on reset
		if( oldShader) oldShader->SetParamDlg(NULL);  // DS 3/9/99 ( undo may bring oldShader back, have to clear invalid pointer)
		if(	oldShaderDlg )	oldShaderDlg->SetThings( this, NULL );

		// NB: this deletes the oldShader ( unless it being ref'd by SwitchShaderRestore)
		SetShader( newShader );


		UpdateTexmaps();
		std2_extended_blk.SetSubTexNo(std2_ep_filter_map, stdIDToChannel[ID_FI]);

		// update UI if open
		if (oldShaderDlg != NULL )
		{
			// enable or disable extra params
			UpdateExtraParams( newParams );

			// now update or replace the basic rollup
			if ((newParams & STD_BASIC2_DLG) && (oldParams & STD_BASIC2_DLG)) {
				// update in case of partial stdParam support
				oldShaderDlg->SetThings( this, newShader );
				newShader->SetParamDlg( oldShaderDlg );
				oldShaderDlg->LoadDialog(TRUE); 
				pShaderDlg = oldShaderDlg;

			} else {

				HWND oldPanel = oldShaderDlg->GetHWnd();
				masterDlg->DeleteDlg(oldShaderDlg);
				oldShaderDlg->DeleteThis();	

				// including oldPanel replaces it...
				pShaderDlg = newShader->CreateParamDialog(oldPanel, 
					curHwmEdit, curImp, this, 
					((flags & NEWSTDMTL_ROLLUP1_OPEN) ? 0 : APPENDROLL_CLOSED));
				newShader->SetParamDlg(pShaderDlg,0);
				pShaderDlg->LoadDialog(TRUE);
				masterDlg->AddDlg(pShaderDlg);

			}
			newShader->SetParamDlg(pShaderDlg,0);
			SyncADTexLock( newShader->GetLockADTex() );
			UpdateMapButtons();
			UpdateExtendedMapButtons();

			UpdateMtlDisplay();

		}// end, oldShaderDlg not NULL

		theHold.Resume(); //--------------------------------------------------

	}
	ResumeAnimate();   
}

// this is exposed at stdmat2 level
BOOL XRayMtl::SwitchShader(Class_ID shaderId)
{
	ClassDesc* pCD;
	int n = FindShader( shaderId, &pCD );
	if ( pCD ){
		//SS 1/26/2000: fix suggested by Kells
		//SwitchShader(pCD);
		SetShaderIndx(n);	// sets pb2 & set calls switchshader w/ a real shader
		return TRUE;
	} 
	return FALSE;
}


void XRayMtl::SwitchShader(ClassDesc* pNewCD)
{
	// create the new shader object
	Shader * newShader = (Shader*)(pNewCD->Create(0));
	newShader->Reset();	// creates pblock

	SwitchShader( newShader );
}


// these provide R 2.5 id access to the old shaders.
void XRayMtl::SetShading(int s)
{
	long cid;
	pb_shader->SetValue(std2_faceted,0,FALSE);
	switch( s ) {
case 0: cid = PHONGClassID; 
	pb_shader->SetValue(std2_faceted,0,TRUE);
	break;
case 1: cid = PHONGClassID; break;
case 2: cid = METALClassID; break;
case 3: cid = BLINNClassID; break;
	}
	long indx =  FindShader( Class_ID(cid, 0) );

	// this sets the new shader via pb2 accessor
	SetShaderIndx( indx );
}

// all faceted shaders approximated by constant, all new shaders by blinn.
int XRayMtl::GetShading() {
	if (IsFaceted()) return 0;
	Class_ID id = pShader->ClassID(); 
	if ( id == Class_ID(PHONGClassID,0) ) return 1;
	else if ( id == Class_ID(METALClassID,0) ) return 2;
	return 3;
}


//-------------------------------------------------------------------
// This handles Undo/Redo of SwitchSampler

class SwitchSamplerRestore: public RestoreObj, public ReferenceMaker {
public:
	XRayMtl *mtl;
	Sampler *saveSampler;
	SwitchSamplerRestore() { saveSampler = NULL; mtl = NULL; }
	SwitchSamplerRestore( XRayMtl * m, Sampler *s) {
		mtl = m;
		saveSampler = NULL;
		theHold.Suspend();
		ReplaceReference(0,s);
		theHold.Resume();
	}
	~SwitchSamplerRestore() { DeleteAllRefsFromMe();	}
	void Switch() {
		Sampler *samp = mtl->pixelSampler;

		samp->SetAFlag(A_LOCK_TARGET); // keeps sh from getting auto-deleted
		mtl->SwitchSampler(saveSampler);
		samp->ClearAFlag(A_LOCK_TARGET);

		ReplaceReference(0,samp);
	}
	void Restore(int isUndo) { Switch(); }
	void Redo() { Switch(); }
	TSTR Description() { return(TSTR(_T("SwitchSamplerRestore"))); }

	// ReferenceMaker 
	RefResult NotifyRefChanged( Interval changeInt,RefTargetHandle hTarget, 
		PartID& partID, RefMessage message ) { 
			if (message==REFMSG_TARGET_DELETED) {
				if (hTarget==saveSampler) 
					saveSampler = NULL;
			}
			return REF_SUCCEED; 
		}
		void DeleteThis() { SwitchSamplerRestore* ptr=this; xr_delete(ptr); }

		// From ref
		int NumRefs() { return 1; }
		RefTargetHandle GetReference(int i) { return saveSampler; }
		void SetReference(int i, RefTargetHandle rtarg) { 	saveSampler = (Sampler*)rtarg;	}
		BOOL CanTransferReference(int i) {return FALSE;}
};


//--------------------------------------------------------------------------

// copy of classid of the default sampler
static Class_ID defaultSamplerID( DEFAULT_SAMPLER_CLASS_ID , 0);



void XRayMtl::SwitchSampler( Sampler* newSampler )
{
	theHold.Suspend(); //-----------------------------------------------------

	DbgAssert( newSampler );
	Class_ID oldSamplerId = pixelSampler? pixelSampler->ClassID(): Class_ID(0,0);
	Class_ID newSamplerId = newSampler->ClassID();

	if ( oldSamplerId != newSamplerId ) {

		theHold.Resume(); //-----------------------------------------------------
		if (theHold.Holding())
			theHold.Put(xr_new<SwitchSamplerRestore>(this, pixelSampler));  // this will make a ref to oldShader
		theHold.Suspend(); //-----------------------------------------------------

		SetPixelSampler( newSampler );
	}

	if ( pb_sampling ) 
		UpdateSamplingParams();

	theHold.Resume(); //-----------------------------------------------------
}


void XRayMtl::SwitchSampler(ClassDesc* pNewCD)
{
	//	if ( ! pNewCD ) {
	// get default
	//		ClassDesc* pCD;
	//		int indx = FindSampler( defaultSamplerID, &pCD );
	//		pNewCD = pCD;
	//		DbgAssert( pNewCD);
	//	}
	DbgAssert( pNewCD);
	Sampler * newSampler = (Sampler*)(pNewCD->Create(0));
	SwitchSampler( newSampler );
}

// this is exposed at stdmat2 level
BOOL XRayMtl::SwitchSampler(Class_ID samplerId){
	//	ClassDesc* pCD;
	//	int n = FindSampler( samplerId, &pCD );
	int n = FindSampler( samplerId );
	//	if ( pCD ){
	if ( n >= 0 ){
		//		SwitchSampler( (Sampler*)(pCD->Create(0)) );
		SetSamplerIndx( n );
		return TRUE;
	} 
	return FALSE;
}



void XRayMtl::SetPixelSampler( Sampler * sampler )
{
	BOOL on = pixelSampler? pixelSampler->GetEnable() : FALSE;
	BOOL superTex = pixelSampler? pixelSampler->GetTextureSuperSampleOn() : TRUE;
	float quality = pixelSampler? pixelSampler->GetQuality() : 0.5f;
	ReplaceReference( SAMPLER_REF, sampler );	
	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
	if(pixelSampler) {
		pixelSampler->SetEnable( on );
		pixelSampler->SetTextureSuperSampleOn( superTex );
		pixelSampler->SetQuality( quality );
	}
}	

static TCHAR dots[] = _T(". . . . . . . . . . . . . . . . . . . . . . . .");

void XRayMtl::UpdateTexmaps()
{
	// if the UI is up for the maps paramblock, adjust button text and enables 
	// according to current shader's capabilities
	// UI controls are updated indirectly via calls on the ParamMap2.
	// further, a Param Alias table is set up for all the pmap Tab<> params for scripter access
	IParamMap2* pmap = pb_maps->GetMap();
	Shader* pShader = GetShader();
	long nShaderMapChannels = pShader->nTexChannelsSupported();
	long nMtlMapChannels = MTL_NTEXMAPS;
	pb_maps->ClearParamAliases();
	int slevChan = pShader->StdIDToChannel( ID_SS );

	for ( long i = 0; i < STD2_NMAX_TEXMAPS; ++i )
	{
		if ( i < nShaderMapChannels ) 
		{
			BOOL isSupported = ! (pShader->ChannelType( i ) & UNSUPPORTED_CHANNEL);				
			if (pmap != NULL)
			{
				TSTR chanName = maps->txmap[i].name = pShader->GetTexChannelName(i);
				// append dots 
				int n = chanName.length();
				chanName.append( _T(" " ) );
				chanName.append( &dots[ n+1 ] );
				pmap->SetText(std2_map_enables, chanName, i);
				pmap->Enable(std2_map_enables, isSupported, i);
				pmap->Enable(std2_map_amnts, isSupported, i);
				pmap->Enable(std2_maps, isSupported, i);

				if ( i == slevChan || pShader->ChannelType( i ) == SLEV_CHANNEL )
					pmap->SetRange(std2_map_amnts, -999, 999, i);
				else 
					pmap->SetRange(std2_map_amnts, 0, 100, i);

			}
			// set up the Tab<> param aliases 
			TSTR chanIntName = pShader->GetTexChannelInternalName(i);
			if (chanIntName.Length() > 0)
			{
				channelTypes[ i ] = pShader->ChannelType( i );
				pb_maps->DefineParamAlias(chanIntName, std2_maps, i);
				pb_maps->DefineParamAlias(chanIntName + _T("Enable"), std2_map_enables, i);  // JBW 5/24/99, made non-localizable (as names s/b internal)
				pb_maps->DefineParamAlias(chanIntName + _T("Amount"), std2_map_amnts, i);
			}
		} 
		else 
		{ // beyond the end of supported shader channels
			if (pmap != NULL)
			{
				// add the mtls texture channels
				if ( nMtlMapChannels > 0 ) {
					TSTR chanName = maps->txmap[i].name = GetString( mtlChannelNameIDS[ i - nShaderMapChannels ] );
					// append dots 
					int n = chanName.length();
					chanName.append( _T(" " ) );
					chanName.append( &dots[ n+1 ] );
					pmap->SetText(std2_map_enables, chanName, i);
					pmap->Enable(std2_map_enables, TRUE, i);
					pmap->Enable(std2_map_amnts, TRUE, i);
					pmap->Enable(std2_maps, TRUE, i);
					// range, first channel is bumps, last is displacement 
					if ( nMtlMapChannels == MTL_NTEXMAPS || nMtlMapChannels == 1 )
						pmap->SetRange(std2_map_amnts, -999, 999, i);
					else 
						pmap->SetRange(std2_map_amnts, 0, 100, i);

					--nMtlMapChannels;

				} else {
					// channel is not used for this mtl/shader
					pmap->SetText(std2_map_enables, dots, i);
					macroRecorder->Disable();
					SetTexmapAmt(i, 0.0f, 0);
					macroRecorder->Enable();
					pmap->Enable(std2_map_enables, FALSE, i);
					pmap->Enable(std2_map_amnts, FALSE, i);
					pmap->Enable(std2_maps, FALSE, i);
				}
			}
			TSTR chanIntName = mtlChannelInternalNames[ i - nShaderMapChannels ] ;
			if (chanIntName.Length() > 0)
			{
				channelTypes[ i ] = mtlChannelType[ i - nShaderMapChannels ];
				pb_maps->DefineParamAlias(chanIntName, std2_maps, i);
				pb_maps->DefineParamAlias(chanIntName + GetString(IDS_JW_ENABLE), std2_map_enables, i);
				pb_maps->DefineParamAlias(chanIntName + GetString(IDS_JW_AMOUNT), std2_map_amnts, i);
			}

		}
	}

	// last do the Channel Ids from stdMat
	for ( i = 0; i < N_ID_CHANNELS; ++i ){
		int chan = pShader->StdIDToChannel(i);
		if ( chan >= 0 ) {
			stdIDToChannel[i] = chan;
		} else if (mtlStdIDToChannel[ i ] >= 0){
			stdIDToChannel[i] = mtlStdIDToChannel[ i ] + pShader->nTexChannelsSupported();
		} else
			stdIDToChannel[i] = -1;
	}

	// disable for old metal
	ULONG stdParms =  pShader->SupportStdParams();
	if ( (stdParms & STD_BASIC2_DLG) &&  (stdParms & STD_PARAM_METAL) ){
		//metal std shader, turn off stuff
		int n = stdIDToChannel[ ID_SP ];
		if ( pmap ) {
			pmap->Enable(std2_map_amnts, FALSE, n );
			pmap->Enable(std2_maps, FALSE, n);
			pmap->Enable(std2_map_enables, FALSE, n);
		}
	}
	// disable filter on additive/subtractive
	if ( (stdParms & STD_EXTRA_DLG) &&  TRANSP_FILTER != GetTransparencyType() ){
		//metal std shader, turn off stuff
		int n = stdIDToChannel[ ID_FI ];
		if ( pmap ) {
			pmap->Enable(std2_map_amnts, FALSE, n );
			pmap->Enable(std2_maps, FALSE, n);
			pmap->Enable(std2_map_enables, FALSE, n);
		}
	}

}

static TCHAR* mapStates[] = { _T(" "), _T("m"),  _T("M") };

void XRayMtl::UpdateMapButtons() 
{
	UpdateTexmaps();
	if (IsShaderInUI()){
		pShader->GetParamDlg()->UpdateMapButtons();
		UpdateLockADTex(TRUE);
	}
	//	UpdateExtendedMapButtons();
}

void XRayMtl::UpdateExtendedMapButtons()
{
	IParamMap2* pmap = pb_extended->GetMap();
	if (pmap != NULL){
		int filtChan = stdIDToChannel[ID_FI];
		int state = GetMapState( filtChan );
		pmap->SetText(std2_ep_filter_map, mapStates[ state ]);

		TSTR nm	 = GetMapName( filtChan );
		pmap->SetTooltip(std2_ep_filter_map, TRUE, nm);
	}
}

void XRayMtl::UpdateSamplingParams()
{
	IParamMap2* map = pb_sampling->GetMap();
	if ( map ) {
		HWND hwnd = map->GetHWnd();
		BOOL qualityOn = pixelSampler->SupportsQualityLevels()>0 ? TRUE:FALSE ;
		map->Show(std2_ssampler_qual, qualityOn );
		HWND hwQual = GetDlgItem( hwnd, IDC_QUALITY_TEXT );
		ShowWindow( hwQual, qualityOn );

		ULONG samplerParms = pixelSampler->SupportsStdParams();
		map->Show(std2_ssampler_adapt_on, samplerParms & ADAPTIVE_CHECK_BOX );
		map->Enable(std2_ssampler_adapt_threshold, pixelSampler->IsAdaptiveOn() );
		map->Show(std2_ssampler_adapt_threshold, samplerParms & ADAPTIVE_THRESHOLD );

		map->Show(std2_ssampler_subsample_tex_on, samplerParms & SUPER_SAMPLE_TEX_CHECK_BOX );
		HWND hwThresh = GetDlgItem( hwnd, IDC_ADAPT_STATIC );
		ShowWindow( hwThresh, samplerParms & ADAPTIVE_THRESHOLD?TRUE:FALSE );
		map->Show(std2_ssampler_advanced, samplerParms & ADVANCED_DLG_BUTTON );
		SetDlgItemText( hwnd, IDC_COMMENT_TEXT, pixelSampler->GetDefaultComment() );

		map->Show(std2_ssampler_param0, samplerParms & OPTIONAL_PARAM_0 );
		if( samplerParms & OPTIONAL_PARAM_0 )
			SetDlgItemText( hwnd, IDC_PARAM0_TEXT, pixelSampler->GetOptionalParamName(0) );
		ShowWindow( GetDlgItem( hwnd, IDC_PARAM0_TEXT ), samplerParms & OPTIONAL_PARAM_0 );

		map->Show(std2_ssampler_param1, samplerParms & OPTIONAL_PARAM_1 );
		if( samplerParms & OPTIONAL_PARAM_1 )
			SetDlgItemText( hwnd, IDC_PARAM1_TEXT, pixelSampler->GetOptionalParamName(1) );
		ShowWindow( GetDlgItem( hwnd, IDC_PARAM1_TEXT ), samplerParms & OPTIONAL_PARAM_1 );
	}
}

// enable & disable extra params possibly overridden by the shader
void XRayMtl::UpdateExtraParams( ULONG stdParams ) 
{
	// update shader dlg map buttons 
	IParamMap2* pmap = pb_extended->GetMap();
	if (pmap != NULL)
	{
		HWND hRollup = pmap->GetHWnd();
		BOOL opacOn = ( stdParams & STD_EXTRA_OPACITY ) ? TRUE : FALSE;
		pmap->Show( std2_opacity_type, opacOn );
		pmap->Show( std2_falloff_type, opacOn );
		pmap->Show( std2_falloff_amnt, opacOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_OPAC_TEXT1), opacOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_OPAC_TEXT2), opacOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_OPAC_TEXT3), opacOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_FILTER_OVERLAY), !opacOn );

		BOOL reflOn = ( stdParams & STD_EXTRA_REFLECTION) ? TRUE : FALSE;
		pmap->Show( std2_apply_refl_dimming, reflOn );
		pmap->Show( std2_dim_lvl, reflOn );
		pmap->Show( std2_refl_lvl, reflOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_REFL_TEXT1), reflOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_REFL_TEXT2), reflOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_REFL_BOX), reflOn );
		//		EnableWindow( GetDlgItem(hRollup,  IDC_REFL_BOX), reflOn );

		BOOL refrOn = ( stdParams & STD_EXTRA_REFRACTION ) ? TRUE : FALSE;
		pmap->Show( std2_ior, refrOn );
		ShowWindow( GetDlgItem(hRollup,  IDC_REFR_TEXT1), refrOn );

		//		BOOL filterOn = (refrOn || opacOn) && GetTransparencyType() == TRANSP_FILTER;
		BOOL filterOn = (refrOn && !opacOn) || (opacOn && GetTransparencyType() == TRANSP_FILTER);
		pmap->Show( std2_filter_color, filterOn );
		pmap->Show( std2_ep_filter_map, filterOn );
		EnableWindow( GetDlgItem(hRollup,  IDC_OPAC_BOX), (refrOn || opacOn) );
	}
}

int XRayMtl::GetMapState( int nMap ) 
{
	Texmap *t = (*maps)[nMap].map;
	if ( t == NULL ) return 0;
	return (*maps)[nMap].mapOn ? 2 :1; 
}


TSTR XRayMtl::GetMapName( int nMap ) 
{
	Texmap *t = (*maps)[nMap].map;
	if ( t == NULL )
		return TSTR( GetString( IDS_KE_NONE ) );
	return t->GetFullName(); 
}


void XRayMtl::SyncADTexLock( BOOL lockOn ) 
{
	pb_maps->SetValue(std2_mp_ad_texlock, 0, lockOn );
	UpdateLockADTex(FALSE);
	//	SetFlag( NEWSTDMTL_LOCK_ADTEX, lockOn );
	// force an update
	//	if (masterDlg)
	//		masterDlg->ReloadDialog();
}

void XRayMtl::UpdateLockADTex( BOOL passOn ) 
{
	IParamMap2* pmap = pb_maps->GetMap();
	int lock = 	GetFlag(NEWSTDMTL_LOCK_ADTEX)!=0;
	if ( pShader->SupportStdParams() & STD_PARAM_LOCKADTEX ) {
		if ( passOn ){
			pShader->SetLockADTex( lock );
			if (pmap != NULL && pShader->GetParamDlg())
				pShader->GetParamDlg()->LoadDialog(TRUE);
		}
		if (pmap != NULL)
		{
			pmap->Show(std2_mp_ad_texlock, TRUE);
			pmap->Enable(std2_maps, !lock, 0);
		}
	} else {
		// no std params, so no amb/diff lock
		if (pmap != NULL)
		{
			pmap->Show(std2_mp_ad_texlock, FALSE);
			pmap->Enable(std2_maps, TRUE, 0);
		}
	}

}

void XRayMtl::UpdateMtlDisplay() 
{
	IParamMap2* map = pb_maps->GetMap();
	if (map != NULL) 
	{
		//	 removed to avoid multiple viewport redraws on change shader
		//		map->RedrawViews(GetCOREInterface()->GetTime());
		map->Invalidate();
	}
}

#define LIMIT0_1(x) if (x<0.0f) x = 0.0f; else if (x>1.0f) x = 1.0f;
#define LIMIT1(x) if (x > 1.0f) x = 1.0f;
#define LIMITMINMAX(x, min, max) if (x<min) x = min; else if (x>max) x = max;

static Color LimitColor(Color c) {
	LIMIT0_1(c.r);
	LIMIT0_1(c.g);
	LIMIT0_1(c.b);
	return c;
}

void XRayMtl::Update(TimeValue t, Interval &valid) {
	Point3 p;
	if (!ivalid.InInterval(t)) {
		ivalid.SetInfinite();

		pb_extended->GetValue(std2_filter_color, t, p, ivalid );
		filter = LimitColor(Color(p.x, p.y, p.z));
		pb_extended->GetValue(std2_ior, t, ioRefract, ivalid );
		pb_extended->GetValue(std2_opacity, t, opacity, ivalid );
		LIMIT0_1(opacity);

		if (IsShaderInUI())
			pShader->GetParamDlg()->UpdateOpacity(); 

		pb_extended->GetValue(std2_falloff_amnt, t, opfall, ivalid );
		LIMIT0_1(opfall);

		pb_extended->GetValue(std2_wire_size, t, wireSize, ivalid );
		pb_extended->GetValue(std2_dim_lvl, t, dimIntens, ivalid );
		LIMIT0_1(dimIntens);
		pb_extended->GetValue(std2_refl_lvl, t, dimMult, ivalid );

		SetFlag(NEWSTDMTL_WIRE, pb_shader->GetInt(std2_wire, t));
		SetFlag(NEWSTDMTL_2SIDE, pb_shader->GetInt(std2_two_sided, t));
		SetFlag(NEWSTDMTL_FACEMAP, pb_shader->GetInt(std2_face_map, t));
		SetFlag(NEWSTDMTL_FACETED, pb_shader->GetInt(std2_faceted, t));

		SetFlag(NEWSTDMTL_ADD_TRANSP, pb_extended->GetInt(std2_opacity_type, t) == 2);
		SetFlag(NEWSTDMTL_FILT_TRANSP, pb_extended->GetInt(std2_opacity_type, t) == 0);
		SetFlag(NEWSTDMTL_FALLOFF_OUT, pb_extended->GetInt(std2_falloff_type, t) == 1);
		SetFlag(NEWSTDMTL_WIRE_UNITS, pb_extended->GetInt(std2_wire_units, t) == 1);
		SetFlag(NEWSTDMTL_LOCK_ADTEX, pb_maps->GetInt(std2_mp_ad_texlock, t) == 1);
		dimReflect = pb_extended->GetInt(std2_apply_refl_dimming, t);

		if (pShader)
			pShader->Update( t, ivalid );

		for (int i = 0; i < STD2_NMAX_TEXMAPS; i++)
			if (MAPACTIVE(i)) 
				maps->txmap[i].Update(t, ivalid);

	}
	valid &= ivalid;
}

BOOL XRayMtl::AmtKeyAtTime(int i, TimeValue t) {
	if (maps->txmap[i].amtCtrl) 
		return 	maps->txmap[i].amtCtrl->IsKeyAtTime(t,0);
	else
		return FALSE;
}

typedef struct {
	float amount;
	Control *amtCtrl;  
	Texmap *map;       
	BOOL mapOn;
	TSTR name;
} saveChan;

void XRayMtl::ShuffleTexMaps( Shader* newShader, Shader* oldShader )
{
	saveChan oldMaps[STD2_NMAX_TEXMAPS];

	for( long i = 0; i < STD2_NMAX_TEXMAPS; ++i ){
		oldMaps[i].amtCtrl = maps->txmap[i].amtCtrl; maps->txmap[i].amtCtrl = NULL; 
		oldMaps[i].amount = maps->txmap[i].amount; maps->txmap[i].amount = 1.0f; 
		oldMaps[i].map = maps->txmap[i].map; maps->txmap[i].map = NULL; 
		oldMaps[i].mapOn = maps->txmap[i].mapOn; maps->txmap[i].mapOn = FALSE; 
		oldMaps[i].name = maps->txmap[i].name; maps->txmap[i].name = _T(""); 
	}
	int nOldShadeMaps = oldShader->nTexChannelsSupported();
	int nNewShadeMaps = newShader->nTexChannelsSupported();

	// for each new shader map
	for( int oldChan, newChan = 0; newChan < nNewShadeMaps; ++newChan ){

		TSTR newName = newShader->GetTexChannelInternalName(newChan);

		// look for match in each old map
		for ( oldChan = 0; oldChan < nOldShadeMaps; ++oldChan ) {
			if ( newName == oldShader->GetTexChannelInternalName(oldChan) )
				break;
		}

		if( oldChan < nOldShadeMaps ) {	// found it
			maps->txmap[newChan].amtCtrl = oldMaps[oldChan].amtCtrl; 
			maps->txmap[newChan].amount = oldMaps[oldChan].amount;
			maps->txmap[newChan].map = oldMaps[oldChan].map;
			maps->txmap[newChan].mapOn = oldMaps[oldChan].mapOn;
			maps->txmap[newChan].name = oldMaps[oldChan].name;
		}

	}// end, for each new map

	// now do the mtl maps
	for ( int n = 0; n < MTL_NTEXMAPS; ++n ){
		newChan = n + nNewShadeMaps;
		oldChan = n + nOldShadeMaps;
		maps->txmap[newChan].amtCtrl = oldMaps[oldChan].amtCtrl; 
		maps->txmap[newChan].amount = oldMaps[oldChan].amount;
		maps->txmap[newChan].map = oldMaps[oldChan].map;
		maps->txmap[newChan].mapOn = oldMaps[oldChan].mapOn;
		maps->txmap[newChan].name = oldMaps[oldChan].name;
	}
}

void XRayMtl::ShuffleShaderParams( Shader* newShader, Shader* oldShader )
{
	ClassDesc2 *oldCD2 = NULL, *newCD2 = NULL;

	ClassDesc * newCD;
	FindShader( newShader->ClassID(), &newCD );

	ClassDesc * oldCD;
	FindShader( oldShader->ClassID(), &oldCD );

	// check if we have a ParamBlock2-based shaders 
	if (oldCD != NULL && oldCD->InternalName() != NULL)
		oldCD2 = (ClassDesc2*)oldCD;
	if (newCD != NULL && newCD->InternalName() != NULL)
		newCD2 = (ClassDesc2*)newCD;

	// loop over destination shaders PB2-based parameters
	if (oldCD2 != NULL && newCD2 != NULL){
		// loop over each block in the new shader
		for (int i = 0; i < newCD2->NumParamBlockDescs(); i++){
			// get block descriptor and source paramblock
			ParamBlockDesc2* newPBdesc = newCD2->GetParamBlockDesc(i);
			IParamBlock2* newPB2 = newShader->GetParamBlockByID(newPBdesc->ID);
			// loop over each new param and copy from old if there's a match
			for (int j = 0; j < newPBdesc->count; j++){
				ParamDef& paramDef = newPBdesc->paramdefs[j];

				// Now search the old shaders blocks
				for (int i = 0; i < oldCD2->NumParamBlockDescs(); i++){
					ParamBlockDesc2* oldPBdesc = oldCD2->GetParamBlockDesc(i);
					int j = oldPBdesc->NameToIndex(paramDef.int_name);
					if (j >= 0){
						// found an internal name match, get the dest pblock and param def
						ParamDef& srcParamDef = oldPBdesc->paramdefs[j];
						// copy only if types match
						if (srcParamDef.type == paramDef.type){
							IParamBlock2* oldPB2 = oldShader->GetParamBlockByID(oldPBdesc->ID);
							newPB2->Assign(paramDef.ID, oldPB2, srcParamDef.ID);
							goto nextParam;	// found it, outta here...
						}

					}
				}
nextParam:		;
			}// end, dest param loop
		}// end, desk block loop
		Interval v;
		v.SetInfinite();
		newShader->Update(0, v);

		// end, pb2 copying
	} else {

		// StdParam based copy
		newShader->CopyStdParams( oldShader );
	}
}

float XRayMtl::GetXParency(int mtlNum, BOOL backFace) { 
	int opacChan = stdIDToChannel[ ID_OP ];

	// When the mtl has "Show Map in Viewport" turned on, and has an opacity map,
	// the opacity map over-rides the opacity.  DS 6/16/00
	// (Actually, when the hardward doesn't support enough texture channels to do
	// the opacity map, the overide is only true for NON-decal maps).
	// 
	if (((*maps)[opacChan].map&&MAPACTIVE(opacChan)&&Active())) {
		return 0.0f;
	}

	// The following statement looks dubious -- what is it trying to do? -- DS 6/16/00
	if(opacity>0.9f && opfall>0.0f) 
		return 0.1f; 
	else 
		return 1.0f-opacity; 
}

ULONG XRayMtl::Requirements(int subMtlNum) {
	ULONG req;

	assert( pShader );
	req = pShader->GetRequirements(subMtlNum);

	int opacChan = stdIDToChannel[ ID_OP ];
	int bumpChan = stdIDToChannel[ ID_BU ];
	int dispChan = stdIDToChannel[ ID_DP ];

	if (opacity!=1.0f||MAPACTIVE(opacChan)||opfall>0.0f) {
		req |= MTLREQ_TRANSP;
	}

	if (opacity!=1.0f|| ((*maps)[opacChan].map&&MAPACTIVE(opacChan)&&Active()))
		req |= MTLREQ_TRANSP_IN_VP;

	for (int i=0; i<STD2_NMAX_TEXMAPS; i++) {
		if (MAPACTIVE(i))	
			req |= (*maps)[i].map->Requirements(subMtlNum);
	}
	if (MAPACTIVE(bumpChan)) {
		ULONG bmpreq = (*maps)[bumpChan].map->Requirements(subMtlNum);
		if (bmpreq&MTLREQ_UV)
			req |= MTLREQ_BUMPUV;
		if (bmpreq&MTLREQ_UV2)
			req |= MTLREQ_BUMPUV2;
	}
	if (flags&NEWSTDMTL_WIRE) 	req|= MTLREQ_WIRE;
	if (flags&NEWSTDMTL_2SIDE) req|= MTLREQ_2SIDE;
	//	if (pixelSampler && pixelSampler->GetEnable() )
	if ( flags&NEWSTDMTL_SSAMP_ON )
		req|= MTLREQ_SUPERSAMPLE;
	if (flags&NEWSTDMTL_WIRE_UNITS) req|= MTLREQ_WIRE_ABS;
	if (flags&NEWSTDMTL_FACEMAP) req |= MTLREQ_FACEMAP;
	if (flags&NEWSTDMTL_ADD_TRANSP) req |= MTLREQ_ADDITIVE_TRANSP;
	if (flags&NEWSTDMTL_FACETED) req |= MTLREQ_FACETED;
	if (MAPACTIVE(dispChan)) req |= MTLREQ_DISPLACEMAP;
	return req;		
}

int XRayMtl::MapSlotType(int i) 
{ 
	if (i==stdIDToChannel[ID_DP]) return MAPSLOT_DISPLACEMENT;  // DS 4/12/99
	return (i==stdIDToChannel[ID_RL] || i==stdIDToChannel[ID_RR])?
MAPSLOT_ENVIRON : MAPSLOT_TEXTURE; 
}

void XRayMtl::MappingsRequired(int subMtlNum, BitArray & mapreq, BitArray &bumpreq) {
	int bumpChan = stdIDToChannel[ ID_BU ];
	for (int i=0; i<STD2_NMAX_TEXMAPS; i++) {
		if (MAPACTIVE(i)) {	
			if (i==bumpChan) 
				(*maps)[i].map->MappingsRequired(subMtlNum,bumpreq,bumpreq);
			else 
				(*maps)[i].map->MappingsRequired(subMtlNum,mapreq,bumpreq);
		}
	}
}

Interval XRayMtl::Validity(TimeValue t) {
	Interval v;
	Update(t, v);
	return ivalid;
}

void XRayMtl::NotifyChanged() {
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void XRayMtl::SetFlag(ULONG f, ULONG val) {
	if (val) flags|=f; 
	else flags &= ~f;
}

void XRayMtl::SetTransparencyType(int type) {
	switch (type) {
case TRANSP_SUBTRACTIVE:  
	SetFlag(NEWSTDMTL_ADD_TRANSP, 0); 
	SetFlag(NEWSTDMTL_FILT_TRANSP, 0);
	pb_extended->SetValue(std2_opacity_type, 0, 1);
	break;
case TRANSP_ADDITIVE:  
	SetFlag(NEWSTDMTL_ADD_TRANSP, 1); 
	SetFlag(NEWSTDMTL_FILT_TRANSP, 0); 
	pb_extended->SetValue(std2_opacity_type, 0, 2);
	break;
case TRANSP_FILTER:  
	SetFlag(NEWSTDMTL_ADD_TRANSP, 0); 
	SetFlag(NEWSTDMTL_FILT_TRANSP, 1); 
	pb_extended->SetValue(std2_opacity_type, 0, 0);
	break;
	}
}

void XRayMtl::DeleteThis() 
{
	xr_delete((XRayMtl*)this);
}


TSTR XRayMtl::SubAnimName(int i) { 
	switch(i){
case TEXMAPS_SUB: return TSTR(GetString( IDS_DS_TEXMAPS ));
case SHADER_SUB: return TSTR(GetString( IDS_KE_SHADER ));
case EXTRA_PB_SUB:	return TSTR(GetString( IDS_DS_EXTRA ));
case SAMPLING_PB_SUB:	return TSTR(GetString( IDS_KE_SAMPLING ));
case DYNAMICS_PB_SUB:	return TSTR(GetString( IDS_DS_DYNAMICS ));
	}
	return TSTR("");
}		

Animatable* XRayMtl::SubAnim(int i) {
	switch(i) {
case TEXMAPS_SUB: return maps;
case SHADER_SUB: return pShader;
case EXTRA_PB_SUB:	return pb_extended;
case SAMPLING_PB_SUB:	return pb_sampling;
case DYNAMICS_PB_SUB:	return pb_dynamics;
default: DbgAssert(0); return NULL;
	}
}

int XRayMtl::SubNumToRefNum(int subNum) 
{ 
	switch (subNum)
	{
	case TEXMAPS_SUB: return TEXMAPS_REF;
	case SHADER_SUB: return SHADER_REF;
	case EXTRA_PB_SUB:	return EXTENDED_PB_REF;
	case SAMPLING_PB_SUB:	return SAMPLING_PB_REF;
	case DYNAMICS_PB_SUB:	return DYNAMICS_PB_REF;
	default: DbgAssert(0); return 0;
	}
}

IParamBlock2* 
XRayMtl::GetParamBlock(int i)
{
	switch (i)
	{
	case 0:	return pb_shader;
	case 1:	return pb_extended;
	case 2:	return pb_sampling;
	case 3:	return pb_maps;
	case 4:	return pb_dynamics;
	case 5: return pb_xray;
	}
	return NULL;
}

IParamBlock2* 
XRayMtl::GetParamBlockByID(BlockID id) 
{ 
	// return id'd ParamBlock	
	switch (id)
	{
	case std2_shader:	return pb_shader;
	case std2_extended:	return pb_extended;
	case std2_sampling:	return pb_sampling;
	case std_maps:		return pb_maps;
	case std2_dynamics:	return pb_dynamics;
	case std2_xray:		return pb_xray;
	}
	return NULL; 
}

RefTargetHandle XRayMtl::GetReference(int i) {
	switch(i) {
case OLD_PBLOCK_REF:	return old_pblock;  // old pblock, replaced by the 6 new pb2's
case TEXMAPS_REF:		return maps;
case SHADER_REF:		return pShader;
case SHADER_PB_REF:		return pb_shader;
case EXTENDED_PB_REF:	return pb_extended;
case SAMPLING_PB_REF:	return pb_sampling;
case MAPS_PB_REF:		return pb_maps;
#ifndef DESIGN_VER
case DYNAMICS_PB_REF:	return pb_dynamics;
#else
case DYNAMICS_PB_REF:	return NULL;
#endif
case XRAY_PB_REF:		return pb_xray;
case SAMPLER_REF:		return pixelSampler;
default: assert(0);		return NULL;
	}
}

void XRayMtl::SetReference(int i, RefTargetHandle rtarg) {
	switch(i) {
case OLD_PBLOCK_REF:	old_pblock = (IParamBlock*)rtarg; return;
case TEXMAPS_REF:	{
	maps = (Texmaps*)rtarg; 
	if (maps != NULL)
		maps->client = this;
	return;
					}
case SHADER_REF:	
	if (IsShaderInUI()){
		pShader->GetParamDlg()->SetThings(this, NULL);
		pShader->SetParamDlg(NULL);	
	}
	pShader = (Shader*)rtarg; 

	return;
case SHADER_PB_REF:		pb_shader		= (IParamBlock2*)rtarg; return;
case EXTENDED_PB_REF:	pb_extended		= (IParamBlock2*)rtarg; return;
case SAMPLING_PB_REF:	pb_sampling		= (IParamBlock2*)rtarg; return;
case MAPS_PB_REF:		pb_maps			= (IParamBlock2*)rtarg; return;
case DYNAMICS_PB_REF:	pb_dynamics		= (IParamBlock2*)rtarg; return;
case XRAY_PB_REF:		pb_xray			= (IParamBlock2*)rtarg; return;
case SAMPLER_REF:		pixelSampler	= (Sampler*)rtarg; return;
default: assert(0);
	}
}

void XRayMtl::SetSubTexmap(int i, Texmap *m)
{
	assert(i < STD2_NMAX_TEXMAPS);
	pb_maps->SetValue(std2_maps, TimeValue(0), m, i);
}

// mjm - begin - 06.02.00
void XRayMtl::UpdateReshadeRequirements(RefTargetHandle hTarget, PartID partID)
{
	if (!killRefmsg.DistributeRefmsg())
	{
		mReshadeRQ = RR_None;
	}
	else if (hTarget == maps)
	{
		mReshadeRQ = RR_NeedPreshade;
	}
	else if (hTarget == pShader)
	{
		mReshadeRQ = RR_NeedReshade;
	}
	else if (hTarget == pb_xray)
	{
		mReshadeRQ = RR_None;
	}
	else if (hTarget == pb_shader)
	{
		mReshadeRQ = RR_NeedReshade;
	}
	else if (hTarget == pb_extended)
	{
		mReshadeRQ = RR_NeedReshade;
	}
	else if (hTarget == pb_sampling)
	{
		mReshadeRQ = RR_None;
	}
	else if (hTarget == pb_maps)
	{
		if ( pb_maps->LastNotifyParamID() == std2_map_amnts)
			mReshadeRQ = RR_NeedReshade;
		else
			mReshadeRQ = RR_NeedPreshade;
	}
	else if (hTarget == pb_dynamics)
	{
		mReshadeRQ = RR_None;
	}
	else if (hTarget == pixelSampler)
	{
		mReshadeRQ = RR_None;
	}

	mReshadeRQ = chooseReshade[mReshadeRQ][mInRQ];
	mInRQ = RR_None;
}
// mjm - end

// invaldate for the viewport
RefResult XRayMtl::NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, 
									   PartID& partID, RefMessage message ) 
{
	switch (message) {
case REFMSG_WANT_SHOWPARAMLEVEL:
	{
		BOOL *pb = (BOOL *)(partID);
		*pb = TRUE;
		return REF_STOP;
	}
case REFMSG_CHANGE:
	UpdateReshadeRequirements(hTarget, partID); // mjm - 06.02.00

	ivalid.SetEmpty();
	// ask ClassDesc if any paramblock change caused the notify
	// if not and changing ref was one of the pblocks, inval all PB2 UI
	// else inval indicated param UI
	IParamBlock2* cpb;
	ParamID changing_param = stdmtl2CD.LastNotifyParamID(this, cpb);
	if (hTarget != pShader) // && hTarget != maps)
	{
		if (changing_param != -1)
		{
			// DS 10/13/00: I needed to add code to check of the dialog for this map was actualy active because 
			// on during the creation of new materials, messages were getting routed to the current
			// medit material, causing problems for undo of sub-object mtl assignment.
			// DS 10/24/31:  Removed the test because it isn't quite right, and was causing some problems ( #264662)
			//  and no longer seems necessary. (we need a better test for whether a mtl is in medit or not)  
			//if (cpb->GetMap()&&cpb->GetMap()->DlgActive())) 
			cpb->GetDesc()->InvalidateUI(changing_param);
			if (changing_param == std2_opacity && 
				cpb == pb_extended && pShader &&
				pShader->GetParamDlg())
				pShader->GetParamDlg()->UpdateOpacity(); 
		}
		else
		{
			stdmtl2CD.InvalidateUI();
			if (hTarget == pb_extended && pShader &&
				pShader->GetParamDlg())
				pShader->GetParamDlg()->UpdateOpacity();
			if (texHandle[0]||texHandle[1]) {
				if (curImp)
					curImp->MtlChanged();  
			}
		}
		DiscardTexHandles(); // DS 5/22/00
	}
	break;
	}

	return REF_SUCCEED;
}

Class_ID XRayMtl::ClassID() { return XRAYMTL_CLASS_ID; }

TSTR XRayMtl::GetSubTexmapSlotName(int i) {
	if ( pShader ){
		long nShaderChan = pShader->nTexChannelsSupported();
		if ( i < nShaderChan )
			return pShader->GetTexChannelName( i );
		else
			return GetString( mtlChannelNameIDS[i - nShaderChan] );
	} else return TSTR("");
}


void XRayMtl::EnableMap(int i, BOOL onoff) { 
	pb_maps->SetValue(std2_map_enables, TimeValue(0), onoff, i);
}

void XRayMtl::SetFilter(Color c, TimeValue t) {
	filter = c;
	pb_extended->SetValue( std2_filter_color, t, Point3(c.r, c.g, c.b));
	int opacChan = stdIDToChannel[ ID_OP ];
	if (opacity!=1.0f||opfall!=0.0f||MAPACTIVE(opacChan)) 
		NotifyChanged();
}

void XRayMtl::SetIOR(float v, TimeValue t) {
	ioRefract = v;
	pb_extended->SetValue(std2_ior, t, v);
}

void XRayMtl::SetOpacity(float v, TimeValue t) {
	opacity = v;
	pb_extended->SetValue(std2_opacity, t, v);
}

void XRayMtl::SetOpacFalloff(float v, TimeValue t) {
	opfall = v;
	pb_extended->SetValue(std2_falloff_amnt, t, v);
	int opacChan = stdIDToChannel[ ID_OP ];
	if (opacity!=1.0f||opfall!=0.0f||MAPACTIVE(opacChan)) 
		NotifyChanged();
}		

void XRayMtl::SetWireSize(float v, TimeValue t) {
	wireSize = v;
	pb_extended->SetValue(std2_wire_size, t, v);
	if (flags&NEWSTDMTL_WIRE)
		NotifyChanged();
}

void XRayMtl::SetDimIntens(float v, TimeValue t) {
	dimIntens = v;
	pb_extended->SetValue(std2_dim_lvl, t, v);
}

void XRayMtl::SetDimMult(float v, TimeValue t) {
	dimMult = v;
	pb_extended->SetValue(std2_refl_lvl, t, v);
}

void XRayMtl::SetTexmapAmt(int imap, float amt, TimeValue t) {
	pb_maps->SetValue(std2_map_amnts, t, amt, imap);
}

/////////////////////////////////////////////////////////////////////
// shaders
void XRayMtl::SetShaderIndx( long indx, BOOL update ) 
{
	if( !update ) shaderId = NO_UPDATE; 
	pb_shader->SetValue(std2_shader_type, 0, indx<0 ? 0 : indx );
	shaderId = indx; 
}

void XRayMtl::SetSamplerIndx( long indx, BOOL update )
{
	if( !update ) samplerId = NO_UPDATE;
	pb_sampling->SetValue(std2_ssampler, 0, indx<0 ? 0 : indx );
	samplerId = indx; 
}


static Class_ID ClassID0(0, 0);
static Class_ID ClassID1(1, 0);

void XRayMtl::SetShader( Shader* pNewShader )
{
	//	if ( (pShader?pShader->ClassID():ClassID0) != (pNewShader?pNewShader->ClassID():ClassID1) ){
	ReplaceReference( SHADER_REF, pNewShader );	
	NotifyChanged();
	NotifyDependents(FOREVER, 0, REFMSG_SUBANIM_STRUCTURE_CHANGED);
	//	}
}

// the shader Gets & Sets
BOOL XRayMtl::IsSelfIllumColorOn()
{	if (pShader->SupportStdParams() & STD_PARAM_SELFILLUM_CLR){
	return pShader->IsSelfIllumClrOn();
}
return FALSE;
}

void XRayMtl::SetSelfIllumColorOn( BOOL on )
{	if (pShader->SupportStdParams() & STD_PARAM_SELFILLUM_CLR){
	pShader->SetSelfIllumClrOn( on );
}
}

void XRayMtl::SetAmbient(Color c, TimeValue t)		
{	if (pShader->SupportStdParams() & STD_PARAM_AMBIENT_CLR){
	pShader->SetAmbientClr( c, t );
}
}

void XRayMtl::SetDiffuse(Color c, TimeValue t)		
{	if (pShader->SupportStdParams() & STD_PARAM_DIFFUSE_CLR){
	pShader->SetDiffuseClr( c, t );
}
}

void XRayMtl::SetSpecular(Color c, TimeValue t)
{	if (pShader->SupportStdParams() & STD_PARAM_SPECULAR_CLR){
	pShader->SetSpecularClr( c, t );
}
}

void XRayMtl::SetShininess(float v, TimeValue t)	
{	if (pShader->SupportStdParams() & STD_PARAM_GLOSSINESS ){
	pShader->SetGlossiness( v, t );
}
}

void XRayMtl::SetShinStr(float v, TimeValue t)		
{	if (pShader->SupportStdParams() & STD_PARAM_SPECULAR_LEV ){
	pShader->SetSpecularLevel( v, t );
}
}

void XRayMtl::SetSelfIllum(float v, TimeValue t)
{	if (pShader->SupportStdParams() & STD_PARAM_SELFILLUM ){
	pShader->SetSelfIllum( v, t );
}
}


void XRayMtl::SetSelfIllumColor(Color c, TimeValue t)
{	if (pShader->SupportStdParams() & STD_PARAM_SELFILLUM_CLR){
	pShader->SetSelfIllumClr( c, t );
}
}

static Color blackClr( 0.0, 0.0, 0.0 );

Color XRayMtl::GetAmbient(int mtlNum, BOOL backFace) 
{
	return pShader->GetAmbientClr( mtlNum, backFace );
}

Color XRayMtl::GetDiffuse(int mtlNum, BOOL backFace)
{	
	return pShader->GetDiffuseClr( mtlNum, backFace );
}

Color XRayMtl::GetSpecular(int mtlNum, BOOL backFace) 
{	
	return pShader->GetSpecularClr( mtlNum, backFace );
}

Color XRayMtl::GetSelfIllumColor(int mtlNum, BOOL backFace) 
{	
	return pShader->GetSelfIllumClr( mtlNum, backFace );
}


float XRayMtl::GetShininess(int mtlNum, BOOL backFace) 	
{	
	float g = pShader->GetGlossiness( mtlNum, backFace );
	return Bound( g, 0.01f, 1.0f );
}

float XRayMtl::GetShinStr(int mtlNum, BOOL backFace) 
{	
	return pShader->GetSpecularLevel( mtlNum, backFace );
}

float XRayMtl::GetSelfIllum(int mtlNum, BOOL backFace)		
{	
	return pShader->GetSelfIllum( mtlNum, backFace );
}

float XRayMtl::GetShininess(TimeValue t) 	
{	
	return pShader->GetGlossiness( t );
}

float XRayMtl::GetShinStr(TimeValue t) 
{	
	return pShader->GetSpecularLevel( t );
}

float XRayMtl::GetSelfIllum(TimeValue t)		
{	
	return pShader->GetSelfIllum(t);
}


BOOL XRayMtl::GetSelfIllumColorOn(int mtlNum, BOOL backFace)		
{	
	return pShader->IsSelfIllumClrOn();
}

Color XRayMtl::GetAmbient(TimeValue t) 	
{	
	return pShader->GetAmbientClr( t );
}
Color XRayMtl::GetDiffuse(TimeValue t) 	
{	
	return pShader->GetDiffuseClr(t);
}
Color XRayMtl::GetSpecular(TimeValue t) 
{
	return pShader->GetSpecularClr( t );
}
Color XRayMtl::GetSelfIllumColor(TimeValue t) 		
{
	return pShader->GetSelfIllumClr(t );
}

float XRayMtl::GetSoftenLevel( TimeValue t)  
{	
	return pShader->GetSoftenLevel(t);
}

Color XRayMtl::GetFilter() {	return filter;	}
float XRayMtl::GetTexmapAmt(int imap) {	return maps->txmap[imap].amount;	}
float XRayMtl::GetTexmapAmt(int imap, TimeValue t) {	return maps->txmap[imap].GetAmount(t); 	}

Color XRayMtl::GetFilter(TimeValue t)   { return pb_extended->GetColor(std2_filter_color, t);	}
float XRayMtl::GetOpacity( TimeValue t) { return  pb_extended->GetFloat(std2_opacity, t); }		
float XRayMtl::GetOpacFalloff(TimeValue t){ return  pb_extended->GetFloat(std2_falloff_amnt, t);}		
float XRayMtl::GetWireSize(TimeValue t) { return  pb_extended->GetFloat(std2_wire_size, t);}
float XRayMtl::GetIOR( TimeValue t)     { return  pb_extended->GetFloat(std2_ior, t);}
float XRayMtl::GetDimIntens( TimeValue t)   { return  pb_extended->GetFloat(std2_dim_lvl, t); }
float XRayMtl::GetDimMult( TimeValue t)   { return  pb_extended->GetFloat(std2_refl_lvl, t); }
BOOL XRayMtl::MapEnabled(int i)         { return maps->txmap[i].mapOn;}


float XRayMtl::GetDynamicsProperty(TimeValue t, int mtlNum, int propID) {
	float val;
	Interval ivalid;
	switch(propID) {
case DYN_BOUNCE:
	pb_dynamics->GetValue(std2_bounce, t, val, ivalid);	
	return val;
case DYN_STATIC_FRICTION:
	pb_dynamics->GetValue(std2_static_friction, t, val, ivalid);	
	return val;
case DYN_SLIDING_FRICTION:
	pb_dynamics->GetValue(std2_sliding_friction, t, val, ivalid);	
	return val;
default: 
	assert(0);
	return 0.0f;
	}
}

void XRayMtl::SetDynamicsProperty(TimeValue t, int mtlNum, int propID, float value){
#ifndef DESIGN_VER
	switch(propID) {
case DYN_BOUNCE: 
	pb_dynamics->SetValue( std2_bounce, t, value);
	break;
case DYN_STATIC_FRICTION:
	pb_dynamics->SetValue( std2_static_friction, t, value);
	break;
case DYN_SLIDING_FRICTION:
	pb_dynamics->SetValue( std2_sliding_friction, t, value);
	break;
default:
	assert(0);
	break;
	}
#endif
}

// returns the index of the shader in the list
int XRayMtl::FindShader( Class_ID& findId, ClassDesc** ppCD )
{
	for (int i = 0; i < XRayMtl::NumShaders(); i++) {
		ClassDesc* pCD = XRayMtl::GetShaderCD(i);
		if ( findId == pCD->ClassID() ){
			if (ppCD) *ppCD = pCD;
			return i;
		}
	}
	if (ppCD) *ppCD = NULL; // not found
	return -1;
}

int XRayMtl::FindSampler( Class_ID findId, ClassDesc** ppCD )
{
	for (int i = 0; i < XRayMtl::NumSamplers(); i++) {
		ClassDesc* pCD = XRayMtl::GetSamplerCD(i);
		if ( findId == pCD->ClassID() ){
			if (ppCD) *ppCD = pCD;
			return i;
		}
	}
	if (ppCD) *ppCD = NULL; // not found
	return -1;
}

/***
ClassDesc* XRayMtl::SamplerIndxToCD( int indx )
{
IParamMap2* map = pb_sampling ? pb_sampling->GetMap() : NULL;
if ( indx < 0 ) indx = 0;
if ( map ) {
HWND hwnd = map->GetHWnd();
HWND hwndSampler = GetDlgItem(hwnd, IDC_PIX_SAMPLER);
ClassDesc* pCD = (ClassDesc*)SendMessage( hwndSampler, CB_GETITEMDATA, indx, 0 );
return pCD;
}
// last resort
SubClassList * scList = GetCOREInterface()->GetDllDir().ClassDir().GetClassList(SAMPLER_CLASS_ID);
return (*scList)[ 1 ].CD();
}
***/



/////////////////////////////////////////////////////////////////////////
//	Filing
//
#define MTL_HDR_CHUNK 0x4000
#define NEWSTDMTL_FLAGS_CHUNK 0x5000
#define NEWSTDMTL_SHADING_CHUNK 0x5004
#define NEWSTDMTL_TEX_ONOFF_CHUNK 0x5002
#define NEWSTDMTL_TEX_AMT0 0x5100
#define NEWSTDMTL_TEX_AMT1 0x5101
#define NEWSTDMTL_TEX_AMT2 0x5102
#define NEWSTDMTL_TEX_AMT3 0x5103
#define NEWSTDMTL_TEX_AMT4 0x5104
#define NEWSTDMTL_TEX_AMT5 0x5105
#define NEWSTDMTL_TEX_AMT6 0x5106
#define NEWSTDMTL_TEX_AMT7 0x5107
#define NEWSTDMTL_TEX_AMT8 0x5108
#define NEWSTDMTL_TEX_AMT9 0x5109
#define NEWSTDMTL_TEX_AMTA 0x510A

//xray part
#define NEWSTDMTL_XRAY_ESHADER_CHUNK 0x5200
#define NEWSTDMTL_XRAY_CSHADER_CHUNK 0x5201
#define NEWSTDMTL_XRAY_GAMEMTL_CHUNK 0x5202

//#define NEWSTDMTL_BUMP1_CHUNK 0x5200
#define NEWSTDMTL_VERS_CHUNK 0x5300
#define NEWSTDMTL_DIM_REFLECT 0x5400

#define NEWSTDMTL_SELFILLUM_CLR_ON 0x5500
#define NEWSTDMTL_SAMPLING_ON		0x5510
#define NEWSTDMTL_SAMPLING_QUALITY	0x5511

// IO
IOResult XRayMtl::Save(ISave *isave) 
{ 
	ULONG nb;
	IOResult res;
	isave->BeginChunk(MTL_HDR_CHUNK);
	res = MtlBase::Save(isave);
	if (res!=IO_OK) return res;
	isave->EndChunk();

	isave->BeginChunk(NEWSTDMTL_VERS_CHUNK);
	int version = CURRENT_NEWSTDMTL_VERSION;
	isave->Write(&version, sizeof(version), &nb);			
	isave->EndChunk();

	// xray params
	isave->BeginChunk(NEWSTDMTL_XRAY_ESHADER_CHUNK);
	isave->WriteCString(GetEShader(eshaderId));
	isave->EndChunk();

	isave->BeginChunk(NEWSTDMTL_XRAY_CSHADER_CHUNK);
	isave->WriteCString(GetCShader(cshaderId));
	isave->EndChunk();

	isave->BeginChunk(NEWSTDMTL_XRAY_GAMEMTL_CHUNK);
	isave->WriteCString(GetGameMtl(gamemtlId));
	isave->EndChunk();

	return IO_OK;
}		


static INode* FindNodeRef(ReferenceTarget *rt) {
	DependentIterator di(rt);
	ReferenceMaker *rm;
	INode *nd = NULL;
	while (rm=di.Next()) {	
		if (rm->SuperClassID()==BASENODE_CLASS_ID) return (INode *)rm;
		nd = FindNodeRef((ReferenceTarget *)rm);
		if (nd) return nd;
	}
	return NULL;
}



static float  GetNodeScaling(Mtl *m) {
	INode *n = FindNodeRef(m);
	if (n==NULL) 
		return 1.0f;
	Interval v;
	Matrix3 tm = n->GetNodeTM(0, &v);
	float f = float(  _abs(Length(tm.GetRow(0))) +_abs(Length(tm.GetRow(1))) + _abs(Length(tm.GetRow(2))) )/3.0f;
	return f;
}


//-----------------------------------------------------------------------
// DS - 4/7/97: Changed Opacity, Self-illumination, SHininess, Shininess strengh
// so that the map amount blends between the corresponding slider 
// setting and the map value.  This code fixes up old files so they
// will render the same way. This does not correctly handle animated values
// for the amount or parameter sliders.
//
// KE - 8/4/98 modified for converting old stdmtls to stdmtl2's
// JBW - 11/19/98 updated to convert old stdmtls and v.10 stdmtl2's to ParamBlock2-based XRayMtl's
//-----------------------------------------------------------------------

void XRayMtl::OldVerFix(int loadVer) 
{
	ULONG stdParams;

	macroRecorder->Disable();

	// ask the ClassDesc to make the P_AUTO_CONSTRUCT paramblock2s
	stdmtl2CD.MakeAutoParamBlocks(this); 

	if (loadVer < 10) {
		// create a shader, note: always a stdshader as it's an old version
		if ( shaderId < 0 || shaderId > 3 ) shaderId = 1;	// v1 files
		long cid;
		switch( shaderId ) {
case 0: cid = PHONGClassID; 
	pb_shader->SetValue(std2_faceted,0,TRUE);
	break;
case 1: cid = PHONGClassID; break;
case 2: cid = METALClassID; break;
case 3: cid = BLINNClassID; break;
		}
		long indx =  FindShader( Class_ID(cid, 0) );

		if ( indx < 0 ) return;

		// this sets the new shader via pb2 accessor
		SetShaderIndx( indx );

		pShader->SetSelfIllumClrOn( FALSE );
		stdParams = pShader->SupportStdParams();

		// convert param blocks
		IParamBlock* oldPB = old_pblock;
		IParamBlock* newParamBlk = UpdateParameterBlock(stdmtlPB, NPARAMS_O, oldPB, 
			stdmtl2PB, NPARAMS, NEWSTDMTL_PBVERSION);

		// if shader is PB2-based, give it the PB2 remapping descriptor
		if (pShader->NumParamBlocks() > 0)
			pShader->ConvertParamBlk(stdmtlPB2, NPARAMS_O, oldPB);
		else
			pShader->ConvertParamBlk(stdmtlPB, NPARAMS_O, oldPB);

		ReplaceReference( OLD_PBLOCK_REF, newParamBlk );	
		UpdateTexmaps();
		std2_extended_blk.SetSubTexNo(std2_ep_filter_map, stdIDToChannel[ID_FI]);

		SetSamplerIndx( FindSampler( Class_ID(DEFAULT_SAMPLER_CLASS_ID,0) ) ); 
		if (flags & NEWSTDMTL_SSAMP_ON) {
			pb_sampling->SetValue( std2_ssampler_enable,0, TRUE );
		}

		// gloss & strength are swapped from old versions
		TexmapSlot tmSlot = maps->txmap[ID_SH];
		maps->txmap[ID_SH] = maps->txmap[ID_SS];
		maps->txmap[ID_SS] = tmSlot;


	}

	// now set values that were previously stored outside the PB, prior to update
	pb_shader->SetValue(std2_wire, 0, GetFlag(NEWSTDMTL_WIRE)!=0);
	pb_shader->SetValue(std2_two_sided, 0, GetFlag(NEWSTDMTL_2SIDE)!=0);
	pb_shader->SetValue(std2_face_map, 0, GetFlag(NEWSTDMTL_FACEMAP)!=0);
	pb_shader->SetValue(std2_faceted, 0, GetFlag(NEWSTDMTL_FACETED)!=0);

	pb_extended->SetValue(std2_opacity_type, 0, (GetFlag(NEWSTDMTL_ADD_TRANSP) ? 2 : GetFlag(NEWSTDMTL_FILT_TRANSP) ? 0 : 1));
	pb_extended->SetValue(std2_falloff_type, 0, GetFlag(NEWSTDMTL_FALLOFF_OUT) ? 1 : 0);
	pb_extended->SetValue(std2_wire_units, 0, GetFlag(NEWSTDMTL_WIRE_UNITS) ? 1 : 0);
	pb_extended->SetValue(std2_apply_refl_dimming, 0, dimReflect);

	pShader->SetLockAD( GetFlag(NEWSTDMTL_LOCK_AD)?TRUE:FALSE );
	pShader->SetLockDS( GetFlag(NEWSTDMTL_LOCK_DS)?TRUE:FALSE );
	pShader->SetLockADTex( GetFlag(NEWSTDMTL_LOCK_ADTEX)!=0 );
	pb_maps->SetValue(std2_mp_ad_texlock, 0, GetFlag(NEWSTDMTL_LOCK_ADTEX)!=0 );

	if (loadVer < 8) {
		Interval v;
		Update(0, v);
		// In old versions, the ID's & the channel numbers correspond
		if (MAPACTIVE(ID_OP)) {
			if (maps->txmap[ID_OP].amount != 1.0f) 
				SetOpacity(0.0f, 0);
		}

		if ( stdParams & STD_PARAM_SELFILLUM )
			if (MAPACTIVE(ID_SI)) {
				if (maps->txmap[ID_SI].amount != 1.0f) 
					pShader->SetSelfIllum(0.0f, 0);
			}

			if ( stdParams & STD_PARAM_SPECULAR_LEV )
				if (MAPACTIVE(ID_SS)) {
					float amt = maps->txmap[ID_SS].amount;
					SetTexmapAmt(ID_SS, amt * pShader->GetSpecularLevel(0, 0), 0);
					pShader->SetSpecularLevel(0.0f, 0);
				}

				if ( stdParams & STD_PARAM_GLOSSINESS )
					if (MAPACTIVE(ID_SH)) {
						float amt = maps->txmap[ID_SH].amount;
						SetTexmapAmt(ID_SH, amt* pShader->GetGlossiness(0, 0), 0);
						pShader->SetGlossiness(0.0f, 0);
					}

	}
	if (loadVer < 9) {
		if ( stdParams & STD_PARAM_SOFTEN_LEV )
			if (flags&NEWSTDMTL_SOFTEN) 
				pShader->SetSoftenLevel(.6f, 0);
			else 
				pShader->SetSoftenLevel(0.0f, 0);
	}

	// at this point we have a v10 Stdmtl2, convert to PB2-based
	// distribute old pblock values to new pb2 blocks 
	UpdateParameterBlock2(extVer10, NPARAMS, old_pblock, &std2_extended_blk, pb_extended);
#ifndef DESIGN_VER
	UpdateParameterBlock2(dynVer10, NPARAMS, old_pblock, &std2_dynamics_blk, pb_dynamics);
#endif

	// mask off  rollup flags
	flags &= ~NEWSTDMTL_ROLLUP_FLAGS;
	flags |= NEWSTDMTL_ROLLUP1_OPEN;
	//	pb_shader->SetRolloutOpen( flags & NEWSTDMTL_ROLLUP1_OPEN );

	macroRecorder->Enable();

	// delete old pblock
	ReplaceReference(OLD_PBLOCK_REF, NULL);

}


// versin update post-load call back
class NewStdMtl2UpdateCB: public PostLoadCallback {
public:
	// create shader & new pblock
	XRayMtl *m;
	int loadVersion;

	NewStdMtl2UpdateCB( XRayMtl *s, int loadVers ){ m = s; loadVersion = loadVers; }
	void proc(ILoad *iload) {
		m->OldVerFix( loadVersion );
		xr_delete((NewStdMtl2UpdateCB*)this); 
	} 
};

void XRayMtl::BumpFix( ) {
	// Fix up bump map amount on old files if the node is scaled.	DS 4/16/99
	int bumpChannel = stdIDToChannel[ID_BU];
	if ((*maps)[ bumpChannel].IsActive()) {
		float s = GetNodeScaling(this);
		if (s!=1.0f) {
			float am = GetTexmapAmt(bumpChannel);
			SetTexmapAmt(bumpChannel, s*am, 0);
		}
	}
}


// Bumpp map amount fix post-load call back
class NewStdMtl2BumpFixCB: public PostLoadCallback {
public:
	// create shader & new pblock
	XRayMtl *m;

	NewStdMtl2BumpFixCB( XRayMtl *s ){ m = s; }
	int Priority() { return 7; } // run at lower priority after normal PLCB's are all finished
	void proc(ILoad *iload) {
		m->BumpFix( );
		xr_delete((NewStdMtl2BumpFixCB*)this); 
	} 
};

// post-load finalize callback
class NewStdMtl2CB: public PostLoadCallback {
public:
	XRayMtl *m;

	NewStdMtl2CB(XRayMtl *s){ m = s; }
	void proc(ILoad *iload) 
	{
		// ensure all map-related parameters are set up
		m->UpdateTexmaps();

		// make sure the right indx is selected for shaders & samplers
		m->SetShaderIndx( m->FindShader( m->pShader->ClassID() ), FALSE );
		if( m->pixelSampler == NULL ) // for some alpha files!
			m->SetSamplerIndx( m->FindSampler( Class_ID(DEFAULT_SAMPLER_CLASS_ID,0) ) ); 

		m->SetSamplerIndx( m->FindSampler( m->pixelSampler->ClassID() ), FALSE ); 
		m->SetFlag(NEWSTDMTL_SSAMP_ON, m->pixelSampler->GetEnable() );

#ifndef	DESIGN_VER
		if( m->pb_dynamics == NULL )
			//create the parameter block
			CreateParameterBlock2(&std2_dynamics_blk, m);
#endif
		if( m->pb_xray == NULL )
			//create the parameter block
			CreateParameterBlock2(&std2_xray_blk, m);

		m->SetEShaderIndx( m->eshaderId );
		m->SetCShaderIndx( m->cshaderId );
		m->SetGameMtlIndx( m->gamemtlId );


		// DS - 2/11/99 This Update should not be here: it causes all
		// submaps to be loaded when loading, which makes loading matlibs very slow.
		//		Interval i;
		//		i.SetInfinite();
		//		m->Update(0, i);

		xr_delete((NewStdMtl2CB*)this); 
	} 
};

IOResult XRayMtl::Load(ILoad *iload) { 
	ULONG nb;
	int id;
	int version = 0;
	dimReflect = FALSE;

	// fix old ver
	gamemtlId = -1;

	IOResult res;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(id = iload->CurChunkID())  {
case MTL_HDR_CHUNK:
	res = MtlBase::Load(iload);
	ivalid.SetEmpty();
	break;
case NEWSTDMTL_FLAGS_CHUNK:
	res = iload->Read(&flags, sizeof(flags), &nb);
	break;
case NEWSTDMTL_SHADING_CHUNK:
	res = iload->Read(&shaderId, sizeof(shaderId), &nb);
	break;
case NEWSTDMTL_XRAY_ESHADER_CHUNK:{
	LPSTR es_name;
	res = iload->ReadCStringChunk(&es_name);
	if (res==IO_OK) eshaderId = XRayMtl::FindEShader(es_name);
	}break;
case NEWSTDMTL_XRAY_CSHADER_CHUNK:{
	LPSTR cs_name;
	res = iload->ReadCStringChunk(&cs_name);
	if (res==IO_OK) cshaderId = XRayMtl::FindCShader(cs_name);
	}break;
case NEWSTDMTL_XRAY_GAMEMTL_CHUNK:{
	LPSTR gm_name;
	res = iload->ReadCStringChunk(&gm_name);
	if (res==IO_OK) gamemtlId = XRayMtl::FindGameMtl(gm_name);
	}break;
case NEWSTDMTL_VERS_CHUNK:
	res = iload->Read(&version, sizeof(version), &nb);
	break;
case NEWSTDMTL_TEX_ONOFF_CHUNK:
	{
		ULONG f;
		res = iload->Read(&f, sizeof(f), &nb);
		for (int i=0; i<STD2_NMAX_TEXMAPS; i++) 
			maps->txmap[i].mapOn = (f&(1<<i))?1:0;
	}
	break;
case NEWSTDMTL_DIM_REFLECT:
	dimReflect = TRUE;
	break;
		}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
	}

	// register version updaters
	if (version <= FINAL_PARAMBLOCK_v1_VERSION) {
		iload->RegisterPostLoadCallback(xr_new<ParamBlockPLCB>((ParamVersionDesc*)oldNewStdMtlVersions, NEWSTDMTL_NUMOLDVER, &stdMtlVersion, this, 0));
		iload->RegisterPostLoadCallback(xr_new<NewStdMtl2UpdateCB>(this, version));
		if (version<12)
			iload->RegisterPostLoadCallback(xr_new<NewStdMtl2BumpFixCB>(this));
		iload->SetObsolete();
	}

	if (TextureDisplayEnabled()) {
		IncrActive();
	}

	if (gamemtlId==-1) gamemtlId = XRayMtl::FindGameMtl("default");

	// register plcb to finalize setup
	iload->RegisterPostLoadCallback(xr_new<NewStdMtl2CB>(this));

	return IO_OK;
}


#define TRANSP_SUB		0
#define TRANSP_ADD		1
#define TRANSP_FILTER	2


// Composite  c_over on top of c. Assume c_over has pre-multiplied alpha.
IC void AlphaCompCol(Color& c,  RGBA c_over) {
	float ia = 1.0f - _abs(c_over.a);
	ia = LBound( ia );
	c.r = c.r * ia + c_over.r;
	c.g = c.g * ia + c_over.g;
	c.b = c.b * ia + c_over.b;
} 

//////////////////////////////////////////////////////////////////////////////
//
//	transpColor utility, same as in shaders/shaderUtil
//
Color XRayMtl::TranspColor( float opac, Color filt, Color diff )
{
	// Compute the color of the transparent filter color
	if (flags & NEWSTDMTL_ADD_TRANSP) {
		float f = 1.0f - opac;
		return Color(f, f, f);   

	} else if ( flags & NEWSTDMTL_FILT_TRANSP ){
		// Transparent Filter color mapping
		if (opac>0.5f) {
			// darken as opac goes ( 0.5--> 1.0)
			// so that max component reaches 0.0f when opac reaches 1.0
			// find max component of filt
			float m = Max(filt);
			float d = 2.0f*(opac-.5f)*m;
			Color fc = filt-d;
			fc = LBound( fc );
			return fc;
		} else {
			// lighten as opac goes ( 0.5--> 0.0)
			// so that min component reaches 1.0f when opac reaches 1.0
			// find min component of filt
			float m = Min(filt);
			float d = (1.0f-2.0f*opac)*(1.0f-m);
			Color fc = filt+d;
			fc = UBound( fc );
			return fc;
		}

	} else {
		// original 3DS transparency, subtractive using diff color
		Color f = (1.0f-diff);  
		return  (1.0f-opac)*f;
	}
}


#define DOMAP(n) ((n>=0) && sc.doMaps&&(*maps)[ n ].IsActive())

#ifdef _DEBUG
static long xStop = -1;
static long yStop = -1;
#endif

// begin - ke/mjm - 03.16.00 - merge reshading code
//BOOL XRayMtl::SupportsReShading(ShadeContext& sc) { return TRUE; }		

void XRayMtl::PreShade(ShadeContext& sc, IReshadeFragment* pFrag ) 
{
	RGBA mval;
	Point3 N, N0, P;

#ifdef _DEBUG
	IPoint2 sPt = sc.ScreenCoord();
	if( sPt.x == xStop && sPt.y == yStop )
		long junk = -1;
#endif

	BOOL bumped = FALSE;
	N = sc.Normal();

	int ambChan = stdIDToChannel[ ID_AM ];
	int diffChan = stdIDToChannel[ ID_DI ];
	int trChan = stdIDToChannel[ ID_OP ];
	int filtChan = stdIDToChannel[ ID_FI ];
	ULONG trType = (flags & NEWSTDMTL_FILT_TRANSP) ? TRANSP_FILTER :
	(flags & NEWSTDMTL_ADD_TRANSP) ? TRANSP_ADD : TRANSP_SUB;


	// Do texture mapping
	sc.SetIOR(ioRefract);

	// Bump mapping: Do this FIRST so other maps can use the perturbed normal
	int bumpChannel = stdIDToChannel[ID_BU];
	if (DOMAP(bumpChannel)) {
		Point3 dn = (*maps)[bumpChannel].EvalNormalPerturb(sc);
		bumped = TRUE;
		N = Normalize( N + (sc.backFace?-dn:dn));
		// NB: shadeContext has perturbed normal
		sc.SetNormal(N);
		pFrag->AddUnitVecChannel( N );
	}

	// All normal color & scalar maps are handled here
	long t0 = pShader->ChannelType(0);
	RGBA c;
	for ( long i = 0; i < STD2_NMAX_TEXMAPS; ++i ) {
		if ( sc.doMaps && (*maps)[ i ].IsActive()) {
			long t = pShader->ChannelType(i);
			// composite the channel
			if ( (t & SKIP_CHANNELS)==0 ){
				if ( t & MONO_CHANNEL ) {
					// note: r always holds channel data for mono channels
					float m = (*maps)[i].map->EvalMono(sc);
					pFrag->AddFloatChannel( m );
				} else {
					if ( i != ambChan || (flags & NEWSTDMTL_LOCK_ADTEX)==0 ) { 
						c = (*maps)[i].map->EvalColor(sc);
						if ((*maps)[i].map->IsHighDynamicRange()) {
							LogLUV32Pixel lc;

							lc = c;
							pFrag->AddIntChannel(lc.value);
							pFrag->AddFloatChannel(c.a);
						}
						else {
							BoundInPlace( c );
							pFrag->AddColorChannel(c); 
						}
					}
				}
			}
		}
	}

	// Evaluate reflection map.
	int reflChan = stdIDToChannel[ ID_RL ];
	if (DOMAP(reflChan)) {
		AColor rcol;
		Texmap *reflmap = (*maps)[reflChan].map;
		if (reflmap->HandleOwnViewPerturb()) {
			sc.TossCache(reflmap);
			rcol = reflmap->EvalColor(sc);
		} else 
			rcol = sc.EvalEnvironMap(reflmap, sc.ReflectVector());

		if ((*maps)[reflChan].map->IsHighDynamicRange()) {
			LogLUV32Pixel lc;

			lc = rcol;
			pFrag->AddIntChannel(lc.value);
			pFrag->AddFloatChannel(rcol.a);
		}
		else {
			pFrag->AddColorChannel(rcol); 
		}
	}

	// do refraction map 
	int refrChan = stdIDToChannel[ ID_RR ];
	if (DOMAP(refrChan)) {
		// Evaluate refraction map, as yet unfiltered by filter color.
		Texmap *refrmap = (*maps)[refrChan].map;
		AColor rcol;
		if (refrmap->HandleOwnViewPerturb()) 
			rcol = refrmap->EvalColor( sc );
		else  
			rcol = sc.EvalEnvironMap( refrmap, sc.RefractVector(ioRefract) );

		if ((*maps)[refrChan].map->IsHighDynamicRange()) {
			LogLUV32Pixel lc;

			lc = rcol;
			pFrag->AddIntChannel(lc.value);
		}
		else {
			pFrag->AddColorChannel(rcol); 
		}
	}

	// see if the shader wants to save anything
	IReshading* pReshading;
	if((pReshading = (IReshading*)pShader->GetInterface(IID_IReshading)) != NULL ){
		pReshading->PreShade( sc, pFrag );
	}
	if( bumped )
		sc.SetNormal( sc.OrigNormal() );
}

void XRayMtl::PostShade(ShadeContext& sc, IReshadeFragment* pFrag, int& nextTexIndex, IllumParams*) 
{
	IllumParams ip(0, NULL);
	Color lightCol, rescol, diffIllum0;
	float opac;
	RGBA mval;
	Point3  P;
	BOOL bumped = FALSE;

#ifdef _DEBUG
	IPoint2 sPt = sc.ScreenCoord();
	if( sPt.x == xStop && sPt.y == yStop )
		long junk = -1;
#endif

	pShader->Update( sc.CurTime(), FOREVER ); // mjm - 12.17.99
	pShader->GetIllumParams( sc, ip );
	ip.ClearOutputs();
	ip.pShader = pShader; 
	ip.pMtl = this;

	opac =  opacity;

	int ambChan = stdIDToChannel[ ID_AM ];
	int diffChan = stdIDToChannel[ ID_DI ];
	int trChan = stdIDToChannel[ ID_OP ];
	int filtChan = stdIDToChannel[ ID_FI ];
	int reflChan = stdIDToChannel[ ID_RL ];
	int refrChan = stdIDToChannel[ ID_RR ];

	if ( filtChan >= 0 )ip.channels[filtChan] = filter;
	if ( trChan >= 0 ) ip.channels[trChan].r = opac;
	ULONG trType = (flags & NEWSTDMTL_FILT_TRANSP) ? TRANSP_FILTER :
	(flags & NEWSTDMTL_ADD_TRANSP) ? TRANSP_ADD : TRANSP_SUB;

	P = sc.P();		// position

	ip.stdParams = pShader->SupportStdParams();

	// Do texture mapping
	ip.hasComponents = 0;
	ip.stdIDToChannel = stdIDToChannel;
	//	sc.SetIOR(ioRefract);

	// All normal color & scalar maps are handled here
	if (pFrag->NChannels() > 0) {

		// bump mapping
		if( DOMAP( stdIDToChannel[ ID_BU ] ) ) {
			bumped = TRUE;
			// NB: shadeContext has perturbed normal
			sc.SetNormal( pFrag->GetUnitVecChannel( nextTexIndex++ ) );
		}

		RGBA c;
		for ( long i = 0; i < STD2_NMAX_TEXMAPS; ++i ) {
			if ( sc.doMaps && (*maps)[ i ].IsActive()) {
				long t = pShader->ChannelType(i);
				// composite the channel
				if ( (t & SKIP_CHANNELS)==0 ){
					if ( t & MONO_CHANNEL ) {
						// note: r always holds channel data for mono channels
						ip.channels[ i ].r = (*maps)[i].LerpMono( ip.channels[ i ].r, pFrag->GetFloatChannel(nextTexIndex++)); // mjm - 1.17.00
					} else {
						if ( i != ambChan || (flags & NEWSTDMTL_LOCK_ADTEX)==0 ) { 
							if ((*maps)[ i ].map->IsHighDynamicRange()) {
								LogLUV32Pixel lc;
								lc.value = pFrag->GetIntChannel(nextTexIndex++);
								lc.GetRGB(c);
								c.a = pFrag->GetFloatChannel(nextTexIndex++);
							}
							else {
								c = pFrag->GetColorChannel(nextTexIndex++); 
							}
							c *= (*maps)[i].amount; // mjm - 1.17.00
							AlphaCompCol( ip.channels[ i ], c); 
						}
					}
					// lock adtex, really channels 0 & 1
					if ( i == diffChan && (flags & NEWSTDMTL_LOCK_ADTEX) && ambChan >= 0  ) { 
						AlphaCompCol( ip.channels[ ambChan ], c ); 
					}
				}// end, don't skip
			} // end, is active
		}// end, for each possible map

		// Evaluate reflection map.
		if ( (*maps)[ reflChan ].map ) 
			ip.hasComponents |= HAS_REFLECT_MAP;

		if (DOMAP(reflChan) ) {
			ip.hasComponents |= HAS_REFLECT;
			AColor rcol;										 // mjm - 1.17.00
			if ((*maps)[ reflChan ].map->IsHighDynamicRange()) {
				LogLUV32Pixel lc;
				lc.value = pFrag->GetIntChannel(nextTexIndex++);
				lc.GetRGB(rcol);
				rcol.a = pFrag->GetFloatChannel(nextTexIndex++);
			}
			else {
				rcol = pFrag->GetColorChannel( nextTexIndex++ ); // mjm - 1.17.00
			}
			Color rc(rcol.r, rcol.g, rcol.b);
			ip.channels[reflChan] = rc;
			ip.reflectAmt = rcol.a * (*maps)[reflChan].amount;
		}

		// do refraction map 
		if ( (*maps)[ refrChan ].map ) 
			ip.hasComponents |= HAS_REFRACT_MAP;

		if (DOMAP(refrChan) ) {
			// Evaluate refraction map, as yet unfiltered by filter color.
			ip.refractAmt = (*maps)[refrChan].amount;
			AColor rcol;										 // mjm - 1.17.00
			if ((*maps)[ refrChan ].map->IsHighDynamicRange()) {
				LogLUV32Pixel lc;
				lc.value = pFrag->GetIntChannel(nextTexIndex++);
				lc.GetRGB(rcol);
			}
			else {
				rcol = pFrag->GetColorChannel( nextTexIndex++ ); // mjm - 1.17.00
			}
			Color rc(  rcol.r,  rcol.g,  rcol.b );
			ip.channels[refrChan] = rc;
			ip.hasComponents |= HAS_REFRACT;
		}
	} // end, at least one map

	// get falloff opacity for possible use by shader or compositor
	opac = ip.channels[trChan].r; // get (perhaps) textured opacity

	ip.finalOpac = GetEffOpacity( sc, opac );
	if( ip.finalOpac < 1.0f )
		ip.hasComponents |= HAS_OPACITY;

	// init the global ambient
	ip.ambIllumOut = sc.ambientLight;

	// mapping done, illuminate...
	IReshading* pReshading;
	if((pReshading = (IReshading*)pShader->GetInterface(IID_IReshading)) != NULL ){
		pReshading->PostShade( sc, pFrag, nextTexIndex, &ip );
	}

	// return color in sc, not the frag itself
	sc.out.c = ip.finalC;
	sc.out.t = ip.finalT;

	if( bumped )
		sc.SetNormal( sc.OrigNormal() );
}

// too late for sdk, eco in progress
IllumParams* CloneIp( IllumParams& ip )
{
	IllumParams* pClone = xr_new<IllumParams>( ip.nUserIllumOut, ip.userIllumNames );
	pClone->finalC = ip.finalC;
	pClone->finalT = ip.finalT;
	pClone->finalOpac = ip.finalOpac;
	pClone->finalAttenuation = ip.finalAttenuation;
	pClone->ambIllumOut = ip.ambIllumOut;
	pClone->diffIllumOut = ip.diffIllumOut;
	pClone->transIllumOut = ip.transIllumOut;
	pClone->selfIllumOut = ip.selfIllumOut;
	pClone->specIllumOut = ip.specIllumOut;
	pClone->reflIllumOut = ip.reflIllumOut;
	pClone->transIllumOut = ip.transIllumOut;
	pClone->diffIllumIntens = ip.diffIllumIntens;
	for( int i=0; i < ip.nUserIllumOut; ++i )
		pClone->userIllumOut[i] = ip.userIllumOut[i];

	pClone->stdParams = ip.stdParams;
	pClone->hasComponents = ip.hasComponents;
	pClone->pShader = ip.pShader;
	pClone->pMtl = ip.pMtl;
	pClone->stdIDToChannel = ip.stdIDToChannel;
	pClone->refractAmt = ip.refractAmt;
	pClone->reflectAmt = ip.reflectAmt;

	for( i=0; i < STD2_NMAX_TEXMAPS; ++i )
		pClone->channels[ i ] = ip.channels[ i ];

	return pClone;
}


// if this function changes, please also check SupportsReShading, PreShade, PostShade
// end - ke/mjm - 03.16.00 - merge reshading code
// [attilas|24.5.2000] if this function changes, please also check EvalColorStdChannel
void XRayMtl::Shade(ShadeContext& sc) 
{
	IllumParams ip( pShader->nUserIllumOut(), pShader->UserIllumNameArray() );
	Color lightCol, rescol, diffIllum0;
	float opac;
	RGBA mval;
	Point3 N0, P;

#ifdef _DEBUG
	IPoint2 sPt = sc.ScreenCoord();
	if( sPt.x == xStop && sPt.y == yStop )
		long junk = -1;
#endif

	BOOL bumped = FALSE;
	if (gbufID) sc.SetGBufferID(gbufID);
	//	ip.mtlFlags = flags;

	pShader->GetIllumParams( sc, ip );
	ip.ClearOutputs();
	ip.pShader = pShader; 
	ip.pMtl = this;

	//	N0 = ip.N = sc.Normal();
	N0 = sc.Normal();
	opac =  opacity;

	int ambChan = stdIDToChannel[ ID_AM ];
	int diffChan = stdIDToChannel[ ID_DI ];
	int trChan = stdIDToChannel[ ID_OP ];
	int filtChan = stdIDToChannel[ ID_FI ];
	if ( filtChan >= 0 )ip.channels[filtChan] = filter;
	if ( trChan >= 0 ) ip.channels[trChan].r = opac;
	ULONG trType = (flags & NEWSTDMTL_FILT_TRANSP) ? TRANSP_FILTER :
	(flags & NEWSTDMTL_ADD_TRANSP) ? TRANSP_ADD : TRANSP_SUB;

	if (sc.mode==SCMODE_SHADOW) {
		// Opacity mapping;
		if (DOMAP(trChan))
			opac = (*maps)[trChan].LerpEvalMono(sc, opac);

		// "Shadow mode": This just computes the transparency, which is all 
		// you need for shadowing.
		if (opac!=1.0f || opfall!=0.0f) {

			if (opfall != 0.0f) {	
				opac = GetEffOpacity(sc, opac);
			}

			// Transparency may use diffuse color mapping, check for orig 3ds transp
			if ((flags & (NEWSTDMTL_ADD_TRANSP|NEWSTDMTL_FILT_TRANSP))==0) {
				if (DOMAP(diffChan)) {
					mval = (*maps)[diffChan].Eval(sc);
					AlphaCompCol(ip.channels[diffChan], mval); 
				}
			}
			// Transparency may also use filter color mapping
			if ( flags & NEWSTDMTL_FILT_TRANSP ){
				if (DOMAP(filtChan)) {
					mval = (*maps)[filtChan].Eval(sc);
					AlphaCompCol(ip.channels[filtChan], mval); 
				}
			}
			// Compute the transpareny color
			sc.out.t = TranspColor( opac, ip.channels[filtChan], ip.channels[diffChan]);
		} else 
			sc.out.t.Black();
		return;
	} // end, shadow mode

	// any shader can be faceted...& bumpmapped as well
	if ( flags & NEWSTDMTL_FACETED ){
		Point3 N = sc.GNormal();
		bumped = TRUE;
		sc.SetNormal(N);
	}

	P = sc.P();		// position

	ip.stdParams = pShader->SupportStdParams();

	// Do texture mapping
	ip.hasComponents = 0;
	ip.stdIDToChannel = stdIDToChannel;
	sc.SetIOR(ioRefract);

	// Bump mapping: Do this FIRST so other maps can use the perturbed normal
	int bumpChannel = stdIDToChannel[ID_BU];
	if (DOMAP(bumpChannel)) {
		ip.hasComponents |= HAS_BUMPS; 
		Point3 dn = (*maps)[bumpChannel].EvalNormalPerturb(sc);
		Point3 N = Normalize( sc.Normal() + (sc.backFace?-dn:dn) );
		sc.SetNormal(N);
		bumped = TRUE;
	}

	// All normal color & scalar maps are handled here
	long t0 = pShader->ChannelType(0);
	RGBA c;
	for ( long i = 0; i < STD2_NMAX_TEXMAPS; ++i ) {
		if ( sc.doMaps && (*maps)[ i ].IsActive()) {
			long t = pShader->ChannelType(i);
			// composite the channel
			if ( (t & SKIP_CHANNELS)==0 ){
				if ( t & MONO_CHANNEL ) {
					// note: x always holds channel data for mono channels
					ip.channels[ i ].r = (*maps)[i].LerpEvalMono(sc, ip.channels[ i ].r );
				} else {
					if ( i != ambChan || (flags & NEWSTDMTL_LOCK_ADTEX)==0 ) { 
						c = (*maps)[i].Eval(sc);
						AlphaCompCol( ip.channels[ i ], c); 
					}
				}
				// lock adtex, really channels 0 & 1
				if ( i == diffChan && ambChan >= 0 && (flags & NEWSTDMTL_LOCK_ADTEX) ) { 
					if ( t0 & MONO_CHANNEL ) {
						ip.channels[ ambChan ].r = (*maps)[diffChan].LerpEvalMono(sc, ip.channels[diffChan].r );
					} else {
						AlphaCompCol( ip.channels[ ambChan ], c ); 
					}

				}
			}
		}
	}

	// Evaluate reflection map.
	int reflChan = stdIDToChannel[ ID_RL ];
	if ( (*maps)[ reflChan ].map ) 
		ip.hasComponents |= HAS_REFLECT_MAP;

	if (DOMAP(reflChan)) {
		ip.hasComponents |= HAS_REFLECT;
		AColor rcol;
		Texmap *reflmap = (*maps)[reflChan].map;
		if (reflmap->HandleOwnViewPerturb()) {
			sc.TossCache(reflmap);
			rcol = reflmap->EvalColor(sc);
		} else 
			rcol = sc.EvalEnvironMap(reflmap, sc.ReflectVector());
		Color rc(rcol.r, rcol.g, rcol.b);
		ip.channels[reflChan] = rc;
		ip.reflectAmt = rcol.a * (*maps)[reflChan].amount;
	}

	// do refraction map 
	int refrChan = stdIDToChannel[ ID_RR ];
	if ( (*maps)[ reflChan ].map ) 
		ip.hasComponents |= HAS_REFRACT_MAP;

	if (DOMAP(refrChan)) {
		// Evaluate refraction map, as yet unfiltered by filter color.
		ip.refractAmt = (*maps)[refrChan].amount;
		Texmap *refrmap = (*maps)[refrChan].map;
		AColor rcol;
		if (refrmap->HandleOwnViewPerturb()) 
			rcol = refrmap->EvalColor( sc );
		else  
			rcol = sc.EvalEnvironMap( refrmap, sc.RefractVector(ioRefract) );

		Color rc(  rcol.r,  rcol.g,  rcol.b );
		ip.channels[refrChan] = rc;
		ip.hasComponents |= HAS_REFRACT;
	}

	// get falloff opacity for possible use by shader or compositor
	opac = ip.channels[trChan].r; // get (perhaps) textured opacity

	ip.finalOpac = GetEffOpacity(sc, opac);
	if(ip.finalOpac < 1.0f)
		ip.hasComponents |= HAS_OPACITY;

	// init the global ambient
	ip.ambIllumOut = sc.ambientLight;

	// mapping done, illuminate...
	pShader->Illum(sc, ip);

	// return color in sc, not the frag itself
	sc.out.c = ip.finalC;
	sc.out.t = ip.finalT;

	// Render Elements
	// note: shadow must be on for any shadows to work. no ele's will get shadows
	// with shadows off
	int nEles = sc.NRenderElements();
	if( nEles != 0 ){
		IllumParams* pIp2 = CloneIp( ip ); // too late for sdk, eco in progress
		if( sc.shadow ){
			// get unshadowed ip
			sc.shadow = FALSE;
			pIp2->ClearOutputs();
			pShader->Illum(sc, *pIp2);
			sc.shadow = TRUE;
		}
		for( int i=0; i < nEles; ++i ){
			IRenderElement* pEle = sc.GetRenderElement(i);
			if( pEle->IsEnabled() ){
				MaxRenderElement* pMaxEle = (MaxRenderElement*)pEle->GetInterface( MaxRenderElement::IID );
				if( pEle->ShadowsApplied() )
					pMaxEle->PostIllum( sc, ip );
				else
					pMaxEle->PostIllum( sc, *pIp2 );
			}
		}
		xr_delete(pIp2);
	}

	if (bumped) sc.SetNormal(N0); // restore normal
}


float XRayMtl::EvalDisplacement(ShadeContext& sc) {
	int dispChan = stdIDToChannel[ ID_DP ];
	if (DOMAP(dispChan))
		return (*maps)[dispChan].EvalMono(sc);
	else return 0.0f; 
}


Interval XRayMtl::DisplacementValidity(TimeValue t) {
	int dispChan = stdIDToChannel[ ID_DP ];
	if (MAPACTIVE(dispChan)) { 
		Interval iv;
		iv.SetInfinite();
		maps->txmap[dispChan].Update(t, iv);
		return iv;
	}
	else 
		return FOREVER;
}

void XRayMtl::DiscardTexHandles() {
	for (int i=0; i<2; i++) {
		if (texHandle[i]) {
			texHandle[i]->DeleteThis();
			texHandle[i] = NULL;
		}
	}
	texHandleValid.SetEmpty();
}

// begin - dds- 04.27.00 - multiple map display support
void XRayMtl::ActivateTexDisplay(BOOL onoff) {
	if (!onoff) {
		DiscardTexHandles();
	}
}

struct TextOps {
	UBYTE colorOp;
	UBYTE colorAlphaSource;
	UBYTE colorScale;
	UBYTE alphaOp;
	UBYTE alphaAlphaSource;
	UBYTE alphaScale;
};

// TBD: these values have to be corrected.
#define TXOP_MODULATE 0
#define TXOP_ALPHABLEND 1
#define TXOP_OPACITY 2
static TextOps txops[] = {
	{ GW_TEX_MODULATE, GW_TEX_TEXTURE, GW_TEX_SCALE_1X, GW_TEX_LEAVE, GW_TEX_TEXTURE, GW_TEX_SCALE_1X }, 
	{ GW_TEX_ALPHA_BLEND, GW_TEX_TEXTURE, GW_TEX_SCALE_1X, GW_TEX_LEAVE,  GW_TEX_TEXTURE, GW_TEX_SCALE_1X }, 
	{ GW_TEX_LEAVE, GW_TEX_TEXTURE, GW_TEX_SCALE_1X, GW_TEX_REPLACE, GW_TEX_TEXTURE, GW_TEX_SCALE_1X }, 
};

void XRayMtl::SetTexOps(Material *mtl, int i, int type) {
	TextureInfo *ti = &mtl->texture[i];
	ti->colorOp = txops[type].colorOp;
	ti->colorAlphaSource = txops[type].colorAlphaSource;
	ti->colorScale = txops[type].colorScale;
	ti->alphaOp = txops[type].alphaOp;
	ti->alphaAlphaSource = txops[type].alphaAlphaSource;
	ti->alphaScale = txops[type].alphaScale;
	texOpsType[i] = type;
}

#define BMIDATA(x) ((UBYTE *)((BYTE *)(x) + sizeof(BITMAPINFOHEADER)))

static void StuffAlpha(BITMAPINFO *bi, float amt, float opac, Color fill) {
	int w = bi->bmiHeader.biWidth;
	int h = bi->bmiHeader.biHeight;
	int npix = w*h;
	UBYTE *a = BMIDATA(bi);
	Color24 col;
	col.r = (int)(fill.r*255.0f);
	col.g = (int)(fill.g*255.0f);
	col.b = (int)(fill.b*255.0f);
	if (amt<1.0f) {
		int ia = int(amt*255.0f);
		int	ic = int( (1.0f-amt)*opac*255.0f);
		for (int j=0; j<npix; j++) {
			a[3] = ((( (a[0]+a[1]+a[2])/3 )*ia )>>8) + ic;
			a[0] = col.b;
			a[1] = col.g;
			a[2] = col.r;
			a += 4;
		}
	}
	else {
		for (int j=0; j<npix; j++) {
			a[3] = (a[0]+a[1]+a[2])/3;
			a[0] = col.b;
			a[1] = col.g;
			a[2] = col.r;
			a += 4;
		}
	}
}


static void StuffAlphaInto(BITMAPINFO *bia, BITMAPINFO *bi, float amt, float opac) {
	int w = bi->bmiHeader.biWidth;
	int h = bi->bmiHeader.biHeight;
	int npix = w*h;
	UBYTE *a = BMIDATA(bia);
	UBYTE *b = BMIDATA(bi);
	if (amt<1.0f) {
		int ia = int(amt*255.0f);
		int	ic = int( (1.0f-amt)*opac*255.0f);
		for (int j=0; j<npix; j++) {
			b[3] = ((( (a[0]+a[1]+a[2])/3 )*ia )>>8) + ic;
			a += 4;
			b += 4;
		}
	}
	else {
		for (int j=0; j<npix; j++) {
			b[3] = (a[0]+a[1]+a[2])/3;
			a += 4;
			b += 4;
		}
	}
}

#if 0
static BOOL SameUV(TextureInfo &ta, TextureInfo &tb) {
	if (ta.uvwSource!=tb.uvwSource) return FALSE;
	if (ta.mapChannel!=tb.mapChannel) return FALSE;
	if (ta.tiling[0]!=tb.tiling[0]) return FALSE;
	if (ta.tiling[1]!=tb.tiling[1]) return FALSE;
	if (ta.tiling[2]!=tb.tiling[2]) return FALSE;
	if (ta.faceMap!=tb.faceMap) return FALSE;
	if (!(ta.textTM==tb.textTM)) return FALSE;
	return TRUE;
}
#endif

static Color whiteCol(1.0f, 1.0f, 1.0f);

void XRayMtl::SetupGfxMultiMaps(TimeValue t, Material *mtl, MtlMakerCallback &cb) {
	if (texHandleValid.InInterval(t)) {
		mtl->texture.SetCount(numTexHandlesUsed);
		for (int i=0; i<numTexHandlesUsed; i++) {
			if (texHandle[i]) {
				mtl->texture[i].textHandle = texHandle[i]->GetHandle();
				Texmap *tx = (*maps)[useSubForTex[i]].map;
				cb.GetGfxTexInfoFromTexmap(t, mtl->texture[i], tx ); 		
				SetTexOps(mtl,i,texOpsType[i]);
			}
		}
		return;
	}

	Texmap *tx[2];
	int diffChan = stdIDToChannel[ ID_DI ];
	int opacChan = stdIDToChannel[ ID_OP ];
	tx[0] = (*maps)[diffChan].IsActive()?(*maps)[diffChan].map:NULL;
	tx[1] = (*maps)[opacChan].IsActive()?(*maps)[opacChan].map:NULL;

	int nsupport = cb.NumberTexturesSupported();

	BITMAPINFO *bmi[NTEXHANDLES];

	int nmaps=0;
	for (int i=0; i<NTEXHANDLES; i++) {
		if (tx[i]) nmaps ++;
		bmi[i] = NULL;
	}
	mtl->texture.SetCount(nmaps);
	if (nmaps==0) 
		return;
	for (i=0; i<nmaps; i++)
		mtl->texture[i].textHandle = NULL;
	texHandleValid.SetInfinite();
	Interval  valid;
	BOOL needDecal = FALSE;
	int ntx = 0;
	int op; 

	int forceW = 0;
	int forceH = 0;
	if (tx[0]) {
		cb.GetGfxTexInfoFromTexmap(t, mtl->texture[0], tx[0]); 		
		TextureInfo &ti = mtl->texture[0];
		if (ti.tiling[0]==GW_TEX_NO_TILING||ti.tiling[1]==GW_TEX_NO_TILING)
			needDecal = TRUE;
		op = needDecal?TXOP_ALPHABLEND:TXOP_MODULATE;
		bmi[0] = tx[0]->GetVPDisplayDIB(t,cb,valid,FALSE); 
		if (bmi[0]) {
			texHandleValid &= valid;
			useSubForTex[0] = diffChan;
			ntx = 1;
			forceW = bmi[0]->bmiHeader.biWidth;
			forceH = bmi[0]->bmiHeader.biHeight;
		}
	}
	if (tx[1]) {
		cb.GetGfxTexInfoFromTexmap(t, mtl->texture[ntx], tx[1]); 		
		if (nsupport>ntx) {
			bmi[1] = tx[1]->GetVPDisplayDIB(t,cb,valid,TRUE); 
			if (bmi[1]) {
				texHandleValid &= valid;
				StuffAlpha(bmi[1], (*maps)[opacChan].amount, GetOpacity(t),ntx?whiteCol:pShader->GetDiffuseClr(t));
				texHandle[ntx] = cb.MakeHandle(bmi[1]); 
				bmi[1] = NULL; 
				mtl->texture[ntx].textHandle = texHandle[ntx]->GetHandle();
				SetTexOps(mtl,ntx,TXOP_OPACITY);
				useSubForTex[ntx] = opacChan;
				ntx++;
			}
		}
		else {
			if (!needDecal) {
				TextureInfo ti;
				//				if (SameUV(mtl->texture[0],mtl->texture[1])) {
				// Not really correct to combine channels for different UV's but what the heck.
				bmi[1] = tx[1]->GetVPDisplayDIB(t,cb,valid,TRUE, forceW, forceH); 
				if (bmi[1]) {
					texHandleValid &= valid;
					StuffAlphaInto(bmi[1], bmi[0], (*maps)[opacChan].amount, GetOpacity(t));
					op = TXOP_OPACITY;
					xr_free(bmi[1]);
					bmi[1] = NULL;
				}
				//					}
			}
		}
	}
	if (bmi[0])	{
		texHandle[0] = cb.MakeHandle(bmi[0]); 
		bmi[0] = NULL; 
		mtl->texture[0].textHandle = texHandle[0]->GetHandle();
		SetTexOps(mtl,0,op);
	}
	mtl->texture.SetCount(ntx);
	numTexHandlesUsed = ntx;
}

// end - dds- 04.27.00 - multiple map display support

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Returns true if the evaluated color\value (output) is constant over all 
// possible inputs. This means all points of a face and all faces the 
// material is assigned to.
// If it cannot determine the correct answer, it returns false
//
bool XRayMtl::IsOutputConst
( 
 ShadeContext& sc, // describes context of evaluation
 int stdID				// must be ID_AM, ect
 )
{
	int channelID = StdIDToChannel( stdID );
	DbgAssert( 0 <= channelID && channelID < NumSubTexmaps() );
	if ( channelID < 0 || channelID >= NumSubTexmaps() )
		return false;

	if ( SubTexmapOn( channelID ) )
	{
		Texmap* texMap = GetSubTexmap( channelID );
		DbgAssert( texMap != NULL );
		if ( texMap != NULL && texMap->IsOutputMeaningful( sc ) )
			// TODO: The texmap can be meaningful but constant
			return false;
	}
	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates the material on a single texmap channel. 
// For a mono channel, the value is copied in all 3 components of the 
// output color.
//
bool XRayMtl::EvalColorStdChannel
( 
 ShadeContext& sc, // describes context of evaluation
 int stdID,				// must be ID_AM, ect
 Color& outClr			// output var
 )
{
	switch ( stdID )
	{
	case ID_BU: // Bump (value 8)
		{
			return EvalBump( sc, outClr );
		}
		break;

	case ID_RL: // Reflection (value 9)
		{
			return EvalReflection( sc, outClr );
		}
		break;

	case ID_RR: // Refraction (value 10)
		{
			return EvalRefraction( sc, outClr );
		}
		break;

	case ID_DP: // Displacement (value 11)
		{
			float outVal;
			bool bRes = EvalDisplacement( sc, outVal );
			if ( bRes )
			{
				outClr.r = outClr.g = outClr.b = outVal;
			}
			return bRes;
		}
		break;

		// For everything else, there's a smart method
	case ID_AM: // Ambient (value 0)
	case ID_DI: // Diffuse (value 1)
	case ID_SP: // Specular (value 2)
	case ID_SH: // Shininess (value 3).  In R3 and later this is called Glossiness.
	case ID_SS: // Shininess strength (value 4).  In R3 and later this is called Specular Level.
	case ID_SI: // Self-illumination (value 5)
	case ID_OP: // Opacity (value 6)
	case ID_FI: // Filter color (value 7)
		{
			int id = StdIDToChannel( stdID );
			if ( id < 0 || id >= NumSubTexmaps() )
				return false;
			return EvalChannel( sc, id, outClr );
		}
		break;

	default:
		// Should never happen
		//DbgAssert( false );
		return false;
		break;
	}
	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates the material on a single texmap channel. 
//
bool XRayMtl::EvalMonoStdChannel
( 
 ShadeContext& sc, // describes context of evaluation
 int stdID,				// must be ID_AM, ect
 float& outVal			// output var
 )
{
	switch ( stdID )
	{
	case ID_BU: // Bump (value 8)
		{
			Color outClr;
			bool bRes = EvalBump( sc, outClr );
			if ( bRes )
				outVal = Intens( outClr );
			return bRes;
		}
		break;

	case ID_RL: // Reflection (value 9)
		{
			return EvalReflection( sc, outVal );
		}
		break;

	case ID_RR: // Refraction (value 10)
		{
			return EvalRefraction( sc, outVal );
		}
		break;

	case ID_DP: // Displacement (value 11)
		{
			return EvalDisplacement( sc, outVal );
		}
		break;

		// For everything else, there's a smart method
	case ID_AM: // Ambient (value 0)
	case ID_DI: // Diffuse (value 1)
	case ID_SP: // Specular (value 2)
	case ID_SH: // Shininess (value 3).  In R3 and later this is called Glossiness.
	case ID_SS: // Shininess strength (value 4).  In R3 and later this is called Specular Level.
	case ID_SI: // Self-illumination (value 5)
	case ID_OP: // Opacity (value 6)
	case ID_FI: // Filter color (value 7)
		{
			int id = StdIDToChannel( stdID );
			if ( id < 0 || id >= NumSubTexmaps() )
				return false;
			Color outClr;
			bool bRes = EvalChannel( sc, id, outClr );
			if ( bRes )
				outVal = Intens( outClr );
			return bRes;
		}
		break;

	default:
		// Should never happen
		//DbgAssert( false );
		return false;
		break;
	}
	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// channelID is supposed to be mapped from std subtexmap ID to subtexmap id
//
bool XRayMtl::EvalChannel(ShadeContext& sc, int channelID, Color& outClr)
{
	// For exmple, the Strauss shader doesn't support the ID_AM channel
	DbgAssert( 0 <= channelID && channelID < NumSubTexmaps() );

	// Check if the mtl has or not a shader
	Shader* pShad = GetShader();
	DbgAssert( pShader != NULL );
	if ( pShad == NULL )
	{
		// Filter color and opacity values are managed by the stdmtl2?
		if ( channelID == StdIDToChannel(ID_FI) )
		{
			outClr = filter;
			return true;
		}
		else if ( channelID == StdIDToChannel(ID_FI) ) 
		{
			outClr.r = outClr.g = outClr.b = opacity;
			return true;
		}
		return false;
	}

	// Get raw color\value from the shader
	IllumParams ip(0, NULL);
	Memory.mem_fill(static_cast<void*>(&ip), 0, sizeof(IllumParams) );
	//	ip.mtlFlags = flags;
	pShad->GetIllumParams( sc, ip );

	// Filter color and opacity values are managed by the stdmtl2?
	if ( channelID == StdIDToChannel(ID_FI) )
		ip.channels[channelID] = filter;
	else if ( channelID == StdIDToChannel(ID_OP) ) 
	{
		ip.channels[channelID].r = opacity;
		ip.channels[channelID].g = opacity;
		ip.channels[channelID].b = opacity;
	}
	long channelType = pShad->ChannelType( channelID );

	// Evaluate the texture map
	if ( sc.doMaps )
	{
		if ( channelID == StdIDToChannel( ID_AM ) && 
			(flags & NEWSTDMTL_LOCK_ADTEX) == NEWSTDMTL_LOCK_ADTEX )
		{
			// Treat exceptional situations first
			//
			// Evaluat the ambient channel using the diffuse texmap 
			// if AD texmap lock is on

			long diffChanID = StdIDToChannel( ID_DI );
			long diffChanType = pShad->ChannelType( ID_DI );
			if ( diffChanID >= 0 && 
				SubTexmapOn( diffChanID ) && 
				( diffChanType & SKIP_CHANNELS ) == 0 ) 
			{
				Texmap* texMap = GetSubTexmap( diffChanID );
				if ( texMap != NULL && texMap->IsOutputMeaningful( sc ) )
				{
					// Evaluate the diffuse channel first
					AColor diffColor;
					if ( (diffChanType & MONO_CHANNEL ) == MONO_CHANNEL ) 
					{
						diffColor.r = (*maps)[diffChanID].LerpEvalMono( sc, ip.channels[diffChanID].r );
						diffColor.g = diffColor.b = diffColor.r;
						diffColor.a = 0.0f;
					}
					else //if ( (diffChanType & CLR_CHANNEL ) == CLR_CHANNEL )
					{
						diffColor = (*maps)[diffChanID].Eval( sc );
						AlphaCompCol( ip.channels[diffChanID], diffColor ); 
					}

					// Now evaluate the ambient channel
					if ( (channelType & MONO_CHANNEL ) == MONO_CHANNEL ) 
					{
						ip.channels[channelID].r = (*maps)[diffChanID].LerpEvalMono( sc, ip.channels[diffChanID].r );
						ip.channels[channelID].g = ip.channels[channelID].b = ip.channels[channelID].r;
					}
					else //if ( (channelType & CLR_CHANNEL ) == CLR_CHANNEL )
						AlphaCompCol( ip.channels[channelID], diffColor ); 
				}
			}
		}
		else
		{
			// Normal course of channel evaluation

			if ( SubTexmapOn( channelID ) && 
				( channelType & SKIP_CHANNELS ) == 0 )
			{
				Texmap* texMap = GetSubTexmap( channelID );
				if ( texMap != NULL && texMap->IsOutputMeaningful( sc ) )
				{
					if ( (channelType & MONO_CHANNEL ) == MONO_CHANNEL ) 
					{
						ip.channels[channelID].r = (*maps)[channelID].LerpEvalMono( sc, ip.channels[channelID].r );
						ip.channels[channelID].g = ip.channels[channelID].b = ip.channels[channelID].r;
					}
					else //if ( (channelType & CLR_CHANNEL ) == CLR_CHANNEL )
					{
						AColor c = (*maps)[channelID].Eval( sc );
						AlphaCompCol( ip.channels[channelID], c ); 
					}
				}
			}
		}
	}

	// Return a potentialy texture mapped value
	outClr = ip.channels[channelID];

	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates the bump map. Returns false if the result is not meaningful
//
bool XRayMtl::EvalBump( ShadeContext& sc, Color& outClr )
{
	int id = StdIDToChannel( ID_BU );
	if ( !ShouldEvalSubTexmap( sc, id ) )
	{
		outClr.Black();
		return false;
	}

	Point3 dn = (*maps)[id].EvalNormalPerturb(sc);
	outClr = Normalize( ( sc.backFace ? -dn : dn ) );

	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates reflection map as a color. Returns false if the result is not 
// meaningful
//
bool XRayMtl::EvalReflection( ShadeContext& sc, Color& outClr )
{
	int id = StdIDToChannel( ID_RL );
	if ( !ShouldEvalSubTexmap( sc, id ) )
	{
		return false;
	}

	AColor rcol;
	Texmap* reflmap = (*maps)[id].map;
	DbgAssert( reflmap != NULL );
	if ( reflmap->HandleOwnViewPerturb() ) 
	{
		sc.TossCache( reflmap );
		rcol = reflmap->EvalColor( sc );
	} 
	else 
		rcol = sc.EvalEnvironMap( reflmap, sc.ReflectVector() );

	outClr.r = rcol.r;
	outClr.g = rcol.g;
	outClr.b = rcol.b;

	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates reflection map as (mono) value. Returns false if the result 
// is not meaningful
//
bool XRayMtl::EvalReflection( ShadeContext& sc, float& outVal )
{
	int id = StdIDToChannel( ID_RL );
	if ( !ShouldEvalSubTexmap( sc, id ) )
		return false;

	Texmap* reflmap = (*maps)[id].map;
	DbgAssert( reflmap != NULL );
	if ( reflmap->HandleOwnViewPerturb() ) 
	{
		sc.TossCache( reflmap );
		outVal = reflmap->EvalMono( sc );
	} 
	else 
	{
		AColor rcol;
		rcol = sc.EvalEnvironMap( reflmap, sc.ReflectVector() );
		outVal = Intens( rcol );
	}

	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates refraction map as a color. Returns false if the result is not 
// meaningful
//
bool XRayMtl::EvalRefraction( ShadeContext& sc, Color& outClr )
{
	int id = StdIDToChannel( ID_RR );
	if ( !ShouldEvalSubTexmap( sc, id ) )
		return false;

	AColor rcol;
	Texmap* refrmap = (*maps)[id].map;
	DbgAssert( refrmap != NULL );
	if ( refrmap->HandleOwnViewPerturb() ) 
	{
		rcol = refrmap->EvalColor( sc );
	} 
	else 
		rcol = sc.EvalEnvironMap( refrmap, sc.RefractVector( ioRefract ) );

	outClr.r = rcol.r;
	outClr.g = rcol.g;
	outClr.b = rcol.b;

	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates reflection map as (mono) value. Returns false if the result 
// is not meaningful
//
bool XRayMtl::EvalRefraction( ShadeContext& sc, float& outVal )
{
	int id = StdIDToChannel( ID_RR );
	if ( !ShouldEvalSubTexmap( sc, id ) )
		return false;

	Texmap* refrmap = (*maps)[id].map;
	DbgAssert( refrmap != NULL );
	if ( refrmap->HandleOwnViewPerturb() ) 
	{
		outVal = refrmap->EvalMono( sc );
	} 
	else 
	{
		AColor rcol;
		rcol = sc.EvalEnvironMap( refrmap, sc.RefractVector( ioRefract ) );
		outVal = Intens( rcol );
	}

	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Evaluates Displacement map as a color. Returns false if the result is not 
// meaningful
//
bool XRayMtl::EvalDisplacement( ShadeContext& sc, float& outVal )
{
	int id = StdIDToChannel( ID_DP );
	if ( !ShouldEvalSubTexmap( sc, id ) )
		return false;

	outVal = (*maps)[id].EvalMono(sc);
	return true;
}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Helper method
//
bool XRayMtl::ShouldEvalSubTexmap( ShadeContext& sc, int id )
{
	if ( id >= 0 && id < NumSubTexmaps() &&
		sc.doMaps &&	 SubTexmapOn( id ) )
	{
		Texmap* texMap = GetSubTexmap( id );
		if ( texMap != NULL && texMap->IsOutputMeaningful( sc ) )
			return true;
	}

	return false;
}

// XRay Part
void XRayMtl::LoadXRayShaderList()
{
	XRayMtl::EShaders.clear	();
	XRayMtl::CShaders.clear	();
	XRayMtl::GameMtls.clear ();
	int e_res = LoadBlenderList	(XRayMtl::EShaders);
	int c_res = LoadShaderLCList(XRayMtl::CShaders);
	int g_res = LoadGameMtlList	(XRayMtl::GameMtls);
	if ((e_res==-1)||(c_res==-1)||(g_res==-1))
	{
		Log("Can't load shader files!\nGame material will work incorrectly.");
		THROW;
	}
}

void XRayMtl::UnloadXRayShaderList()
{
	ClearList(XRayMtl::EShaders);
	ClearList(XRayMtl::CShaders);
	ClearList(XRayMtl::GameMtls);
}

void XRayMtl::SetEShaderIndx( long indx ) 
{
	pb_xray->SetValue(std2_eshader_type, 0, indx<0 ? 0 : indx );
	eshaderId = indx; 
}
void XRayMtl::SetCShaderIndx( long indx ) 
{
	pb_xray->SetValue(std2_cshader_type, 0, indx<0 ? 0 : indx );
	cshaderId = indx; 
}
void XRayMtl::SetGameMtlIndx( long indx ) 
{
	pb_xray->SetValue(std2_gamemtl_type, 0, indx<0 ? 0 : indx );
	gamemtlId = indx; 
}
TCHAR*	XRayMtl::GetEShader	(DWORD i)
{ 
	return (i<EShaders.size())?(TCHAR*)EShaders[i]:0;
}
TCHAR*	XRayMtl::GetCShader	(DWORD i)
{ 
	return (i<CShaders.size())?(TCHAR*)CShaders[i]:0;
}
TCHAR*	XRayMtl::GetGameMtl	(DWORD i)
{ 
	return (i<GameMtls.size())?(TCHAR*)GameMtls[i]:0;
}

int		XRayMtl::FindEShader(LPCSTR name)
{
	if (!name) return -1;
	for (LPSTRIt it=XRayMtl::EShaders.begin(); it!=XRayMtl::EShaders.end(); it++)
		if (0==xr_strcmp(*it,name)) return it-XRayMtl::EShaders.begin();
	return -1;
}

int		XRayMtl::FindCShader(LPCSTR name)
{
	if (!name) return -1;
	for (LPSTRIt it=XRayMtl::CShaders.begin(); it!=XRayMtl::CShaders.end(); it++)
		if (0==xr_strcmp(*it,name)) return it-XRayMtl::CShaders.begin();
	return -1;
}

int		XRayMtl::FindGameMtl(LPCSTR name)
{
	if (!name) return -1;
	for (LPSTRIt it=XRayMtl::GameMtls.begin(); it!=XRayMtl::GameMtls.end(); it++)
		if (0==xr_strcmp(*it,name)) return it-XRayMtl::GameMtls.begin();
	return -1;
}
