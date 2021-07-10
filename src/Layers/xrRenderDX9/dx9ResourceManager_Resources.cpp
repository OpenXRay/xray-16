#include "stdafx.h"

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/tss.h"
#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_Recorder.h"
#include "Layers/xrRender/ShaderResourceTraits.h"

template <class T>
BOOL reclaim(xr_vector<T*>& vec, const T* ptr)
{
    auto it = vec.begin();
    auto end = vec.end();
    for (; it != end; ++it)
    {
        if (*it == ptr)
        {
            vec.erase(it);
            return TRUE;
        }
    }
    return FALSE;
}

//--------------------------------------------------------------------------------------------------------------
SPass* CResourceManager::_CreatePass(const SPass& proto)
{
    for (SPass* pass : v_passes)
        if (pass->equal(proto))
            return pass;

    SPass* P = v_passes.emplace_back(xr_new<SPass>());
    P->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    P->state = proto.state;
    P->ps = proto.ps;
    P->vs = proto.vs;
    P->constants = proto.constants;
    P->T = proto.T;
#ifdef _EDITOR
    P->M = proto.M;
#endif
    P->C = proto.C;

    return P;
}

//--------------------------------------------------------------------------------------------------------------
static BOOL dcl_equal(VertexElement* a, VertexElement* b)
{
    // check sizes
    u32 a_size = GetDeclLength(a);
    u32 b_size = GetDeclLength(b);
    if (a_size != b_size)
        return FALSE;
    return 0 == memcmp(a, b, a_size * sizeof(VertexElement));
}

SDeclaration* CResourceManager::_CreateDecl(VertexElement* dcl)
{
    // Search equal code
    for (SDeclaration* D : v_declarations)
    {
        if (dcl_equal(dcl, &*D->dcl_code.begin()))
            return D;
    }

    // Create _new
    SDeclaration* D = v_declarations.emplace_back(xr_new<SDeclaration>());
    u32 dcl_size = GetDeclLength(dcl) + 1;
    CHK_DX(HW.pDevice->CreateVertexDeclaration(dcl, &D->dcl));
    D->dcl_code.assign(dcl, dcl + dcl_size);
    D->dwFlags |= xr_resource_flagged::RF_REGISTERED;

    return D;
}

//--------------------------------------------------------------------------------------------------------------
SVS* CResourceManager::_CreateVS(cpcstr shader, u32 flags /*= 0*/)
{
    string_path name;
    xr_strcpy(name, shader);
    switch (GEnv.Render->m_skinning)
    {
    case 0:
        xr_strcat(name, "_0");
        break;
    case 1:
        xr_strcat(name, "_1");
        break;
    case 2:
        xr_strcat(name, "_2");
        break;
    case 3:
        xr_strcat(name, "_3");
        break;
    case 4:
        xr_strcat(name, "_4");
        break;
    }

    return CreateShader<SVS>(name, shader, flags);
}

void CResourceManager::_DeleteVS(const SVS* vs) { DestroyShader(vs); }

//--------------------------------------------------------------------------------------------------------------
SPS* CResourceManager::_CreatePS(LPCSTR name) { return CreateShader<SPS>(name, nullptr); }
void CResourceManager::_DeletePS(const SPS* ps) { DestroyShader(ps); }

//--------------------------------------------------------------------------------------------------------------
SGeometry* CResourceManager::CreateGeom(VertexElement* decl, VertexBufferHandle vb, IndexBufferHandle ib)
{
    R_ASSERT(decl && vb);

    SDeclaration* dcl = _CreateDecl(decl);
    u32 vb_stride = GetDeclVertexSize(decl, 0);

    // ***** first pass - search already loaded shader
    for (SGeometry* v_geom : v_geoms)
    {
        SGeometry& G = *v_geom;
        if ((G.dcl == dcl) && (G.vb == vb) && (G.ib == ib) && (G.vb_stride == vb_stride))
            return v_geom;
    }

    SGeometry* Geom = v_geoms.emplace_back(xr_new<SGeometry>());
    Geom->dwFlags |= xr_resource_flagged::RF_REGISTERED;
    Geom->dcl = dcl;
    Geom->vb = vb;
    Geom->vb_stride = vb_stride;
    Geom->ib = ib;

    return Geom;
}
SGeometry* CResourceManager::CreateGeom(u32 FVF, VertexBufferHandle vb, IndexBufferHandle ib)
{
    VertexElement dcl[MAX_FVF_DECL_SIZE];
    CHK_DX(D3DXDeclaratorFromFVF(FVF, dcl));
    SGeometry* g = CreateGeom(dcl, vb, ib);
    return g;
}

