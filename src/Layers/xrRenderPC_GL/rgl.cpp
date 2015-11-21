#include "stdafx.h"
#include "rgl.h"
#include "../xrRender/fbasicvisual.h"
#include "glWallMarkArray.h"
#include "glUIShader.h"
#include "glRenderDeviceRender.h"

CRender		RImplementation;

extern ENGINE_API BOOL r2_sun_static;
extern ENGINE_API BOOL r2_advanced_pp;	//	advanced post process and effects

float		r_dtex_range = 50.f;
//////////////////////////////////////////////////////////////////////////
ShaderElement*			CRender::rimp_select_sh_dynamic(dxRender_Visual	*pVisual, float cdist_sq)
{
	int		id = SE_R2_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R)<r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}
//////////////////////////////////////////////////////////////////////////
ShaderElement*			CRender::rimp_select_sh_static(dxRender_Visual	*pVisual, float cdist_sq)
{
	int		id = SE_R2_SHADOW;
	if (CRender::PHASE_NORMAL == RImplementation.phase)
	{
		id = ((_sqrt(cdist_sq) - pVisual->vis.sphere.R)<r_dtex_range) ? SE_R2_NORMAL_HQ : SE_R2_NORMAL_LQ;
	}
	return pVisual->shader->E[id]._get();
}
static class cl_parallax		: public R_constant_setup		{	virtual void setup	(R_constant* C)
{
	float			h			=	ps_r2_df_parallax_h;
	RCache.set_c	(C,h,-h/2.f,1.f/r_dtex_range,1.f/r_dtex_range);
}}	binder_parallax;

static class cl_pos_decompress_params		: public R_constant_setup		{	virtual void setup	(R_constant* C)
{
	float VertTan =  tanf( deg2rad(Device.fFOV/2.0f ) );
	float HorzTan =  VertTan / Device.fASPECT;

	RCache.set_c	( C, HorzTan, VertTan, ( 2.0f * HorzTan )/(float)Device.dwWidth, ( 2.0f * VertTan ) /(float)Device.dwHeight );

}}	binder_pos_decompress_params;

static class cl_pos_decompress_params2		: public R_constant_setup		{	virtual void setup	(R_constant* C)
{
	RCache.set_c	(C,(float)Device.dwWidth, (float)Device.dwHeight, 1.0f/(float)Device.dwWidth, 1.0f/(float)Device.dwHeight );

}}	binder_pos_decompress_params2;

static class cl_water_intensity : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E.m_fWaterIntensity;
		RCache.set_c	(C, fValue, fValue, fValue, 0);
	}
}	binder_water_intensity;

static class cl_sun_shafts_intensity : public R_constant_setup		
{	
	virtual void setup	(R_constant* C)
	{
		CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E.m_fSunShaftsIntensity;
		RCache.set_c	(C, fValue, fValue, fValue, 0);
	}
}	binder_sun_shafts_intensity;

void					CRender::model_Delete(IRenderVisual* &V, BOOL bDiscard)
{
	dxRender_Visual* pVisual = (dxRender_Visual*)V;
	Models->Delete(pVisual, bDiscard);
	V = 0;
}

IRender_DetailModel*	CRender::model_CreateDM(IReader*	F)
{
	CDetail*	D = new CDetail();
	D->Load(F);
	return D;
}

void					CRender::model_Delete(IRender_DetailModel* & F)
{
	if (F)
	{
		CDetail*	D = (CDetail*)F;
		D->Unload();
		xr_delete(D);
		F = NULL;
	}
}

IRenderVisual*			CRender::model_CreatePE(LPCSTR name)
{
	PS::CPEDef*	SE = PSLibrary.FindPED(name);		R_ASSERT3(SE, "Particle effect doesn't exist", name);
	return					Models->CreatePE(SE);
}

IRenderVisual*			CRender::model_CreateParticles(LPCSTR name)
{
	PS::CPEDef*	SE = PSLibrary.FindPED(name);
	if (SE) return			Models->CreatePE(SE);
	else{
		PS::CPGDef*	SG = PSLibrary.FindPGD(name);		R_ASSERT3(SG, "Particle effect or group doesn't exist", name);
		return				Models->CreatePG(SG);
	}
}

CRender::CRender()
{
}

CRender::~CRender()
{
}

