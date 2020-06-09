#ifndef	glR_Backend_Runtime_included
#define	glR_Backend_Runtime_included
#pragma once

#include "glStateUtils.h"

IC void CBackend::set_xform(u32 ID, const Fmatrix& M)
{
    stat.xforms++;
    //	TODO: OGL: Implement CBackend::set_xform
    //VERIFY(!"Implement CBackend::set_xform");
}

IC GLuint CBackend::get_FB()
{
    return pFB;
}

IC void CBackend::set_FB(GLuint FB)
{
    if (FB != pFB)
    {
        PGO(Msg("PGO:set_FB"));
        pFB = FB;
        CHK_GL(glBindFramebuffer(GL_FRAMEBUFFER, pFB));
    }
}

IC void CBackend::set_RT(GLuint RT, u32 ID)
{
    if (RT != pRT[ID])
    {
        PGO(Msg("PGO:setRT"));
        stat.target_rt++;
        pRT[ID] = RT;
        // TODO: OGL: Implement support for multi-sampled render targets
        CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + ID, GL_TEXTURE_2D, RT, 0));
    }
}

IC void CBackend::set_ZB(GLuint ZB)
{
    if (ZB != pZB)
    {
        PGO(Msg("PGO:setZB"));
        stat.target_zb++;
        pZB = ZB;
        // TODO: OGL: Implement support for multi-sampled render targets
        CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, ZB, 0));
    }
}

IC void CBackend::ClearRT(GLuint rt, const Fcolor& color)
{
    // TODO: OGL: Implement support for multi-sampled render targets
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt, 0));

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(color.r, color.g, color.b, color.a);

    CHK_GL(glClear(GL_COLOR_BUFFER_BIT));
}

IC void CBackend::ClearZB(GLuint zb, float depth)
{
    // TODO: OGL: Implement support for multi-sampled render targets
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, zb, 0));

    glDepthMask(GL_TRUE);
    glClearDepthf(depth);

    CHK_GL(glClear(GL_DEPTH_BUFFER_BIT));
}

IC void CBackend::ClearZB(GLuint zb, float depth, u8 stencil)
{
    // TODO: OGL: Implement support for multi-sampled render targets
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, zb, 0));

    glDepthMask(GL_TRUE);
    glClearDepthf(depth);

    glStencilMask(~0);
    glClearStencil(stencil);

    CHK_GL(glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}

IC bool CBackend::ClearRTRect(GLuint rt, const Fcolor& color, size_t numRects, const Irect* rects)
{
    // TODO: OGL: Implement support for multi-sampled render targets
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt, 0));

    CHK_GL(glEnable(GL_SCISSOR_TEST));

    for (size_t i = 0; i < numRects; ++i, ++rects)
    {
        // The window space is inverted compared to DX
        // so we have to invert our vertical coordinates
        const u32 bottom = Device.dwHeight - rects->bottom;

        // The origin of the scissor box is lower-left
        CHK_GL(glScissor(rects->left, bottom, rects->width(), rects->height()));

        // Clear the color buffer without affecting the global state
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(color.r, color.g, color.b, color.a);

        CHK_GL(glClear(GL_COLOR_BUFFER_BIT));
    }

    CHK_GL(glDisable(GL_SCISSOR_TEST));

    return true;
}

IC bool CBackend::ClearZBRect(GLuint zb, float depth, size_t numRects, const Irect* rects)
{
    // TODO: OGL: Implement support for multi-sampled render targets
    CHK_GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, zb, 0));

    CHK_GL(glEnable(GL_SCISSOR_TEST));

    for (size_t i = 0; i < numRects; ++i, ++rects)
    {
        // The window space is inverted compared to DX
        // so we have to invert our vertical coordinates
        const u32 bottom = Device.dwHeight - rects->bottom;

        // The origin of the scissor box is lower-left
        CHK_GL(glScissor(rects->left, bottom, rects->width(), rects->height()));

        glDepthMask(GL_TRUE);
        glClearDepthf(depth);

        CHK_GL(glClear(GL_DEPTH_BUFFER_BIT));
    }

    CHK_GL(glDisable(GL_SCISSOR_TEST));

    return true;
}

ICF void CBackend::set_Format(SDeclaration* _decl)
{
    if (decl != _decl)
    {
        PGO(Msg("PGO:v_format:%x", _decl));
#ifdef DEBUG
		stat.decl++;
#endif
        decl = _decl;
        CHK_GL(glBindVertexArray(_decl->dcl));

        // Clear cached index buffer
        ib = 0;
    }
}

