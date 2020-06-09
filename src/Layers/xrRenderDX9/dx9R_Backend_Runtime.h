#ifndef dx9R_Backend_Runtime_included
#define dx9R_Backend_Runtime_included
#pragma once

IC void CBackend::set_xform(u32 ID, const Fmatrix& M)
{
    stat.xforms++;
    CHK_DX(HW.pDevice->SetTransform((D3DTRANSFORMSTATETYPE)ID, (D3DMATRIX*)&M));
}

IC void CBackend::set_RT(ID3DRenderTargetView* RT, u32 ID)
{
    if (RT != pRT[ID])
    {
        PGO(Msg("PGO:setRT"));
        stat.target_rt++;
        pRT[ID] = RT;
        CHK_DX(HW.pDevice->SetRenderTarget(ID, RT));
    }
}

IC void CBackend::set_ZB(ID3DDepthStencilView* ZB)
{
    if (ZB != pZB)
    {
        PGO(Msg("PGO:setZB"));
        stat.target_zb++;
        pZB = ZB;
        CHK_DX(HW.pDevice->SetDepthStencilSurface(ZB));
    }
}

IC void CBackend::ClearRT(ID3DRenderTargetView* rt, const Fcolor& color)
{
    VERIFY(rt == pRT[0]);
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, color.get(), 1.0f, 0));
}

IC void CBackend::ClearZB(ID3DDepthStencilView* zb, float depth)
{
    VERIFY(zb == pZB);
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_ZBUFFER, 0, depth, 0));
}

IC void CBackend::ClearZB(ID3DDepthStencilView* zb, float depth, u8 stencil)
{
    VERIFY(zb == pZB);
    CHK_DX(HW.pDevice->Clear(0, nullptr, D3DCLEAR_ZBUFFER | (HW.Caps.bStencil ? D3DCLEAR_STENCIL : 0), 0, depth, stencil));
}

IC bool CBackend::ClearRTRect(ID3DRenderTargetView* rt, const Fcolor& color, size_t numRects, const Irect* rects)
{
    VERIFY(rt == pRT[0]);
    CHK_DX(HW.pDevice->Clear(numRects, reinterpret_cast<const D3DRECT*>(rects), D3DCLEAR_TARGET, color.get(), 1.0f, 0));
    return true;
}

IC bool CBackend::ClearZBRect(ID3DDepthStencilView* zb, float depth, size_t numRects, const Irect* rects)
{
    VERIFY(zb == pZB);
    CHK_DX(HW.pDevice->Clear(numRects, reinterpret_cast<const D3DRECT*>(rects), D3DCLEAR_ZBUFFER, 0, depth, 0));
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
        CHK_DX(HW.pDevice->SetVertexDeclaration(decl->dcl));
    }
}

ICF void CBackend::set_PS(ID3DPixelShader* _ps, LPCSTR _n)
{
    if (ps != _ps)
    {
        PGO(Msg("PGO:Pshader:%x", _ps));
        stat.ps++;
        ps = _ps;
        CHK_DX(HW.pDevice->SetPixelShader(ps));
#ifdef DEBUG
        ps_name = _n;
#endif
    }
}

ICF void CBackend::set_VS(ID3DVertexShader* _vs, LPCSTR _n)
{
    if (vs != _vs)
    {
        PGO(Msg("PGO:Vshader:%x", _vs));
        stat.vs++;
        vs = _vs;
        CHK_DX(HW.pDevice->SetVertexShader(vs));
#ifdef DEBUG
        vs_name = _n;
#endif
    }
}

ICF void CBackend::set_Vertices(VertexBufferHandle _vb, u32 _vb_stride)
{
    if ((vb != _vb) || (vb_stride != _vb_stride))
    {
        PGO(Msg("PGO:VB:%x,%d", _vb, _vb_stride));
#ifdef DEBUG
        stat.vb++;
#endif
        vb = _vb;
        vb_stride = _vb_stride;
        CHK_DX(HW.pDevice->SetStreamSource(0, vb, 0, vb_stride));
    }
}

ICF void CBackend::set_Indices(IndexBufferHandle _ib)
{
    if (ib != _ib)
    {
        PGO(Msg("PGO:IB:%x", _ib));
#ifdef DEBUG
        stat.ib++;
#endif
        ib = _ib;
        CHK_DX(HW.pDevice->SetIndices(ib));
    }
}