void					CRender::create()
{
	//Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 0x12345678);

	m_skinning = -1;
	m_MSAASample = -1;

	// hardware
	o.smapsize = 2048;
	o.mrt = TRUE;
	o.mrtmixdepth = TRUE;

	// Check for NULL render target support
	o.nullrt = false;

	// SMAP / DST
	o.HW_smap_FETCH4 = FALSE;
	o.HW_smap = true;
	o.HW_smap_PCF = o.HW_smap;
	if (o.HW_smap)
	{
		o.HW_smap_FORMAT = D3DFMT_D24X8;
		Msg("* HWDST/PCF supported and used");
	}

	o.fp16_filter = true;
	o.fp16_blend = true;

	if (o.mrtmixdepth)		o.albedo_wo = FALSE;
	else if (o.fp16_blend)	o.albedo_wo = FALSE;
	else					o.albedo_wo = TRUE;

	// nvstencil on NV40 and up
	o.nvstencil = FALSE;
	if (strstr(Core.Params, "-nonvs"))		o.nvstencil = FALSE;

	// nv-dbt
	o.nvdbt = glewIsSupported("GL_EXT_depth_bounds_test");
	if (o.nvdbt)		Msg("* NV-DBT supported and used");

	// options (smap-pool-size)
	if (strstr(Core.Params, "-smap1536"))	o.smapsize = 1536;
	if (strstr(Core.Params, "-smap2048"))	o.smapsize = 2048;
	if (strstr(Core.Params, "-smap2560"))	o.smapsize = 2560;
	if (strstr(Core.Params, "-smap3072"))	o.smapsize = 3072;
	if (strstr(Core.Params, "-smap4096"))	o.smapsize = 4096;

	// gloss
	char*	g = strstr(Core.Params, "-gloss ");
	o.forcegloss = g ? TRUE : FALSE;
	if (g)				{
		o.forcegloss_v = float(atoi(g + xr_strlen("-gloss "))) / 255.f;
	}

	// options
	o.bug = (strstr(Core.Params, "-bug")) ? TRUE : FALSE;
	o.sunfilter = (strstr(Core.Params, "-sunfilter")) ? TRUE : FALSE;
	//.	o.sunstatic			= (strstr(Core.Params,"-sunstatic"))?	TRUE	:FALSE	;
	o.sunstatic = r2_sun_static;
	o.advancedpp = r2_advanced_pp;
	//o.volumetricfog = ps_r2_ls_flags.test(R3FLAG_VOLUMETRIC_SMOKE);
	o.sjitter = (strstr(Core.Params, "-sjitter")) ? TRUE : FALSE;
	o.depth16 = (strstr(Core.Params, "-depth16")) ? TRUE : FALSE;
	o.noshadows = (strstr(Core.Params, "-noshadows")) ? TRUE : FALSE;
	o.Tshadows = (strstr(Core.Params, "-tsh")) ? TRUE : FALSE;
	o.mblur = (strstr(Core.Params, "-mblur")) ? TRUE : FALSE;
	o.distortion_enabled = (strstr(Core.Params, "-nodistort")) ? FALSE : TRUE;
	o.distortion = o.distortion_enabled;
	o.disasm = (strstr(Core.Params, "-disasm")) ? TRUE : FALSE;
	o.forceskinw = (strstr(Core.Params, "-skinw")) ? TRUE : FALSE;

	o.ssao_blur_on = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_BLUR) && (ps_r_ssao != 0);
	o.ssao_opt_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_OPT_DATA) && (ps_r_ssao != 0);
	o.ssao_half_data = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HALF_DATA) && o.ssao_opt_data && (ps_r_ssao != 0);
	o.ssao_hdao = ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HDAO) && (ps_r_ssao != 0);
	o.ssao_hbao = !o.ssao_hdao && ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HBAO) && (ps_r_ssao != 0);


	// constants
	glRenderDeviceRender::Instance().Resources->RegisterConstantSetup("parallax", &binder_parallax);
	glRenderDeviceRender::Instance().Resources->RegisterConstantSetup("water_intensity", &binder_water_intensity);
	glRenderDeviceRender::Instance().Resources->RegisterConstantSetup("sun_shafts_intensity", &binder_sun_shafts_intensity);
	glRenderDeviceRender::Instance().Resources->RegisterConstantSetup("pos_decompression_params", &binder_pos_decompress_params);

	c_lmaterial = "L_material";
	c_sbase = "s_base";

	Target = new CRenderTarget();	// Main target

	Models = new CModelPool();
	PSLibrary.OnCreate();
	HWOCC.occq_create(occq_size);

	rmNormal					();
	marker = 0;
	//R_CHK						(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT,&q_sync_point[0]));
	//R_CHK						(HW.pDevice->CreateQuery(D3DQUERYTYPE_EVENT,&q_sync_point[1]));
	ZeroMemory(q_sync_point, sizeof(q_sync_point));

	xrRender_apply_tf();
	::PortalTraverser.initialize();
}