ICF void CBackend::set_PS(GLuint _ps, LPCSTR _n)
{
    if (ps != _ps)
    {
#ifdef RBackend_PGO
		string_path name;
#endif
        PGO(glGetObjectLabel(GL_PROGRAM, _ps, sizeof(name), nullptr, name));
        PGO(Msg("PGO:Pshader:%d,%s", _ps, _n ? _n : name));
        stat.ps++;
        ps = _ps;
        CHK_GL(glUseProgramStages(HW.pPP, GL_FRAGMENT_SHADER_BIT, ps));
#ifdef DEBUG
		ps_name = _n;
#endif
    }
}

ICF void CBackend::set_GS(GLuint _gs, LPCSTR _n)
{
    if (gs != _gs)
    {
#ifdef RBackend_PGO
		string_path name;
#endif
        PGO(glGetObjectLabel(GL_PROGRAM, _gs, sizeof(name), nullptr, name));
        PGO(Msg("PGO:Gshader:%d,%s", _gs, _n ? _n : name));
        //	TODO: OGL: Get statistics for G Shader change
        //stat.gs			++;
        gs = _gs;
        CHK_GL(glUseProgramStages(HW.pPP, GL_GEOMETRY_SHADER_BIT, gs));
#ifdef DEBUG
		gs_name = _n;
#endif
    }
}

ICF void CBackend::set_VS(GLuint _vs, LPCSTR _n)
{
    if (vs != _vs)
    {
#ifdef RBackend_PGO
		string_path name;
#endif
        PGO(glGetObjectLabel(GL_PROGRAM, _vs, sizeof(name), nullptr, name));
        PGO(Msg("PGO:Vshader:%d,%s", _vs, _n ? _n : name));
        stat.vs++;
        vs = _vs;
        CHK_GL(glUseProgramStages(HW.pPP, GL_VERTEX_SHADER_BIT, vs));
#ifdef DEBUG
		vs_name = _n;
#endif
    }
}

ICF void CBackend::set_Vertices(GLuint _vb, u32 _vb_stride)
{
    if (vb != _vb || vb_stride != _vb_stride)
    {
        PGO(Msg("PGO:VB:%x,%d", _vb, _vb_stride));
#ifdef DEBUG
		stat.vb++;
#endif
        vb = _vb;
        vb_stride = _vb_stride;
        CHK_GL(glBindVertexBuffer(0, vb, 0, vb_stride));
    }
}

ICF void CBackend::set_Indices(GLuint _ib)
{
    if (ib != _ib)
    {
        PGO(Msg("PGO:IB:%x", _ib));
#ifdef DEBUG
		stat.ib++;
#endif
        ib = _ib;
        CHK_GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib));
    }
}

IC GLenum TranslateTopology(D3DPRIMITIVETYPE T)
{
    static GLenum translateTable[] =
    {
        GL_NONE, //	None
        GL_POINTS, //	D3DPT_POINTLIST = 1,
        GL_LINES, //	D3DPT_LINELIST = 2,
        GL_LINE_STRIP, //	D3DPT_LINESTRIP = 3,
        GL_TRIANGLES, //	D3DPT_TRIANGLELIST = 4,
        GL_TRIANGLE_STRIP, //	D3DPT_TRIANGLESTRIP = 5,
        GL_TRIANGLE_FAN, //	D3DPT_TRIANGLEFAN = 6,
    };

    VERIFY(T<sizeof(translateTable) / sizeof(translateTable[0]));
    VERIFY(T >= 0);

    GLenum result = translateTable[T];

    VERIFY(result != NULL);

    return result;
}

IC u32 GetIndexCount(D3DPRIMITIVETYPE T, u32 iPrimitiveCount)
{
    switch (T)
    {
    case D3DPT_POINTLIST:
        return iPrimitiveCount;
    case D3DPT_LINELIST:
        return iPrimitiveCount * 2;
    case D3DPT_LINESTRIP:
        return iPrimitiveCount + 1;
    case D3DPT_TRIANGLELIST:
        return iPrimitiveCount * 3;
    case D3DPT_TRIANGLESTRIP:
        return iPrimitiveCount + 2;
    default: NODEFAULT;
#ifdef DEBUG
		return 0;
#endif // #ifdef DEBUG
    }
}

ICF void CBackend::Render(D3DPRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
    GLenum Topology = TranslateTopology(T);
    u32 iIndexCount = GetIndexCount(T, PC);

    stat.calls++;
    stat.verts += countV;
    stat.polys += PC;
    constants.flush();
    CHK_GL(glDrawElementsBaseVertex(Topology, iIndexCount, GL_UNSIGNED_SHORT, (void*)(startI * sizeof(GLushort)), baseV));
    PGO(Msg("PGO:DIP:%dv/%df", countV, PC));
}

