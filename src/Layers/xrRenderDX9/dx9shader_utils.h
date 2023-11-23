#pragma once

#if __has_include(<d3dx9.h>)
#   include <d3dx9.h>
#   define USE_D3DX
#else
#   include <d3dcompiler.h>
#endif

#ifdef USE_D3DX
// When building with Unity build, d3dx9.h and d3dcompiler.h both get included in files
// So we cannot just declare:
// using D3D_SHADER_MACRO = D3DXMACRO;
// because it would conflict.
// I could exclude some files from Unity build, but why?
// Let's just make it robustly compileable in all cases.
using IShaderBlob = ID3DXBuffer;
using IShaderIncluder = ID3DXInclude;
using SHADER_MACRO = D3DXMACRO;
using INCLUDE_TYPE = D3DXINCLUDE_TYPE;
#else
using IShaderBlob = ID3DBlob;
using IShaderIncluder = ID3DInclude;
using SHADER_MACRO = D3D_SHADER_MACRO;
using INCLUDE_TYPE = D3D_INCLUDE_TYPE;

typedef struct _D3DXSHADER_CONSTANTTABLE
{
    DWORD Size;             // sizeof(D3DXSHADER_CONSTANTTABLE)
    DWORD Creator;          // LPCSTR offset
    DWORD Version;          // shader version
    DWORD Constants;        // number of constants
    DWORD ConstantInfo;     // D3DXSHADER_CONSTANTINFO[Constants] offset
    DWORD Flags;            // flags shader was compiled with
    DWORD Target;           // LPCSTR offset

} D3DXSHADER_CONSTANTTABLE, * LPD3DXSHADER_CONSTANTTABLE;


typedef struct _D3DXSHADER_CONSTANTINFO
{
    DWORD Name;             // LPCSTR offset
    WORD  RegisterSet;      // D3DXREGISTER_SET
    WORD  RegisterIndex;    // register number
    WORD  RegisterCount;    // number of registers
    WORD  Reserved;         // reserved
    DWORD TypeInfo;         // D3DXSHADER_TYPEINFO offset
    DWORD DefaultValue;     // offset of default value

} D3DXSHADER_CONSTANTINFO, * LPD3DXSHADER_CONSTANTINFO;


typedef struct _D3DXSHADER_TYPEINFO
{
    WORD  Class;            // D3DXPARAMETER_CLASS
    WORD  Type;             // D3DXPARAMETER_TYPE
    WORD  Rows;             // number of rows (matrices)
    WORD  Columns;          // number of columns (vectors and matrices)
    WORD  Elements;         // array dimension
    WORD  StructMembers;    // number of struct members
    DWORD StructMemberInfo; // D3DXSHADER_STRUCTMEMBERINFO[Members] offset

} D3DXSHADER_TYPEINFO, * LPD3DXSHADER_TYPEINFO;


typedef struct _D3DXSHADER_STRUCTMEMBERINFO
{
    DWORD Name;             // LPCSTR offset
    DWORD TypeInfo;         // D3DXSHADER_TYPEINFO offset

} D3DXSHADER_STRUCTMEMBERINFO, * LPD3DXSHADER_STRUCTMEMBERINFO;

typedef enum _D3DXREGISTER_SET
{
    D3DXRS_BOOL,
    D3DXRS_INT4,
    D3DXRS_FLOAT4,
    D3DXRS_SAMPLER,
    D3DXRS_FORCE_DWORD = 0x7fffffff
} D3DXREGISTER_SET, * LPD3DXREGISTER_SET;

typedef enum D3DXPARAMETER_CLASS
{
    D3DXPC_SCALAR,
    D3DXPC_VECTOR,
    D3DXPC_MATRIX_ROWS,
    D3DXPC_MATRIX_COLUMNS,
    D3DXPC_OBJECT,
    D3DXPC_STRUCT,
    D3DXPC_FORCE_DWORD = 0x7fffffff
} D3DXPARAMETER_CLASS, * LPD3DXPARAMETER_CLASS;

typedef enum D3DXPARAMETER_TYPE
{
    D3DXPT_VOID,
    D3DXPT_BOOL,
    D3DXPT_INT,
    D3DXPT_FLOAT,
    D3DXPT_STRING,
    D3DXPT_TEXTURE,
    D3DXPT_TEXTURE1D,
    D3DXPT_TEXTURE2D,
    D3DXPT_TEXTURE3D,
    D3DXPT_TEXTURECUBE,
    D3DXPT_SAMPLER,
    D3DXPT_SAMPLER1D,
    D3DXPT_SAMPLER2D,
    D3DXPT_SAMPLER3D,
    D3DXPT_SAMPLERCUBE,
    D3DXPT_PIXELSHADER,
    D3DXPT_VERTEXSHADER,
    D3DXPT_PIXELFRAGMENT,
    D3DXPT_VERTEXFRAGMENT,
    D3DXPT_UNSUPPORTED,
    D3DXPT_FORCE_DWORD = 0x7fffffff
} D3DXPARAMETER_TYPE, * LPD3DXPARAMETER_TYPE;