void CRender::LoadIncludes(LPCSTR pSrcData, UINT SrcDataLen, xr_vector<char*>& source, xr_vector<char*>& includes)
{
	char* srcData = xr_alloc<char>(SrcDataLen + 2);
	memcpy(srcData, pSrcData, SrcDataLen);
	srcData[SrcDataLen] = '\n';
	srcData[SrcDataLen + 1] = '\0';
	includes.push_back(srcData);
	source.push_back(srcData);

	string_path path;
	char* str = srcData;
	while (strstr(str, "#include") != nullptr)
	{
		// Get filename of include directive
		str = strstr(str, "#include");		// Find the include directive
		char* fn = strchr(str, '"') + 1;	// Get filename, skip quotation
		*str = '\0';						// Terminate previous source
		str = strchr(fn, '"');				// Get end of filename path
		*str = '\0';						// Terminate filename path

		// Create path to included shader
		strconcat(sizeof(path), path, ::Render->getShaderPath(), fn);
		FS.update_path(path, "$game_shaders$", path);
		while (char* sep = strchr(path, '/')) *sep = '\\';

		// Open and read file, recursively load includes
		IReader* R = FS.r_open(path);
		R_ASSERT2(R, path);
		LoadIncludes((char*)R->pointer(), R->length(), source, includes);
		FS.r_close(R);

		// Add next source, skip quotation
		str++;
		source.push_back(str);
	}
}

struct SHADER_MACRO {
	char *Define = "#define ", *Name = "\n", *Sep = "\t", *Definition = "\n", *EOL = "\n";
};

