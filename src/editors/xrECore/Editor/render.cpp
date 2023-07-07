#include "stdafx.h"
#include "render.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

float ssaDISCARD = 4.f;
float ssaDONTSORT = 32.f;

ECORE_API float r_ssaDISCARD;
ECORE_API float g_fSCREEN;

CRender RImplementation;
ECORE_API CRender *Render = &RImplementation;

CRender::CRender()
{
	m_skinning = 0;
}

CRender::~CRender()
{
	xr_delete(Target);
}

void CRender::Initialize()
{
    c_ssky0 = "s_sky0";
    c_ssky1 = "s_sky1";
    c_sclouds0 = "s_clouds0";
    c_sclouds1 = "s_clouds1";
	PSLibrary.OnCreate();
}
void CRender::ShutDown()
{
	PSLibrary.OnDestroy();
}

void CRender::OnDeviceCreate(pcstr str)
{
	Models = xr_new<CModelPool>();
	Models->Logging(FALSE);
}
void CRender::OnDeviceDestroy(bool keep_textures)
{
	xr_delete(Models);
}

ref_shader CRender::getShader(int id) { return 0; } // VERIFY(id<int(Shaders.size()));	return Shaders[id];	}

bool CRender::occ_visible(Fbox &B)
{
	u32 mask = 0xff;
	return ViewBase.testAABB(B.data(), mask);
}

bool CRender::occ_visible(sPoly &P)
{
	return ViewBase.testPolyInside(P);
}

bool CRender::occ_visible(vis_data &P)
{
	return occ_visible(P.box);
}

void CRender::Calculate()
{
	// Transfer to global space to avoid deep pointer access
	g_fSCREEN = float(EDevice.m_RenderWidth * EDevice.m_RenderHeight);
	r_ssaDISCARD = (ssaDISCARD * ssaDISCARD) / g_fSCREEN;
	//	r_ssaLOD_A						=	(ssaLOD_A*ssaLOD_A)/g_fSCREEN;
	//	r_ssaLOD_B						=	(ssaLOD_B*ssaLOD_B)/g_fSCREEN;

	ViewBase.CreateFromMatrix(EDevice.mFullTransform, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);
}

#ifndef _EDITOR
#include "environment.h"
#endif
void CRender::Render()
{
}

void CRender::RenderMenu()
{
}

IRender_DetailModel *CRender::model_CreateDM(IReader *F)
{
	VERIFY(F);
	CDetail *D = xr_new<CDetail>();
	D->Load(F);
	return D;
}

IRenderVisual *CRender::model_CreatePE(LPCSTR name)
{
	PS::CPEDef *source = PSLibrary.FindPED(name);
	return Models->CreatePE(source);
}

IRenderVisual *CRender::model_CreateParticles(LPCSTR name)
{
	PS::CPEDef *SE = PSLibrary.FindPED(name);
	if (SE)
		return Models->CreatePE(SE);
	else
	{
		PS::CPGDef *SG = PSLibrary.FindPGD(name);
		return SG ? Models->CreatePG(SG) : 0;
	}
}

void CRender::rmNear(CBackend& cmd_list)
{
    IRender_Target* T = getTarget();
    RCache.SetViewport({ 0, 0, T->get_width(RCache), T->get_height(RCache), 0, 0.02f });
}

void CRender::rmFar(CBackend& cmd_list)
{
    IRender_Target* T = getTarget();
    RCache.SetViewport({ 0, 0, T->get_width(RCache), T->get_height(RCache), 0.99999f, 1.f });
}

void CRender::rmNormal(CBackend& cmd_list)
{
    IRender_Target* T = getTarget();
    RCache.SetViewport({ 0, 0, T->get_width(RCache), T->get_height(RCache), 0, 1.f });
}

void CRender::set_Transform(Fmatrix *M)
{
	current_matrix.set(*M);
}

void CRender::add_Visual(IRenderVisual *visual) { Models->RenderSingle(dynamic_cast<dxRender_Visual *>(visual), current_matrix, 1.f); }
IRenderVisual *CRender::model_Create(LPCSTR name, IReader *data) { return Models->Create(name, data); }
IRenderVisual *CRender::model_CreateChild(LPCSTR name, IReader *data) { return Models->CreateChild(name, data); }
void CRender::model_Delete(IRenderVisual *&V, bool bDiscard)
{
	auto v = dynamic_cast<dxRender_Visual *>(V);
	Models->Delete(v, bDiscard);
	if (v == nullptr)
		V = nullptr;
}
IRenderVisual *CRender::model_Duplicate(IRenderVisual *V) { return Models->Instance_Duplicate(dynamic_cast<dxRender_Visual *>(V)); }
void CRender::model_Render(IRenderVisual *m_pVisual, const Fmatrix &mTransform, int priority, bool strictB2F, float m_fLOD) { Models->Render(dynamic_cast<dxRender_Visual *>(m_pVisual), mTransform, priority, strictB2F, m_fLOD); }
void CRender::model_RenderSingle(IRenderVisual *m_pVisual, const Fmatrix &mTransform, float m_fLOD) { Models->RenderSingle(dynamic_cast<dxRender_Visual *>(m_pVisual), mTransform, m_fLOD); }

//#pragma comment(lib,"d3dx_r1")
HRESULT CRender::CompileShader(
	LPCSTR pSrcData,
	UINT SrcDataLen,
	void *_pDefines,
	void *_pInclude,
	LPCSTR pFunctionName,
	LPCSTR pTarget,
	DWORD Flags,
	void *_ppShader,
	void *_ppErrorMsgs,
	void *_ppConstantTable)
{
	CONST D3DXMACRO *pDefines = (CONST D3DXMACRO *)_pDefines;
	LPD3DXINCLUDE pInclude = (LPD3DXINCLUDE)_pInclude;
	LPD3DXBUFFER *ppShader = (LPD3DXBUFFER *)_ppShader;
	LPD3DXBUFFER *ppErrorMsgs = (LPD3DXBUFFER *)_ppErrorMsgs;
	LPD3DXCONSTANTTABLE *ppConstantTable = (LPD3DXCONSTANTTABLE *)_ppConstantTable;
	return D3DXCompileShader(pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName, pTarget, Flags, ppShader, ppErrorMsgs, ppConstantTable);
}