// Errors
#define _FACDD  0x876
#define MAKE_DDHRESULT(code) MAKE_HRESULT(1, _FACDD, code)

enum _D3DXERR
{
    D3DXERR_CANNOTMODIFYINDEXBUFFER = MAKE_DDHRESULT(2900),
    D3DXERR_INVALIDMESH = MAKE_DDHRESULT(2901),
    D3DXERR_CANNOTATTRSORT = MAKE_DDHRESULT(2902),
    D3DXERR_SKINNINGNOTSUPPORTED = MAKE_DDHRESULT(2903),
    D3DXERR_TOOMANYINFLUENCES = MAKE_DDHRESULT(2904),
    D3DXERR_INVALIDDATA = MAKE_DDHRESULT(2905),
    D3DXERR_LOADEDMESHASNODATA = MAKE_DDHRESULT(2906),
    D3DXERR_DUPLICATENAMEDFRAGMENT = MAKE_DDHRESULT(2907),
    D3DXERR_CANNOTREMOVELASTITEM = MAKE_DDHRESULT(2908),
};
#endif

class ShaderIncluder final : public IShaderIncluder
{
public:
    HRESULT __stdcall Open(
        INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) noexcept override
    {
        string_path pname;
        strconcat(pname, RImplementation.getShaderPath(), pFileName);
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

    HRESULT __stdcall Close(LPCVOID pData) noexcept override
    {
        auto mutableData = const_cast<LPVOID>(pData);
        xr_free(mutableData);
        return D3D_OK;
    }
};

inline HRESULT WINAPI FindShaderComment(const DWORD* byte_code, DWORD fourcc, const void** data, UINT* size)
{
#ifdef USE_D3DX
    return D3DXFindShaderComment(byte_code, fourcc, data, size);
#else
    const DWORD* ptr = byte_code;

    if (data)
        *data = nullptr;
    if (size)
        *size = 0;

    if (!byte_code)
        return D3DERR_INVALIDCALL;

    const DWORD version = *ptr >> 16;
    if (version != 0x4658         /* FX */
        && version != 0x5458  /* TX */
        && version != 0x7ffe
        && version != 0x7fff
        && version != 0xfffe  /* VS */
        && version != 0xffff) /* PS */
    {
        return D3DXERR_INVALIDDATA;
    }

    while (*++ptr != D3DSIO_END)
    {
        /* Check if it is a comment */
        if ((*ptr & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
        {
            const DWORD comment_size = (*ptr & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;

            /* Check if this is the comment we are looking for */
            if (*(ptr + 1) == fourcc)
            {
                const UINT ctab_size = (comment_size - 1) * sizeof(DWORD);
                const void* ctab_data = ptr + 2;
                if (size)
                    *size = ctab_size;
                if (data)
                    *data = ctab_data;
                return D3D_OK;
            }
            ptr += comment_size;
        }
    }

    return S_FALSE;
#endif
}

inline HRESULT WINAPI DisassembleShader(CONST DWORD* pShader, SIZE_T SrcDataSize,
    BOOL EnableColorCode, LPCSTR pComments, IShaderBlob** ppDisassembly)
{
#ifdef USE_D3DX
    std::ignore = SrcDataSize;
    return D3DXDisassembleShader(pShader, EnableColorCode, pComments, ppDisassembly);
#else
    return D3DDisassemble(pShader, SrcDataSize, EnableColorCode ? D3D_DISASM_ENABLE_COLOR_CODE : 0, pComments, ppDisassembly);
#endif
}

inline HRESULT WINAPI CompileShader(LPCVOID pSrcData, SIZE_T SrcDataSize,
    CONST SHADER_MACRO* pDefines, IShaderIncluder* pInclude,
    LPCSTR pEntrypoint, LPCSTR pTarget, DWORD Flags,
    IShaderBlob** ppCode, IShaderBlob** ppErrorMsgs)
{
#ifdef USE_D3DX
    return D3DXCompileShader((LPCSTR)pSrcData, SrcDataSize, pDefines, pInclude,
        pEntrypoint, pTarget, Flags | D3DXSHADER_USE_LEGACY_D3DX9_31_DLL,
        ppCode, ppErrorMsgs, nullptr);
#else
    return D3DCompile(pSrcData, SrcDataSize, nullptr, pDefines, pInclude,
        pEntrypoint, pTarget, Flags | D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY, 0,
        ppCode, ppErrorMsgs);
#endif
}