HRESULT	CRender::shader_compile(
	LPCSTR							name,
	LPCSTR                          pSrcData,
	UINT                            SrcDataLen,
	void*							_pDefines,
	void*							_pInclude,
	LPCSTR                          pFunctionName,
	LPCSTR                          pTarget,
	DWORD                           Flags,
	void*							_ppShader,
	void*							_ppErrorMsgs,
	void*							_ppConstantTable)
{
	xr_vector<char*>				source, includes;
	SHADER_MACRO					defines[128];
	int								def_it = 0;
	char							c_smapsize[32];
	char							c_gloss[32];
	char							c_sun_shafts[32];
	char							c_ssao[32];
	char							c_sun_quality[32];

	// TODO: OGL: Implement these parameters.
	VERIFY(!_pDefines);
	VERIFY(!_pInclude);
	VERIFY(!pFunctionName);
	VERIFY(!pTarget);
	VERIFY(!Flags);
	VERIFY(!_ppConstantTable);

	// open included files
	LoadIncludes(pSrcData, SrcDataLen, source, includes);

	// options
	{
		sprintf(c_smapsize, "%d", u32(o.smapsize));
		defines[def_it].Name = "SMAP_size";
		defines[def_it].Definition = c_smapsize;
		def_it++;
	}
	if (o.fp16_filter)		{
		defines[def_it].Name = "FP16_FILTER";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.fp16_blend)		{
		defines[def_it].Name = "FP16_BLEND";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.HW_smap)			{
		defines[def_it].Name = "USE_HWSMAP";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.HW_smap_PCF)			{
		defines[def_it].Name = "USE_HWSMAP_PCF";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.HW_smap_FETCH4)			{
		defines[def_it].Name = "USE_FETCH4";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.sjitter)			{
		defines[def_it].Name = "USE_SJITTER";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.Tshadows)			{
		defines[def_it].Name = "USE_TSHADOWS";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.mblur)			{
		defines[def_it].Name = "USE_MBLUR";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.sunfilter)		{
		defines[def_it].Name = "USE_SUNFILTER";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.sunstatic)		{
		defines[def_it].Name = "USE_R2_STATIC_SUN";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (o.forcegloss)		{
		sprintf(c_gloss, "%f", o.forcegloss_v);
		defines[def_it].Name = "FORCE_GLOSS";
		defines[def_it].Definition = c_gloss;
		def_it++;
	}
	if (o.forceskinw)		{
		defines[def_it].Name = "SKIN_COLOR";
		defines[def_it].Definition = "1";
		def_it++;
	}

	if (o.ssao_blur_on)
	{
		defines[def_it].Name = "USE_SSAO_BLUR";
		defines[def_it].Definition = "1";
		def_it++;
	}

	if (o.ssao_opt_data)
	{
		defines[def_it].Name = "SSAO_OPT_DATA";
		if (o.ssao_half_data)
			defines[def_it].Definition = "2";
		else
			defines[def_it].Definition = "1";
		def_it++;
	}

	if (o.ssao_hdao)
	{
		defines[def_it].Name = "HDAO";
		defines[def_it].Definition = "1";
		def_it++;
	}

	if (o.ssao_hbao)
	{
		defines[def_it].Name = "USE_HBAO";
		defines[def_it].Definition = "1";
		def_it++;
	}

	// skinning
	if (m_skinning<0)		{
		defines[def_it].Name = "SKIN_NONE";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (0 == m_skinning)		{
		defines[def_it].Name = "SKIN_0";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (1 == m_skinning)		{
		defines[def_it].Name = "SKIN_1";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (2 == m_skinning)		{
		defines[def_it].Name = "SKIN_2";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (3 == m_skinning)		{
		defines[def_it].Name = "SKIN_3";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (4 == m_skinning)		{
		defines[def_it].Name = "SKIN_4";
		defines[def_it].Definition = "1";
		def_it++;
	}
	R_ASSERT(m_skinning < 5);

	//	Igor: need restart options
	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_WATER))
	{
		defines[def_it].Name = "USE_SOFT_WATER";
		defines[def_it].Definition = "1";
		def_it++;
	}

	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_SOFT_PARTICLES))
	{
		defines[def_it].Name = "USE_SOFT_PARTICLES";
		defines[def_it].Definition = "1";
		def_it++;
	}

	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_DOF))
	{
		defines[def_it].Name = "USE_DOF";
		defines[def_it].Definition = "1";
		def_it++;
	}

	if (RImplementation.o.advancedpp && ps_r_sun_shafts)
	{
		sprintf_s(c_sun_shafts, "%d", ps_r_sun_shafts);
		defines[def_it].Name = "SUN_SHAFTS_QUALITY";
		defines[def_it].Definition = c_sun_shafts;
		def_it++;
	}

	if (RImplementation.o.advancedpp && ps_r_ssao)
	{
		sprintf_s(c_ssao, "%d", ps_r_ssao);
		defines[def_it].Name = "SSAO_QUALITY";
		defines[def_it].Definition = c_ssao;
		def_it++;
	}

	if (RImplementation.o.advancedpp && ps_r_sun_quality)
	{
		sprintf_s(c_sun_quality, "%d", ps_r_sun_quality);
		defines[def_it].Name = "SUN_QUALITY";
		defines[def_it].Definition = c_sun_quality;
		def_it++;
	}

	if (RImplementation.o.advancedpp && ps_r2_ls_flags.test(R2FLAG_STEEP_PARALLAX))
	{
		defines[def_it].Name = "ALLOW_STEEPPARALLAX";
		defines[def_it].Definition = "1";
		def_it++;
	}

	// Compile sources list
	size_t def_len = def_it * 5;
	size_t sources_len = source.size() + def_len + 2;
	string256 name_comment;
	sprintf_s(name_comment, "// %s\n", name);
	const char** sources = xr_alloc<const char*>(sources_len);
	sources[0] = "#version 410\n";
	sources[1] = name_comment;
	memcpy(sources + 2, defines, def_len * sizeof(char*));
	memcpy(sources + def_len + 2, source.data(), source.size() * sizeof(char*));

	// Compile the shader
	GLuint shader = *(GLuint*)_ppShader;
	R_ASSERT(shader);
	CHK_GL(glShaderSource(shader, sources_len, sources, nullptr));
	CHK_GL(glCompileShader(shader));

	// Create the shader program
	GLuint program = glCreateProgram();
	R_ASSERT(program);
	CHK_GL(glObjectLabel(GL_PROGRAM, program, -1, name));
	CHK_GL(glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE));
	*(GLuint*)_ppShader = program;

	// Free string resources
	xr_free(sources);
	for (xr_vector<char*>::iterator it = includes.begin(); it != includes.end(); it++)
		xr_free(*it);

	// Get the compilation result
	GLint result;
	CHK_GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &result));

	// Link program if compilation succeeded
	if (result) {
		CHK_GL(glAttachShader(program, shader));
		CHK_GL(glLinkProgram(program));
		CHK_GL(glDetachShader(program, shader));
		CHK_GL(glGetProgramiv(program, GL_LINK_STATUS, &result));

		if (_ppErrorMsgs)
		{
			// Get the compilation log, if requested
			GLint length;
			GLchar** _pErrorMsgs = (GLchar**)_ppErrorMsgs;
			CHK_GL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
			*_pErrorMsgs = xr_alloc<GLchar>(length);
			CHK_GL(glGetProgramInfoLog(program, length, nullptr, *_pErrorMsgs));
		}
	}
	else if (_ppErrorMsgs)
	{
		// Get the compilation log, if requested
		GLint length;
		GLchar** _pErrorMsgs = (GLchar**)_ppErrorMsgs;
		CHK_GL(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
		*_pErrorMsgs = xr_alloc<GLchar>(length);
		CHK_GL(glGetShaderInfoLog(shader, length, nullptr, *_pErrorMsgs));
	}

	CHK_GL(glDeleteShader(shader));
	return		result;
}