ICF void CBackend::Render(D3DPRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
    // Fix D3D ERROR
    if (PC == 0)
        return;

    stat.calls++;
    stat.verts += countV;
    stat.polys += PC;
    constants.flush();
    CHK_DX(HW.pDevice->DrawIndexedPrimitive(T, baseV, startV, countV, startI, PC));
    PGO(Msg("PGO:DIP:%dv/%df", countV, PC));
}

ICF void CBackend::Render(D3DPRIMITIVETYPE T, u32 startV, u32 PC)
{
    // Fix D3D ERROR
    if (PC == 0)
        return;

    stat.calls++;
    stat.verts += 3 * PC;
    stat.polys += PC;
    constants.flush();
    CHK_DX(HW.pDevice->DrawPrimitive(T, startV, PC));
    PGO(Msg("PGO:DIP:%dv/%df", 3 * PC, PC));
}

IC void CBackend::set_Geometry(SGeometry* _geom)
{
    set_Format(_geom->dcl._get());
    set_Vertices(_geom->vb, _geom->vb_stride);
    set_Indices(_geom->ib);
}

IC void CBackend::set_Scissor(Irect* R)
{
    if (R)
    {
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE));
        RECT* clip = (RECT*)R;
        CHK_DX(HW.pDevice->SetScissorRect(clip));
    }
    else
    {
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE));
    }
}

IC void CBackend::SetViewport(const D3D_VIEWPORT& viewport) const
{
    CHK_DX(HW.pDevice->SetViewport(&viewport));
}

IC void CBackend::set_Stencil(
    u32 _enable, u32 _func, u32 _ref, u32 _mask, u32 _writemask, u32 _fail, u32 _pass, u32 _zfail)
{
    // Simple filter
    if (stencil_enable != _enable)
    {
        stencil_enable = _enable;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILENABLE, _enable));
    }
    if (!stencil_enable)
        return;
    if (stencil_func != _func)
    {
        stencil_func = _func;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFUNC, _func));
    }
    if (stencil_ref != _ref)
    {
        stencil_ref = _ref;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, _ref));
    }
    if (stencil_mask != _mask)
    {
        stencil_mask = _mask;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILMASK, _mask));
    }
    if (stencil_writemask != _writemask)
    {
        stencil_writemask = _writemask;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILWRITEMASK, _writemask));
    }
    if (stencil_fail != _fail)
    {
        stencil_fail = _fail;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILFAIL, _fail));
    }
    if (stencil_pass != _pass)
    {
        stencil_pass = _pass;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILPASS, _pass));
    }
    if (stencil_zfail != _zfail)
    {
        stencil_zfail = _zfail;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILZFAIL, _zfail));
    }
}

IC void CBackend::set_Z(u32 _enable)
{
    if (z_enable != _enable)
    {
        z_enable = _enable;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZENABLE, _enable));
    }
}

IC void CBackend::set_ZFunc(u32 _func)
{
    if (z_func != _func)
    {
        z_func = _func;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_ZFUNC, _func));
    }
}

IC void CBackend::set_AlphaRef(u32 _value)
{
    if (alpha_ref != _value)
    {
        alpha_ref = _value;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAREF, _value));
    }
}

IC void CBackend::set_ColorWriteEnable(u32 _mask)
{
    if (colorwrite_mask != _mask)
    {
        colorwrite_mask = _mask;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, _mask));
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE1, _mask));
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE2, _mask));
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_COLORWRITEENABLE3, _mask));
    }
}

ICF void CBackend::set_CullMode(u32 _mode)
{
    if (cull_mode != _mode)
    {
        cull_mode = _mode;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_CULLMODE, _mode));
    }
}

ICF void CBackend::set_FillMode(u32 _mode)
{
    if (fill_mode != _mode)
    {
        fill_mode = _mode;
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_FILLMODE, _mode));
    }
}

ICF void CBackend::set_VS(ref_vs& _vs) { set_VS(_vs->sh, _vs->cName.c_str()); }
IC void CBackend::set_Constants(R_constant_table* C)
{
    // caching
    if (ctable == C)
        return;
    ctable = C;
    xforms.unmap();
    hemi.unmap();
    tree.unmap();
    if (nullptr == C)
        return;

    PGO(Msg("PGO:c-table"));

    // process constant-loaders
    R_constant_table::c_table::iterator it = C->table.begin();
    R_constant_table::c_table::iterator end = C->table.end();
    for (; it != end; ++it)
    {
        R_constant* Cs = &**it;
        VERIFY(Cs);
        if (Cs && Cs->handler)
            Cs->handler->setup(Cs);
    }
}

#endif //	dx9R_Backend_Runtime_included
