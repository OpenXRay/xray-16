#ifndef QueryHelper_included
#define QueryHelper_included
#pragma once

//	Interface
#ifdef USE_OGL
IC HRESULT CreateQuery(GLuint* pQuery, D3D_QUERY type);
IC HRESULT GetData(GLuint query, void* pData, u32 DataSize);
IC HRESULT BeginQuery(GLuint query);
IC HRESULT EndQuery(GLuint query);
IC HRESULT ReleaseQuery(GLuint pQuery);
#else
IC HRESULT CreateQuery(ID3DQuery** ppQuery);
IC HRESULT GetData(ID3DQuery* pQuery, void* pData, u32 DataSize);
IC HRESULT BeginQuery(ID3DQuery* pQuery);
IC HRESULT EndQuery(ID3DQuery* pQuery);
IC HRESULT ReleaseQuery(ID3DQuery *pQuery);
#endif

//	Implementation

#if defined(USE_OGL)

IC HRESULT CreateQuery(GLuint* pQuery, D3D_QUERY type)
{
    R_ASSERT(type == D3D_QUERY_OCCLUSION);
    glGenQueries(1, pQuery);
    return S_OK;
}

IC HRESULT GetData(GLuint query, void* pData, u32 DataSize)
{
    if (DataSize == sizeof(GLint64))
        CHK_GL(glGetQueryObjecti64v(query, GL_QUERY_RESULT, (GLint64*)pData));
    else
        CHK_GL(glGetQueryObjectiv(query, GL_QUERY_RESULT, (GLint*)pData));
    return S_OK;
}

IC HRESULT BeginQuery(GLuint query)
{
    CHK_GL(glBeginQuery(GL_SAMPLES_PASSED, query));
    return S_OK;
}

IC HRESULT EndQuery(GLuint query)
{
    CHK_GL(glEndQuery(GL_SAMPLES_PASSED));
    return S_OK;
}

IC HRESULT ReleaseQuery(GLuint query)
{
    CHK_GL(glDeleteQueries(1, &query));
    return S_OK;
}

#elif defined(USE_DX11)

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
    return HW.pContext->GetData(pQuery, pData, DataSize, 0);
}

IC HRESULT BeginQuery(ID3DQuery* pQuery)
{
    HW.pContext->Begin(pQuery);
    return S_OK;
}

IC HRESULT EndQuery(ID3DQuery* pQuery)
{
    HW.pContext->End(pQuery);
    return S_OK;
}

IC HRESULT ReleaseQuery(ID3DQuery* pQuery)
{
    _RELEASE(pQuery);
    return S_OK;
}

#else // USE_DX9

IC HRESULT CreateQuery(ID3DQuery** ppQuery, D3D_QUERY type) { return HW.pDevice->CreateQuery(type, ppQuery); }
IC HRESULT GetData(ID3DQuery* pQuery, void* pData, u32 DataSize)
{
    return pQuery->GetData(pData, DataSize, D3DGETDATA_FLUSH);
}

IC HRESULT BeginQuery(ID3DQuery* pQuery) { return pQuery->Issue(D3DISSUE_BEGIN); }
IC HRESULT EndQuery(ID3DQuery* pQuery) { return pQuery->Issue(D3DISSUE_END); }

IC HRESULT ReleaseQuery(ID3DQuery* pQuery)
{
    _RELEASE(pQuery);
    return S_OK;
}

#endif

#endif // QueryHelper_included
