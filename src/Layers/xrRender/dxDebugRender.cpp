#include "stdafx.h"

#ifdef DEBUG

#include "dxDebugRender.h"
#include "dxUIShader.h"

dxDebugRender DebugRenderImpl;
dxDebugRender::dxDebugRender()
{
    m_line_indices.reserve(line_vertex_limit);
    m_line_vertices.reserve(line_index_limit);
}

void dxDebugRender::Render()
{
    if (m_line_vertices.empty())
        return;

    RCache.set_xform_world(Fidentity);
    RCache.dbg_Draw(D3DPT_LINELIST, &*m_line_vertices.begin(), m_line_vertices.size(), &*m_line_indices.begin(),
        m_line_indices.size() / 2);
    m_line_vertices.resize(0);
    m_line_indices.resize(0);
}

void dxDebugRender::try_render(u32 const& vertex_count, u32 const& index_count)
{
    VERIFY((m_line_indices.size() % 2) == 0);

    if ((m_line_vertices.size() + vertex_count) >= line_vertex_limit)
    {
        Render();
        return;
    }

    if ((m_line_indices.size() + 2 * index_count) >= line_index_limit)
    {
        Render();
        return;
    }
}
void _add_lines(xr_vector<FVF::L>& vertices, xr_vector<u16>& indices, Fvector const* pvertices, u32 const& vertex_count,
    u16 const* pairs, u32 const& pair_count, u32 const& color)
{
    VERIFY(vertices.size() < u16(-1));
    u16 vertices_size = (u16)vertices.size();

    u32 indices_size = indices.size();
    indices.resize(indices_size + 2 * pair_count);
    xr_vector<u16>::iterator I = indices.begin() + indices_size;
    xr_vector<u16>::iterator E = indices.end();
    const u16* J = pairs;
    for (; I != E; ++I, ++J)
        *I = vertices_size + *J;

    vertices.resize(vertices_size + vertex_count);
    xr_vector<FVF::L>::iterator i = vertices.begin() + vertices_size;
    xr_vector<FVF::L>::iterator e = vertices.end();
    Fvector const* j = pvertices;
    for (; i != e; ++i, ++j)
    {
        (*i).color = color;
        (*i).p = *j;
    }
}
void dxDebugRender::add_lines(
    Fvector const* vertices, u32 const& vertex_count, u16 const* pairs, u32 const& pair_count, u32 const& color)
{
    try_render(vertex_count, pair_count);
    _add_lines(m_line_vertices, m_line_indices, vertices, vertex_count, pairs, pair_count, color);
}

void dxDebugRender::NextSceneMode()
{
//	This mode is not supported in DX10
#ifndef USE_DX10
    HW.Caps.SceneMode = (HW.Caps.SceneMode + 1) % 3;
#endif //	USE_DX10
}

void dxDebugRender::ZEnable(bool bEnable)
{
    // CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE,bEnable));
    RCache.set_Z(bEnable);
}

void dxDebugRender::OnFrameEnd() { RCache.OnFrameEnd(); }
void dxDebugRender::SetShader(const debug_shader& shader) { RCache.set_Shader(((dxUIShader*)&*shader)->hShader); }
void dxDebugRender::CacheSetXformWorld(const Fmatrix& M) { RCache.set_xform_world(M); }
void dxDebugRender::CacheSetCullMode(CullMode m) { RCache.set_CullMode(CULL_NONE + m); }
void dxDebugRender::SetAmbient(u32 colour)
{
#ifndef USE_DX9
    //	TODO: DX10: Check if need this for DX10
    VERIFY(!"Not implemented for DX10");
    UNUSED(colour);
#else //	USE_DX10
    CHK_DX(HW.pDevice->SetRenderState(D3DRS_AMBIENT, colour));
#endif //	USE_DX10
}

void dxDebugRender::SetDebugShader(dbgShaderHandle shdHandle)
{
    R_ASSERT(shdHandle < dbgShaderCount);

    static const LPCSTR dbgShaderParams[][2] = {
        {"hud" DELIMITER "default", "ui" DELIMITER "ui_pop_up_active_back"}, // dbgShaderWindow
    };

    if (!m_dbgShaders[shdHandle])
        m_dbgShaders[shdHandle].create(dbgShaderParams[shdHandle][0], dbgShaderParams[shdHandle][1]);

    RCache.set_Shader(m_dbgShaders[shdHandle]);
}

void dxDebugRender::DestroyDebugShader(dbgShaderHandle shdHandle)
{
    R_ASSERT(shdHandle < dbgShaderCount);

    m_dbgShaders[shdHandle].destroy();
}

void dxDebugRender::dbg_DrawTRI(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C)
{
    RCache.dbg_DrawTRI(T, p1, p2, p3, C);
}

struct RDebugRender : public dxDebugRender, public pureRender
{
private:
    xr_vector<u16> _line_indices;
    xr_vector<FVF::L> _line_vertices;

public:
    RDebugRender()
    {
        // Device.seqRender.Add		(this);
        Device.seqRender.Add(this, REG_PRIORITY_LOW - 100);
    }

    virtual ~RDebugRender() { Device.seqRender.Remove(this); }
    void OnRender()
    {
        m_line_indices = _line_indices;
        m_line_vertices = _line_vertices;

        Render();
    }
    virtual void add_lines(
        Fvector const* vertices, u32 const& vertex_count, u16 const* pairs, u32 const& pair_count, u32 const& color)
    {
        _line_indices.resize(0);
        _line_vertices.resize(0);
        _add_lines(_line_vertices, _line_indices, vertices, vertex_count, pairs, pair_count, color);
    }
} rdebug_render_impl;
dxDebugRender* rdebug_render = &rdebug_render_impl;

#endif //	DEBUG