#ifdef _EDITOR
//--------------------------------------------------------------------------------------------------------------
class includer : public ID3DXInclude
{
public:
    HRESULT __stdcall Open(
        D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
    {
        string_path pname;
        strconcat(sizeof(pname), pname, GEnv.Render->getShaderPath(), pFileName);
        IReader* R = FS.r_open("$game_shaders$", pname);
        if (0 == R)
        {
            // possibly in shared directory or somewhere else - open directly
            R = FS.r_open("$game_shaders$", pFileName);
            if (0 == R)
                return E_FAIL;
        }

        // duplicate and zero-terminate
        u32 size = R->length();
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

SVS* CResourceManager::_CreateVS(LPCSTR _name)
{
    string_path name;
    xr_strcpy(name, _name);
    if (0 == GEnv.Render->m_skinning)
        xr_strcat(name, "_0");
    if (1 == GEnv.Render->m_skinning)
        xr_strcat(name, "_1");
    if (2 == GEnv.Render->m_skinning)
        xr_strcat(name, "_2");
    if (3 == GEnv.Render->m_skinning)
        xr_strcat(name, "_3");
    if (4 == GEnv.Render->m_skinning)
        xr_strcat(name, "_4");
    pstr N = pstr(name);
    map_VS::iterator I = m_vs.find(N);
    if (I != m_vs.end())
        return I->second;
    else
    {
        SVS* _vs = xr_new<SVS>();
        _vs->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        m_vs.insert(std::make_pair(_vs->set_name(name), _vs));
        if (0 == xr_stricmp(_name, "null"))
        {
            _vs->vs = NULL;
            return _vs;
        }

        includer Includer;
        LPD3DXBUFFER pShaderBuf = NULL;
        LPD3DXBUFFER pErrorBuf = NULL;
        LPD3DXSHADER_CONSTANTTABLE pConstants = NULL;
        HRESULT _hr = S_OK;
        string_path cname;
        strconcat(sizeof(cname), cname, GEnv.Render->getShaderPath(), _name, ".vs");
        FS.update_path(cname, "$game_shaders$", cname);
        //		LPCSTR						target		= NULL;

        IReader* fs = FS.r_open(cname);
        R_ASSERT3(fs, "shader file doesnt exist", cname);

        // Select target
        LPCSTR c_target = "vs_2_0";
        LPCSTR c_entry = "main";
        /*if (HW.Caps.geometry.dwVersion>=CAP_VERSION(3,0))			target="vs_3_0";
		else*/ if (HW.Caps.geometry_major >= 2)
            c_target = "vs_2_0";
        else
            c_target = "vs_1_1";

        u32 needed_len = fs->length() + 1;
        pstr pfs = xr_alloc<char>(needed_len);
        strncpy_s(pfs, needed_len, (LPCSTR)fs->pointer(), fs->length());
        pfs[fs->length()] = 0;

        if (strstr(pfs, "main_vs_1_1"))
        {
            c_target = "vs_1_1";
            c_entry = "main_vs_1_1";
        }
        if (strstr(pfs, "main_vs_2_0"))
        {
            c_target = "vs_2_0";
            c_entry = "main_vs_2_0";
        }

        xr_free(pfs);

        // vertex
        R_ASSERT2(fs, cname);
        _hr = GEnv.Render->shader_compile(name, LPCSTR(fs->pointer()), fs->length(), NULL, &Includer, c_entry,
            c_target, D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR /*| D3DXSHADER_PREFER_FLOW_CONTROL*/,
            &pShaderBuf, &pErrorBuf, NULL);
        //		_hr = D3DXCompileShader		(LPCSTR(fs->pointer()),fs->length(), NULL, &Includer, "main", target,
        // D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR, &pShaderBuf, &pErrorBuf, NULL);
        FS.r_close(fs);

        if (SUCCEEDED(_hr))
        {
            if (pShaderBuf)
            {
                _hr = HW.pDevice->CreateVertexShader((DWORD*)pShaderBuf->GetBufferPointer(), &_vs->vs);
                if (SUCCEEDED(_hr))
                {
                    LPCVOID data = NULL;
                    _hr = D3DXFindShaderComment(
                        (DWORD*)pShaderBuf->GetBufferPointer(), MAKEFOURCC('C', 'T', 'A', 'B'), &data, NULL);
                    if (SUCCEEDED(_hr) && data)
                    {
                        pConstants = LPD3DXSHADER_CONSTANTTABLE(data);
                        _vs->constants.parse(pConstants, 0x2);
                    }
                    else
                    {
                        Log("! VS: ", _name);
                        Msg("! D3DXFindShaderComment hr == 0x%08x", _hr);
                        _hr = E_FAIL;
                    }
                }
                else
                {
                    Log("! VS: ", _name);
                    Msg("! CreateVertexShader hr == 0x%08x", _hr);
                }
            }
            else
            {
                Log("! VS: ", _name);
                Log("! pShaderBuf == NULL");
                _hr = E_FAIL;
            }
        }
        else
        {
            Log("! VS: ", _name);
            if (pErrorBuf)
                Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
            else
                Msg("Can't compile shader hr=0x%08x", _hr);
        }

        _RELEASE(pShaderBuf);
        _RELEASE(pErrorBuf);
        pConstants = NULL;

        CHECK_OR_EXIT(!FAILED(_hr), "Your video card doesn't meet game requirements.\n\nTry to lower game settings.");

        return _vs;
    }
}

//--------------------------------------------------------------------------------------------------------------
SPS* CResourceManager::_CreatePS(LPCSTR name)
{
    pstr N = pstr(name);
    map_PS::iterator I = m_ps.find(N);
    if (I != m_ps.end())
        return I->second;
    else
    {
        SPS* _ps = xr_new<SPS>();
        _ps->dwFlags |= xr_resource_flagged::RF_REGISTERED;
        m_ps.insert(std::make_pair(_ps->set_name(name), _ps));
        if (0 == xr_stricmp(name, "null"))
        {
            _ps->ps = NULL;
            return _ps;
        }

        // Open file
        includer Includer;
        string_path cname;
        LPCSTR shader_path = GEnv.Render->getShaderPath();
        strconcat(sizeof(cname), cname, shader_path, name, ".ps");
        FS.update_path(cname, "$game_shaders$", cname);

        // duplicate and zero-terminate
        IReader* R = FS.r_open(cname);
        R_ASSERT2(R, cname);
        u32 size = R->length();
        char* data = xr_alloc<char>(size + 1);
        CopyMemory(data, R->pointer(), size);
        data[size] = 0;
        FS.r_close(R);

        // Select target
        LPCSTR c_target = "ps_2_0";
        LPCSTR c_entry = "main";
        if (strstr(data, "main_ps_1_1"))
        {
            c_target = "ps_1_1";
            c_entry = "main_ps_1_1";
        }
        if (strstr(data, "main_ps_1_2"))
        {
            c_target = "ps_1_2";
            c_entry = "main_ps_1_2";
        }
        if (strstr(data, "main_ps_1_3"))
        {
            c_target = "ps_1_3";
            c_entry = "main_ps_1_3";
        }
        if (strstr(data, "main_ps_1_4"))
        {
            c_target = "ps_1_4";
            c_entry = "main_ps_1_4";
        }
        if (strstr(data, "main_ps_2_0"))
        {
            c_target = "ps_2_0";
            c_entry = "main_ps_2_0";
        }

        // Compile
        LPD3DXBUFFER pShaderBuf = NULL;
        LPD3DXBUFFER pErrorBuf = NULL;
        LPD3DXSHADER_CONSTANTTABLE pConstants = NULL;
        HRESULT _hr = S_OK;
        _hr = GEnv.Render->shader_compile(name, data, size, NULL, &Includer, c_entry, c_target,
            D3DXSHADER_DEBUG | D3DXSHADER_PACKMATRIX_ROWMAJOR, &pShaderBuf, &pErrorBuf, NULL);
        //_hr = D3DXCompileShader		(text,text_size, NULL, &Includer, c_entry, c_target, D3DXSHADER_DEBUG |
        // D3DXSHADER_PACKMATRIX_ROWMAJOR, &pShaderBuf, &pErrorBuf, NULL);
        xr_free(data);

        if (SUCCEEDED(_hr))
        {
            if (pShaderBuf)
            {
                _hr = HW.pDevice->CreatePixelShader((DWORD*)pShaderBuf->GetBufferPointer(), &_ps->ps);
                if (SUCCEEDED(_hr))
                {
                    LPCVOID data = NULL;
                    _hr = D3DXFindShaderComment(
                        (DWORD*)pShaderBuf->GetBufferPointer(), MAKEFOURCC('C', 'T', 'A', 'B'), &data, NULL);
                    if (SUCCEEDED(_hr) && data)
                    {
                        pConstants = LPD3DXSHADER_CONSTANTTABLE(data);
                        _ps->constants.parse(pConstants, 0x1);
                    }
                    else
                    {
                        Log("! PS: ", name);
                        Msg("! D3DXFindShaderComment hr == 0x%08x", _hr);
                        _hr = E_FAIL;
                    }
                }
                else
                {
                    Log("! PS: ", name);
                    Msg("! CreatePixelShader hr == 0x%08x", _hr);
                }
            }
            else
            {
                Log("! PS: ", name);
                Log("! pShaderBuf == NULL");
                _hr = E_FAIL;
            }
        }
        else
        {
            Log("! PS: ", name);
            if (pErrorBuf)
                Log("! error: ", (LPCSTR)pErrorBuf->GetBufferPointer());
            else
                Msg("Can't compile shader hr=0x%08x", _hr);
        }

        _RELEASE(pShaderBuf);
        _RELEASE(pErrorBuf);
        pConstants = NULL;

        CHECK_OR_EXIT(!FAILED(_hr), "Your video card doesn't meet game requirements.\n\nTry to lower game settings.");

        return _ps;
    }
}

#endif