class includer : public ID3DXInclude
{
public:
    HRESULT __stdcall Open(
        D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        string_path pname;
        strconcat(sizeof(pname), pname, GEnv.Render->getShaderPath(), pFileName);
        IReader* R = FS.r_open("$game_shaders$", pname);
        if (nullptr == R)
        {
            // possibly in shared directory or somewhere else - open directly
            R = FS.r_open("$game_shaders$", pFileName);
            if (nullptr == R)
                return E_FAIL;
        }

        // duplicate and zero-terminate
        const size_t size = R->length();
        u8* data = xr_alloc<u8>(size + 1);
        CopyMemory(data, R->pointer(), size);
        data[size] = 0;
        FS.r_close(R);

        *ppData = data;
        *pBytes = size;
        return D3D_OK;
    }
    HRESULT __stdcall Close(LPCVOID pData)
    {
        xr_free(pData);
        return D3D_OK;
    }
};

template <typename T>
static HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    T*& result, bool const disasm)
{
    HRESULT _hr = ShaderTypeTraits<T>::CreateHWShader(buffer, buffer_size, result->sh);
    if (!SUCCEEDED(_hr))
    {
        Log("! Shader: ", file_name);
        Msg("! CreateHWShader hr == 0x%08x", _hr);
        return E_FAIL;
    }

    LPCVOID data = nullptr;

    _hr = D3DXFindShaderComment(buffer, MAKEFOURCC('C', 'T', 'A', 'B'), &data, nullptr);

    if (SUCCEEDED(_hr) && data)
    {
        // Parse constant table data
        LPD3DXSHADER_CONSTANTTABLE pConstants = LPD3DXSHADER_CONSTANTTABLE(data);
        result->constants.parse(pConstants, ShaderTypeTraits<T>::GetShaderDest());
    }
    else
        Msg("! D3DXFindShaderComment %s hr == 0x%08x", file_name, _hr);

    if (disasm)
    {
        ID3DXBuffer* disasm = nullptr;
        D3DXDisassembleShader(LPDWORD(buffer), FALSE, nullptr, &disasm);
        if (!disasm)
            return _hr;

        string_path dname;
        strconcat(sizeof(dname), dname, "disasm" DELIMITER, file_name, ('v' == pTarget[0]) ? ".vs" : ".ps");
        IWriter* W = FS.w_open("$app_data_root$", dname);
        W->w(disasm->GetBufferPointer(), disasm->GetBufferSize());
        FS.w_close(W);
        _RELEASE(disasm);
    }

    return _hr;
}

inline HRESULT create_shader(LPCSTR const pTarget, DWORD const* buffer, u32 const buffer_size, LPCSTR const file_name,
    void*& result, bool const disasm)
{
    if (pTarget[0] == 'p')
        return create_shader(pTarget, buffer, buffer_size, file_name, (SPS*&)result, disasm);

    if (pTarget[0] == 'v')
        return create_shader(pTarget, buffer, buffer_size, file_name, (SVS*&)result, disasm);

    NODEFAULT;
    return E_FAIL;
}

HRESULT CRender::shader_compile(pcstr name, IReader* fs, pcstr pFunctionName, pcstr pTarget, u32 Flags, void*& result)
{
	D3DXMACRO defines[128];
	int def_it = 0;

	// options
	if (m_skinning < 0)
	{
		defines[def_it].Name = "SKIN_NONE";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (0 == m_skinning)
	{
		defines[def_it].Name = "SKIN_0";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (1 == m_skinning)
	{
		defines[def_it].Name = "SKIN_1";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (2 == m_skinning)
	{
		defines[def_it].Name = "SKIN_2";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (3 == m_skinning)
	{
		defines[def_it].Name = "SKIN_3";
		defines[def_it].Definition = "1";
		def_it++;
	}
	if (4 == m_skinning)
	{
		defines[def_it].Name = "SKIN_4";
		defines[def_it].Definition = "1";
		def_it++;
	}
	// finish
	defines[def_it].Name = 0;
	defines[def_it].Definition = 0;
	def_it++;

    fs->seek(0);

    includer Includer;
    LPD3DXBUFFER pShaderBuf = nullptr;
    LPD3DXBUFFER pErrorBuf = nullptr;
    LPD3DXCONSTANTTABLE pConstants = nullptr;
    LPD3DXINCLUDE pInclude = (LPD3DXINCLUDE)&Includer;

    HRESULT _result = D3DXCompileShader((LPCSTR)fs->pointer(), fs->length(), defines, pInclude, pFunctionName, pTarget,
        Flags | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL, &pShaderBuf, &pErrorBuf, &pConstants);
    if (SUCCEEDED(_result))
    {
#ifdef DEBUG
        Log("- Compile shader:", name);
#endif
        _result = create_shader(
            pTarget, (DWORD*)pShaderBuf->GetBufferPointer(), pShaderBuf->GetBufferSize(), name, result, o.disasm);
    }
    else
    {
        Log("! ", name);
        if (pErrorBuf)
            Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
        else
            Msg("Can't compile shader hr=0x%08x", _result);
    }

	return _result;
}

void CRender::reset_begin()
{
	xr_delete(Target);
}
void CRender::reset_end()
{
	Target = xr_new<CRenderTarget>();
}