void CRender::reset_begin()
{
	// Update incremental shadowmap-visibility solver
	// BUG-ID: 10646
	{
		u32 it = 0;
		for (it = 0; it<Lights_LastFrame.size(); it++)	{
			if (0 == Lights_LastFrame[it])	continue;
			try {
				Lights_LastFrame[it]->svis.resetoccq();
			}
			catch (...)
			{
				Msg("! Failed to flush-OCCq on light [%d] %X", it, *(u32*)(&Lights_LastFrame[it]));
			}
		}
		Lights_LastFrame.clear();
	}

	xr_delete(Target);
	HWOCC.occq_destroy();
}

void CRender::reset_end()
{
	HWOCC.occq_create(occq_size);
	Target = new CRenderTarget();
	xrRender_apply_tf();
}

void CRender::rmNear()
{
	IRender_Target* T = getTarget();
	CHK_GL(glViewport(0, 0, T->get_width(), T->get_height()));
	CHK_GL(glDepthRangef(0.f, 0.02f));
}

void CRender::rmFar()
{
	IRender_Target* T = getTarget();
	CHK_GL(glViewport(0, 0, T->get_width(), T->get_height()));
	CHK_GL(glDepthRangef(0.99999f, 1.f));
}

void CRender::rmNormal()
{
	IRender_Target* T = getTarget();
	CHK_GL(glViewport(0, 0, T->get_width(), T->get_height()));
	CHK_GL(glDepthRangef(0.f, 1.f));
}

void CRender::add_StaticWallmark(ref_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* verts)
{
	if (T->suppress_wm)	return;
	VERIFY2(_valid(P) && _valid(s) && T && verts && (s>EPS_L), "Invalid static wallmark params");
	Wallmarks->AddStaticWallmark(T, verts, P, &*S, s);
}

void CRender::add_StaticWallmark(IWallMarkArray *pArray, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	glWallMarkArray *pWMA = (glWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->glGenerateWallmark();
	if (pShader) add_StaticWallmark(*pShader, P, s, T, V);
}

void CRender::add_StaticWallmark(const wm_shader& S, const Fvector& P, float s, CDB::TRI* T, Fvector* V)
{
	glUIShader* pShader = (glUIShader*)&*S;
	add_StaticWallmark(pShader->hShader, P, s, T, V);
}

void CRender::add_SkeletonWallmark(const Fmatrix* xf, CKinematics* obj, ref_shader& sh, const Fvector& start, const Fvector& dir, float size)
{
	Wallmarks->AddSkeletonWallmark(xf, obj, sh, start, dir, size);
}

void CRender::add_SkeletonWallmark(const Fmatrix* xf, IKinematics* obj, IWallMarkArray *pArray, const Fvector& start, const Fvector& dir, float size)
{
	glWallMarkArray *pWMA = (glWallMarkArray *)pArray;
	ref_shader *pShader = pWMA->glGenerateWallmark();
	if (pShader) add_SkeletonWallmark(xf, (CKinematics*)obj, *pShader, start, dir, size);
}
