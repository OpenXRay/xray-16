#pragma once

#include "StateManager/dx11ShaderResourceStateCache.h"

namespace xray::render::fg
{
IC void CBackend::set_xform(u32 ID, const Fmatrix& M)
{
    stat.xforms++;
    //  TODO: DX11: Implement CBackend::set_xform
    // VERIFY(!"Implement CBackend::set_xform");
}

IC void CBackend::set_RT(ID3DRenderTargetView* RT, u32 ID)
{
    if (RT != pRT[ID])
    {
        PGO(Msg("PGO:setRT"));
        stat.target_rt++;
        pRT[ID] = RT;
        //  Mark RT array dirty
        // HW.pDevice->OMSetRenderTargets(sizeof(pRT)/sizeof(pRT[0]), pRT, 0);
        // HW.pDevice->OMSetRenderTargets(sizeof(pRT)/sizeof(pRT[0]), pRT, pZB);
        //  Reset all RT's here to allow RT to be bounded as input
        if (!m_bChangedRTorZB)
            HW.get_context(context_id)->OMSetRenderTargets(0, 0, 0);

        m_bChangedRTorZB = true;
    }
}

IC void CBackend::set_ZB(ID3DDepthStencilView* ZB)
{
    if (ZB != pZB)
    {
        PGO(Msg("PGO:setZB"));
        stat.target_zb++;
        pZB = ZB;
        // HW.pDevice->OMSetRenderTargets(0, 0, pZB);
        // HW.pDevice->OMSetRenderTargets(sizeof(pRT)/sizeof(pRT[0]), pRT, pZB);
        //  Reset all RT's here to allow RT to be bounded as input
        if (!m_bChangedRTorZB)
            HW.get_context(context_id)->OMSetRenderTargets(0, 0, 0);
        m_bChangedRTorZB = true;
    }
}

IC void CBackend::ClearRT(ID3DRenderTargetView* rt, const Fcolor& color)
{
    HW.get_context(context_id)->ClearRenderTargetView(rt, reinterpret_cast<const FLOAT*>(&color));
}

IC void CBackend::ClearZB(ID3DDepthStencilView* zb, float depth)
{
    HW.get_context(context_id)->ClearDepthStencilView(zb, D3D_CLEAR_DEPTH, depth, 0);
}

IC void CBackend::ClearZB(ID3DDepthStencilView* zb, float depth, u8 stencil)
{
    HW.get_context(context_id)->ClearDepthStencilView(zb, D3D_CLEAR_DEPTH | D3D_CLEAR_STENCIL, depth, stencil);
}

IC bool CBackend::ClearRTRect(ID3DRenderTargetView* rt, const Fcolor& color, size_t numRects, const Irect* rects)
{
#ifdef USE_DX11
    if (HW.pContext1)
    {
        HW.pContext1->ClearView(rt, reinterpret_cast<const FLOAT*>(&color),
            reinterpret_cast<const D3D_RECT*>(rects), numRects);
        return true;
    }
#else
    UNUSED(numRects);
    UNUSED(rects);
#endif

    return false;
}

IC bool CBackend::ClearZBRect(ID3DDepthStencilView* zb, float depth, size_t numRects, const Irect* rects)
{
#ifdef USE_DX11
    if (HW.pContext1)
    {
        Fcolor color = { depth, depth, depth, depth };
        HW.pContext1->ClearView(zb, reinterpret_cast<FLOAT*>(&color),
            reinterpret_cast<const D3D_RECT*>(rects), numRects);
        return true;
    }
#else
    UNUSED(numRects);
    UNUSED(rects);
#endif

    return false;
}

ICF void CBackend::set_Format(SDeclaration* _decl)
{
    if (decl != _decl)
    {
        PGO(Msg("PGO:v_format:%x", _decl));
        stat.decl++;
        decl = _decl;
    }
}

ICF void CBackend::set_PS(ID3DPixelShader* _ps, LPCSTR _n)
{
    if (ps != _ps)
    {
        PGO(Msg("PGO:Pshader:%x", _ps));
        stat.ps++;
        ps = _ps;
#ifdef USE_DX11
        HW.get_context(context_id)->PSSetShader(ps, 0, 0);
#else
        HW.pContext->PSSetShader(ps);
#endif

#ifdef DEBUG
        ps_name = _n;
#endif
    }
}

ICF void CBackend::set_GS(ID3DGeometryShader* _gs, LPCSTR _n)
{
    if (gs != _gs)
    {
        PGO(Msg("PGO:Gshader:%x", _ps));
        stat.gs++;
        gs = _gs;
#ifdef USE_DX11
        HW.get_context(context_id)->GSSetShader(gs, 0, 0);
#else
        HW.pContext->GSSetShader(gs);
#endif

#ifdef DEBUG
        gs_name = _n;
#endif
    }
}

#ifdef USE_DX11
ICF void CBackend::set_HS(ID3D11HullShader* _hs, LPCSTR _n)
{
    if (hs != _hs)
    {
        PGO(Msg("PGO:Hshader:%x", _ps));
        stat.hs++;
        hs = _hs;
        HW.get_context(context_id)->HSSetShader(hs, 0, 0);

#ifdef DEBUG
        hs_name = _n;
#endif
    }
}

ICF void CBackend::set_DS(ID3D11DomainShader* _ds, LPCSTR _n)
{
    if (ds != _ds)
    {
        PGO(Msg("PGO:Dshader:%x", _ps));
        stat.ds++;
        ds = _ds;
        HW.get_context(context_id)->DSSetShader(ds, 0, 0);

#ifdef DEBUG
        ds_name = _n;
#endif
    }
}

ICF void CBackend::set_CS(ID3D11ComputeShader* _cs, LPCSTR _n)
{
    if (cs != _cs)
    {
        PGO(Msg("PGO:Cshader:%x", _ps));
        stat.cs++;
        cs = _cs;
        HW.get_context(context_id)->CSSetShader(cs, 0, 0);

#ifdef DEBUG
        cs_name = _n;
#endif
    }
}

ICF bool CBackend::is_TessEnabled() { return HW.FeatureLevel >= D3D_FEATURE_LEVEL_11_0 && (ds != 0 || hs != 0); }
#endif

ICF void CBackend::set_VS(ID3DVertexShader* _vs, LPCSTR _n)
{
    if (vs != _vs)
    {
        PGO(Msg("PGO:Vshader:%x", _vs));
        stat.vs++;
        vs = _vs;
#ifdef USE_DX11
        HW.get_context(context_id)->VSSetShader(vs, 0, 0);
#else
        HW.pContext->VSSetShader(vs);
#endif

#ifdef DEBUG
        vs_name = _n;
#endif
    }
}

ICF void CBackend::set_Vertices(VertexBufferHandle _vb, u32 _vb_stride)
{
    if ((vb != _vb) || (vb_stride != _vb_stride))
    {
        PGO(Msg("PGO:VB:%p,%d", _vb.Get(), _vb_stride));
        stat.vb++;
        vb = _vb;
        vb_stride = _vb_stride;

        // For D3D12/NVRHI: Vertex buffer binding is handled by FrameGraph pipeline
        // Legacy D3D11 API calls are no longer used
        // The actual binding happens in NVRHI draw commands
    }
}

ICF void CBackend::set_Indices(IndexBufferHandle _ib)
{
    if (ib != _ib)
    {
        PGO(Msg("PGO:IB:%p", _ib.Get()));
        stat.ib++;
        ib = _ib;

        // For D3D12/NVRHI: Index buffer binding is handled by FrameGraph pipeline
        // Legacy D3D11 API calls are no longer used
        // The actual binding happens in NVRHI draw commands
    }
}

IC D3D_PRIMITIVE_TOPOLOGY TranslateTopology(D3D_PRIMITIVETYPE T)
{
    static D3D_PRIMITIVE_TOPOLOGY translateTable[] = {
        D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, // None
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST, // D3D_PT_POINTLIST = 1,
        D3D_PRIMITIVE_TOPOLOGY_LINELIST, // D3D_PT_LINELIST = 2,
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP, // D3D_PT_LINESTRIP = 3,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, // D3D_PT_TRIANGLELIST = 4,
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, // D3D_PT_TRIANGLESTRIP = 5,
        D3D_PRIMITIVE_TOPOLOGY_UNDEFINED, // D3D_PT_TRIANGLEFAN = 6,
    };

    VERIFY(T < sizeof(translateTable) / sizeof(translateTable[0]));
    VERIFY(T >= 0);

    D3D_PRIMITIVE_TOPOLOGY result = translateTable[T];

    VERIFY(result != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

    return result;
}

IC u32 GetIndexCount(D3D_PRIMITIVETYPE T, u32 iPrimitiveCount)
{
    switch (T)
    {
    case D3D_PT_POINTLIST: return iPrimitiveCount;
    case D3D_PT_LINELIST: return iPrimitiveCount * 2;
    case D3D_PT_LINESTRIP: return iPrimitiveCount + 1;
    case D3D_PT_TRIANGLELIST: return iPrimitiveCount * 3;
    case D3D_PT_TRIANGLESTRIP: return iPrimitiveCount + 2;
    default: NODEFAULT;
#ifdef DEBUG
        return 0;
#endif // #ifdef DEBUG
    }
}

IC void CBackend::ApplyPrimitieTopology(D3D_PRIMITIVE_TOPOLOGY Topology)
{
    if (m_PrimitiveTopology != Topology)
    {
        m_PrimitiveTopology = Topology;
        HW.get_context(context_id)->IASetPrimitiveTopology(m_PrimitiveTopology);
    }
}

#ifdef USE_DX11
IC void CBackend::Compute(u32 ThreadGroupCountX, u32 ThreadGroupCountY, u32 ThreadGroupCountZ)
{
    stat.compute.calls++;
    stat.compute.groups_x = ThreadGroupCountX;
    stat.compute.groups_y = ThreadGroupCountY;
    stat.compute.groups_z = ThreadGroupCountZ;

    SRVSManager.Apply(context_id);
    StateManager.Apply();
    //  State manager may alter constants
    constants.flush();
    HW.get_context(context_id)->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
}

IC void CBackend::RenderInstancedIndexed(D3D_PRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC, u32 instanceCount, u32 startInstanceLocation)
{
    D3D_PRIMITIVE_TOPOLOGY Topology = TranslateTopology(T);
    u32 iIndexCount = GetIndexCount(T, PC);

    //!!! HACK !!!
    if (hs != 0 || ds != 0)
    {
        R_ASSERT(Topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
    }

    stat.render.calls++;
    stat.render.verts += countV * instanceCount;
    stat.render.polys += PC * instanceCount;

    ApplyPrimitieTopology(Topology);

    SRVSManager.Apply(context_id);
    ApplyRTandZB();
    ApplyVertexLayout();
    StateManager.Apply();
    //  State manager may alter constants
    constants.flush();

    HW.get_context(context_id)->DrawIndexedInstanced(iIndexCount, instanceCount, startI, baseV, startInstanceLocation);

    PGO(Msg("PGO:DIP:%dv/%df", countV, PC));
}

IC void CBackend::RenderIndexedInstancedIndirect(D3D_PRIMITIVETYPE T, ID3DBuffer* pBufferForArgs, u32 AlignedByteOffsetForArgs)
{
    D3D_PRIMITIVE_TOPOLOGY Topology = TranslateTopology(T);

    //!!! HACK !!!
    if (hs != 0 || ds != 0)
    {
        R_ASSERT(Topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
    }

    stat.render.calls++;
    // Note: Can't update verts/polys stats accurately since instance count is GPU-determined

    ApplyPrimitieTopology(Topology);

    SRVSManager.Apply(context_id);
    ApplyRTandZB();
    ApplyVertexLayout();
    StateManager.Apply();
    //  State manager may alter constants
    constants.flush();

    HW.get_context(context_id)->DrawIndexedInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);

    PGO(Msg("PGO:DIP:Indirect"));
}
#endif

IC void CBackend::Render(D3D_PRIMITIVETYPE T, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
    // VERIFY(vs);
    // HW.pDevice->VSSetShader(vs);
    // HW.pDevice->GSSetShader(0);

    D3D_PRIMITIVE_TOPOLOGY Topology = TranslateTopology(T);
    u32 iIndexCount = GetIndexCount(T, PC);

//!!! HACK !!!
#ifdef USE_DX11
    if (hs != 0 || ds != 0)
    {
        R_ASSERT(Topology == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Topology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
    }
#endif

    stat.render.calls++;
    stat.render.verts += countV;
    stat.render.polys += PC;

    ApplyPrimitieTopology(Topology);

    // CHK_DX(HW.pDevice->DrawIndexedPrimitive(T,baseV, startV, countV,startI,PC));
    // D3D_PRIMITIVETYPE Type,
    // INT BaseVertexIndex,
    // UINT MinIndex,
    // UINT NumVertices,
    // UINT StartIndex,
    // UINT PriResmitiveCount

    // UINT IndexCount,
    // UINT StartIndexLocation,
    // INT BaseVertexLocation
    SRVSManager.Apply(context_id);
    ApplyRTandZB();
    ApplyVertexLayout();
    StateManager.Apply();
    //  State manager may alter constants
    constants.flush();
    //  Msg("DrawIndexed: Start");
    //  Msg("iIndexCount=%d, startI=%d, baseV=%d", iIndexCount, startI, baseV);
    HW.get_context(context_id)->DrawIndexed(iIndexCount, startI, baseV);
    //  Msg("DrawIndexed: End\n");

    PGO(Msg("PGO:DIP:%dv/%df", countV, PC));
}

IC void CBackend::Render(D3D_PRIMITIVETYPE T, u32 startV, u32 PC)
{
    //  TODO: DX11: Remove triangle fan usage from the engine
    if (T == D3D_PT_TRIANGLEFAN)
        return;

    // VERIFY(vs);
    // HW.pDevice->VSSetShader(vs);

    D3D_PRIMITIVE_TOPOLOGY Topology = TranslateTopology(T);
    u32 iVertexCount = GetIndexCount(T, PC);

    stat.render.calls++;
    stat.render.verts += 3 * PC;
    stat.render.polys += PC;

    ApplyPrimitieTopology(Topology);
    SRVSManager.Apply(context_id);
    ApplyRTandZB();
    ApplyVertexLayout();
    StateManager.Apply();
    //  State manager may alter constants
    constants.flush();
    //  Msg("Draw: Start");
    //  Msg("iVertexCount=%d, startV=%d", iVertexCount, startV);
    // CHK_DX               (HW.pDevice->DrawPrimitive(T, startV, PC));
    HW.get_context(context_id)->Draw(iVertexCount, startV);
    //  Msg("Draw: End\n");
    PGO(Msg("PGO:DIP:%dv/%df", 3 * PC, PC));
}

IC void CBackend::set_Geometry(SGeometry* _geom)
{
    set_Format(&*_geom->dcl);

    set_Vertices(_geom->vb, _geom->vb_stride);
    set_Indices(_geom->ib);
}

IC void CBackend::set_Scissor(const Irect* R)
{
    if (R)
    {
        // CHK_DX       (HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE));
        StateManager.EnableScissoring();
        // For D3D12/FrameGraph: Scissor is set per-pass in NVRHI graphics state
        // Only call D3D11 context method if we have a valid D3D11 context
        if (auto* ctx = HW.get_context(context_id))
        {
            RECT* clip = (RECT*)R;
            ctx->RSSetScissorRects(1, clip);
        }
    }
    else
    {
        // CHK_DX       (HW.pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE));
        StateManager.EnableScissoring(FALSE);
        // For D3D12/FrameGraph: Scissor is set per-pass in NVRHI graphics state
        if (auto* ctx = HW.get_context(context_id))
        {
            ctx->RSSetScissorRects(0, 0);
        }
    }
}

IC void CBackend::SetViewport(const D3D_VIEWPORT& viewport) const
{
    // For D3D12/FrameGraph: Viewport is set per-pass in NVRHI graphics state
    // Only call D3D11 context method if we have a valid D3D11 context
    if (auto* ctx = HW.get_context(context_id))
    {
        ctx->RSSetViewports(1, &viewport);
    }
}

IC void CBackend::set_Stencil(
    u32 _enable, u32 _func, u32 _ref, u32 _mask, u32 _writemask, u32 _fail, u32 _pass, u32 _zfail)
{
    StateManager.SetStencil(_enable, _func, _ref, _mask, _writemask, _fail, _pass, _zfail);
    // Simple filter
    // if (stencil_enable       != _enable)     { stencil_enable=_enable;       CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILENABLE,     _enable             )); }
    // if (!stencil_enable)                 return;
    // if (stencil_func     != _func)       { stencil_func=_func;           CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILFUNC,
    // _func                )); }
    // if (stencil_ref          != _ref)        { stencil_ref=_ref;             CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILREF,
    // _ref
    // )); }
    // if (stencil_mask     != _mask)       { stencil_mask=_mask;           CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILMASK,
    // _mask                )); }
    // if (stencil_writemask    != _writemask)  { stencil_writemask=_writemask; CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILWRITEMASK,  _writemask          )); }
    // if (stencil_fail     != _fail)       { stencil_fail=_fail;           CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILFAIL,
    // _fail                )); }
    // if (stencil_pass     != _pass)       { stencil_pass=_pass;           CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILPASS,
    // _pass                )); }
    // if (stencil_zfail        != _zfail)      { stencil_zfail=_zfail;         CHK_DX(HW.pDevice->SetRenderState   (
    // D3DRS_STENCILZFAIL,
    // _zfail               )); }
}

IC void CBackend::set_Z(u32 _enable)
{
    StateManager.SetDepthEnable(_enable);
    // if (z_enable != _enable)
    //{
    //  z_enable=_enable;
    //  CHK_DX(HW.pDevice->SetRenderState   ( D3DRS_ZENABLE, _enable ));
    //}
}

IC void CBackend::set_ZFunc(u32 _func)
{
    StateManager.SetDepthFunc(_func);
    // if (z_func!=_func)
    //{
    //  z_func = _func;
    //  CHK_DX(HW.pDevice->SetRenderState( D3DRS_ZFUNC, _func));
    //}
}

IC void CBackend::set_AlphaRef(u32 _value)
{
    //  TODO: DX11: Implement rasterizer state update to support alpha ref
    VERIFY(!"Not implemented.");
    // if (alpha_ref != _value)
    //{
    //  alpha_ref = _value;
    //  CHK_DX(HW.pDevice->SetRenderState(D3DRS_ALPHAREF,_value));
    //}
}

IC void CBackend::set_ColorWriteEnable(u32 _mask)
{
    StateManager.SetColorWriteEnable(_mask);
    // if (colorwrite_mask      != _mask)       {
    //  colorwrite_mask=_mask;
    //  CHK_DX(HW.pDevice->SetRenderState   ( D3DRS_COLORWRITEENABLE,   _mask   ));
    //  CHK_DX(HW.pDevice->SetRenderState   ( D3DRS_COLORWRITEENABLE1,  _mask   ));
    //  CHK_DX(HW.pDevice->SetRenderState   ( D3DRS_COLORWRITEENABLE2,  _mask   ));
    //  CHK_DX(HW.pDevice->SetRenderState   ( D3DRS_COLORWRITEENABLE3,  _mask   ));
    //}
}
ICF void CBackend::set_CullMode(u32 _mode)
{
    StateManager.SetCullMode(_mode);
    cull_mode = _mode;
}

ICF void CBackend::set_FillMode(u32 _mode)
{
    StateManager.SetFillMode(_mode);
}

ICF void CBackend::SetTextureFactor(u32 /*factor*/) const
{
    // Not supported
}

ICF void CBackend::SetAmbient(u32 /*factor*/) const
{
    // Not supported
}

IC void CBackend::ApplyVertexLayout()
{
    VERIFY(vs);
    VERIFY(decl);
    VERIFY(m_pInputSignature);

    xr_map<ID3DBlob*, ID3DInputLayout*>::iterator it;

    it = decl->vs_to_layout.find(m_pInputSignature);

    if (it == decl->vs_to_layout.end())
    {
        ID3DInputLayout* pLayout;

        CHK_DX(HW.pDevice->CreateInputLayout(&decl->dx11_dcl_code[0], decl->dx11_dcl_code.size() - 1,
            m_pInputSignature->GetBufferPointer(), m_pInputSignature->GetBufferSize(), &pLayout));

        it = decl->vs_to_layout.insert(std::pair<ID3DBlob*, ID3DInputLayout*>(m_pInputSignature, pLayout)).first;
    }

    if (m_pInputLayout != it->second)
    {
        m_pInputLayout = it->second;
        HW.get_context(context_id)->IASetInputLayout(m_pInputLayout);
    }
}

ICF void CBackend::set_VS(ref_vs& _vs)
{
    // No-op in FrameGraph mode - vertex shaders are set via NVRHI pipeline
}

ICF void CBackend::set_VS(SVS* _vs)
{
    // No-op in FrameGraph mode - vertex shaders are set via NVRHI pipeline
}

IC bool CBackend::CBuffersNeedUpdate(
    ref_cbuffer buf1[MaxCBuffers], ref_cbuffer buf2[MaxCBuffers], u32& uiMin, u32& uiMax)
{
    bool bRes = false;
    int i = 0;
    while ((i < MaxCBuffers) && (buf1[i] == buf2[i]))
        ++i;

    uiMin = i;

    for (; i < MaxCBuffers; ++i)
    {
        if (buf1[i] != buf2[i])
        {
            bRes = true;
            uiMax = i;
        }
    }

    return bRes;
}

IC void CBackend::set_Constants(R_constant_table* C)
{
}

ICF void CBackend::ApplyRTandZB()
{
    if (m_bChangedRTorZB)
    {
        m_bChangedRTorZB = false;
        HW.get_context(context_id)->OMSetRenderTargets(sizeof(pRT) / sizeof(pRT[0]), pRT, pZB);
    }
}

IC void CBackend::get_ConstantDirect(const shared_str& n, size_t DataSize, void** pVData, void** pGData, void** pPData)
{
    ref_constant C = get_c(n);

    if (C)
        constants.access_direct(&*C, DataSize, pVData, pGData, pPData);
    else
    {
        if (pVData)
            *pVData = 0;
        if (pGData)
            *pGData = 0;
        if (pPData)
            *pPData = 0;
    }
}

IC void CBackend::gpu_mark_begin(const wchar_t* name)
{
    pAnnotation->BeginEvent(name);
}

IC void CBackend::gpu_mark_end()
{
    pAnnotation->EndEvent();
}

IC void CBackend::set_pass_targets(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& zb)
{
    if (_1)
    {
        curr_rt_width = _1->dwWidth;
        curr_rt_height = _1->dwHeight;
    }
    else
    {
        VERIFY(zb);
        curr_rt_width = zb->dwWidth;
        curr_rt_height = zb->dwHeight;
    }

    set_RT(_1 ? _1->pRT : nullptr, 0);
    set_RT(_2 ? _2->pRT : nullptr, 1);
    set_RT(_3 ? _3->pRT : nullptr, 2);
    set_ZB(zb ? zb->pZRT[context_id] : nullptr);

    const D3D_VIEWPORT viewport = { 0, 0, curr_rt_width, curr_rt_height, 0.f, 1.f };
    SetViewport(viewport);
}
} // namespace xray::render::fg
