#pragma once

namespace xray::render::RENDER_NAMESPACE
{
//	Interface
#if defined(USE_DX11)
IC HRESULT CreateQuery(ID3DQuery** ppQuery);
IC HRESULT GetData(ID3DQuery* pQuery, void* pData, u32 DataSize);
IC HRESULT BeginQuery(ID3DQuery* pQuery);
IC HRESULT EndQuery(ID3DQuery* pQuery);
IC HRESULT ReleaseQuery(ID3DQuery *pQuery);
#elif defined(USE_OGL)
IC HRESULT CreateQuery(GLuint* pQuery, D3D_QUERY type);
IC HRESULT GetData(GLuint query, void* pData, u32 DataSize);
IC HRESULT BeginQuery(GLuint query);
IC HRESULT EndQuery(GLuint query);
IC HRESULT ReleaseQuery(GLuint pQuery);
#else
#   error No graphics API selected or enabled!
#endif

//	Implementation

#if defined(USE_DX11)

IC HRESULT CreateQuery(ID3DQuery** ppQuery, D3D_QUERY type)
{
    D3D_QUERY_DESC desc;
    desc.MiscFlags = 0;
    desc.Query = type;
    return HW.pDevice->CreateQuery(&desc, ppQuery);
}

IC HRESULT GetData(ID3DQuery* pQuery, void* pData, u32 DataSize)
{
    //	Use D3Dxx_ASYNC_GETDATA_DONOTFLUSH for prevent flushing
    return HW.get_context(CHW::IMM_CTX_ID)->GetData(pQuery, pData, DataSize, 0); // we can fetch data on imm only
}

IC HRESULT BeginQuery(ID3DQuery* pQuery)
{
    HW.get_context(CHW::IMM_CTX_ID)->Begin(pQuery);
    return S_OK;
}

IC HRESULT EndQuery(ID3DQuery* pQuery)
{
    HW.get_context(CHW::IMM_CTX_ID)->End(pQuery);
    return S_OK;
}

IC HRESULT ReleaseQuery(ID3DQuery* pQuery)
{
    _RELEASE(pQuery);
    return S_OK;
}

#elif defined(USE_OGL)

IC HRESULT CreateQuery(GLuint* pQuery, D3D_QUERY type)
{
    R_ASSERT(type == D3D_QUERY_OCCLUSION);
    glGenQueries(1, pQuery);
    return *pQuery ? S_OK : E_FAIL;
}

IC HRESULT GetData(GLuint query, void* pData, u32 DataSize)
{
    if (!query)
        return E_FAIL;
    if (GLAD_GL_ES_VERSION_3_0)
    {
        VERIFY(DataSize == sizeof(GLuint));
        GLuint available = GL_FALSE;
        CHK_GL(glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available));
        if (!available)
            return S_FALSE;
        CHK_GL(glGetQueryObjectuiv(query, GL_QUERY_RESULT, static_cast<GLuint*>(pData)));
    }
    else if (DataSize == sizeof(GLint64))
        CHK_GL(glGetQueryObjecti64v(query, GL_QUERY_RESULT, (GLint64*)pData));
    else
        CHK_GL(glGetQueryObjectiv(query, GL_QUERY_RESULT, (GLint*)pData));
    return S_OK;
}

IC HRESULT BeginQuery(GLuint query)
{
    const GLenum target = GLAD_GL_ES_VERSION_3_0 ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED;
    GLint active = 0;
    glGetQueryiv(target, GL_CURRENT_QUERY, &active);
    if (active != 0)
        return E_FAIL;
    CHK_GL(glBeginQuery(target, query));
    return S_OK;
}

IC HRESULT EndQuery(GLuint query)
{
    const GLenum target = GLAD_GL_ES_VERSION_3_0 ? GL_ANY_SAMPLES_PASSED : GL_SAMPLES_PASSED;
    GLint active = 0;
    glGetQueryiv(target, GL_CURRENT_QUERY, &active);
    if (active != static_cast<GLint>(query))
        return E_FAIL;
    CHK_GL(glEndQuery(target));
    return S_OK;
}

IC HRESULT ReleaseQuery(GLuint query)
{
    CHK_GL(glDeleteQueries(1, &query));
    return S_OK;
}

#else
#   error No graphics API selected or enabled!
#endif
} // namespace xray::render::RENDER_NAMESPACE