ICF void CBackend::Render(D3DPRIMITIVETYPE T, u32 startV, u32 PC)
{
    GLenum Topology = TranslateTopology(T);
    u32 iIndexCount = GetIndexCount(T, PC);

    stat.calls++;
    stat.verts += iIndexCount;
    stat.polys += PC;
    constants.flush();
    CHK_GL(glDrawArrays(Topology, startV, iIndexCount));
    PGO(Msg("PGO:DIP:%dv/%df", iIndexCount, PC));
}

IC void CBackend::set_Geometry(SGeometry* _geom)
{
    set_Format(&*_geom->dcl);

    set_Vertices(_geom->vb, _geom->vb_stride);
    set_Indices(_geom->ib);
}

IC void CBackend::set_Scissor(Irect* R)
{
    if (R)
    {
        CHK_GL(glEnable(GL_SCISSOR_TEST));

        // The window space is inverted compared to DX,
        // so we have to invert our vertical coordinates
        u32 bottom = Device.dwHeight - R->bottom;

        // The origin of the scissor box is lower-left
        CHK_GL(glScissor(R->left, bottom, R->width(), R->height()));
    }
    else
    {
        CHK_GL(glDisable(GL_SCISSOR_TEST));
    }
}

IC void CBackend::SetViewport(const D3D_VIEWPORT& viewport) const
{
    glViewport(viewport.TopLeftX, viewport.TopLeftY, viewport.Width, viewport.Height);
    glDepthRangef(viewport.MinDepth, viewport.MaxDepth);
}

IC void CBackend::set_Stencil(u32 _enable, u32 _func, u32 _ref, u32 _mask, u32 _writemask, u32 _fail, u32 _pass,
                              u32 _zfail)
{
    if (_enable)
    {
        glEnable(GL_STENCIL_TEST);
        CHK_GL(glStencilFunc(glStateUtils::ConvertCmpFunction(_func), _ref, _mask));
        CHK_GL(glStencilMask(_writemask));
        CHK_GL(glStencilOp(glStateUtils::ConvertStencilOp(_fail),
            glStateUtils::ConvertStencilOp(_zfail),
            glStateUtils::ConvertStencilOp(_pass)));
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }
}

IC void CBackend::set_Z(u32 _enable)
{
    if (z_enable != _enable)
    {
        z_enable = _enable;
        if (_enable)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }
}

IC void CBackend::set_ZFunc(u32 _func)
{
    if (z_func != _func)
    {
        z_func = _func;
        CHK_GL(glDepthFunc(glStateUtils::ConvertCmpFunction(_func)));
    }
}

IC void CBackend::set_AlphaRef(u32 _value)
{
    VERIFY(!"Not implemented.");
}

IC void CBackend::set_ColorWriteEnable(u32 _mask)
{
    if (colorwrite_mask != _mask)
    {
        colorwrite_mask = _mask;
        CHK_GL(glColorMask(
            (_mask & D3DCOLORWRITEENABLE_RED) ? GL_TRUE : GL_FALSE,
            (_mask & D3DCOLORWRITEENABLE_GREEN) ? GL_TRUE : GL_FALSE,
            (_mask & D3DCOLORWRITEENABLE_BLUE) ? GL_TRUE : GL_FALSE,
            (_mask & D3DCOLORWRITEENABLE_ALPHA) ? GL_TRUE : GL_FALSE));
    }
}

ICF void CBackend::set_CullMode(u32 _mode)
{
    if (cull_mode != _mode)
    {
        cull_mode = _mode;
        if (_mode == D3DCULL_NONE)
        {
            glDisable(GL_CULL_FACE);
        }
        else
        {
            glEnable(GL_CULL_FACE);
            CHK_GL(glCullFace(glStateUtils::ConvertCullMode(_mode)));
        }
    }
}

ICF void CBackend::set_FillMode(u32 _mode)
{
    if (fill_mode != _mode)
    {
        fill_mode = _mode;
        glPolygonMode(GL_FRONT_AND_BACK, glStateUtils::ConvertFillMode(_mode));
    }
}

ICF void CBackend::set_VS(ref_vs& _vs)
{
    set_VS(_vs->sh, _vs->cName.c_str());
}

IC void CBackend::set_Constants(R_constant_table* C)
{
    // caching
    if (ctable == C) return;
    ctable = C;
    xforms.unmap();
    hemi.unmap();
    tree.unmap();
    if (nullptr == C) return;

    PGO(Msg("PGO:c-table"));

    // process constant-loaders
    R_constant_table::c_table::iterator it = C->table.begin();
    R_constant_table::c_table::iterator end = C->table.end();
    for (; it != end; ++it)
    {
        R_constant* Cs = &**it;
        if (Cs->handler) Cs->handler->setup(Cs);
    }
}

#endif	//	glR_Backend_Runtime_included
