#include "stdafx.h"
#include "dx103DFluidManager.h"
#include "dx103DFluidBlenders.h"
#include "dx103DFluidData.h"
#include "dx103DFluidGrid.h"
#include "dx103DFluidRenderer.h"
#include "dx103DFluidObstacles.h"
#include "dx103DFluidEmitters.h"

dx103DFluidManager FluidManager;

namespace
{
// For render call
// DrawTextureShaderVariable = pEffect->GetVariableByName( "textureNumber")->AsScalar();
shared_str strDrawTexture("textureNumber");
// For project, advect
// ModulateShaderVariable = pEffect->GetVariableByName( "modulate")->AsScalar();
shared_str strModulate("modulate");
// For gaussian
//ImpulseSizeShaderVariable = pEffect->GetVariableByName("size")->AsScalar();
//shared_str strImpulseSize("size");
//ImpulseCenterShaderVariable = pEffect->GetVariableByName("center")->AsVector();
//shared_str strImpulseCenter("center");
//SplatColorShaderVariable = pEffect->GetVariableByName("splatColor")->AsVector();
//shared_str strSplatColor("splatColor");
// For confinement
// EpsilonShaderVariable = pEffect->GetVariableByName( "epsilon")->AsScalar();
shared_str strEpsilon("epsilon");
// For confinement, advect
shared_str strTimeStep("timestep");
// For advect BFECC
// ForwardShaderVariable = pEffect->GetVariableByName( "forward")->AsScalar();
shared_str strForward("forward");
// HalfVolumeDimShaderVariable = pEffect->GetVariableByName( "halfVolumeDim")->AsVector();
shared_str strHalfVolumeDim("halfVolumeDim");

shared_str strGravityBuoyancy("GravityBuoyancy");
}

LPCSTR dx103DFluidManager::m_pEngineTextureNames[NUM_RENDER_TARGETS] = {
    "$user$Texture_velocity1", //RENDER_TARGET_VELOCITY1 = 0,
    // Swap with object's
    "$user$Texture_color_out", //RENDER_TARGET_COLOR,
    "$user$Texture_obstacles", //RENDER_TARGET_OBSTACLES,
    "$user$Texture_obstvelocity", //RENDER_TARGET_OBSTVELOCITY,
    "$user$Texture_tempscalar", //RENDER_TARGET_TEMPSCALAR,
    "$user$Texture_tempvector", // RENDER_TARGET_TEMPVECTOR,
    // For textures generated from local data
    "$user$Texture_velocity0", //RENDER_TARGET_VELOCITY0 = NUM_OWN_RENDER_TARGETS,
    "$user$Texture_pressure", //RENDER_TARGET_PRESSURE,
    "$user$Texture_color", //RENDER_TARGET_COLOR_IN,
};

LPCSTR dx103DFluidManager::m_pShaderTextureNames[NUM_RENDER_TARGETS] = {
    "Texture_velocity1", //RENDER_TARGET_VELOCITY1 = 0,
    // Swap with object's
    "Texture_color_out", //RENDER_TARGET_COLOR,
    "Texture_obstacles", //RENDER_TARGET_OBSTACLES,
    "Texture_obstvelocity", //RENDER_TARGET_OBSTVELOCITY,
    "Texture_tempscalar", //RENDER_TARGET_TEMPSCALAR,
    "Texture_tempvector", //RENDER_TARGET_TEMPVECTOR,
    // For textures generated from local data
    "Texture_velocity0", //RENDER_TARGET_VELOCITY0 = NUM_OWN_RENDER_TARGETS,
    "Texture_pressure", //  RENDER_TARGET_PRESSURE,
    "Texture_color", // RENDER_TARGET_COLOR_IN,
};

dx103DFluidManager::dx103DFluidManager()
    : m_bInited(false),
      // m_nIterations(10), m_bUseBFECC(true),
      m_nIterations(6), m_bUseBFECC(true),
      // m_nIterations(6), m_bUseBFECC(false),
      m_fSaturation(0.78f), m_bAddDensity(true), m_fImpulseSize(0.15f), m_fConfinementScale(0.0f), m_fDecay(1.0f),
      m_pGrid(0), m_pRenderer(0), m_pObstaclesHandler(0)
{
    ZeroMemory(pRenderTargetViews, sizeof(pRenderTargetViews));

    // RenderTargetFormats [RENDER_TARGET_VELOCITY0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    RenderTargetFormats[RENDER_TARGET_VELOCITY1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    // RenderTargetFormats [RENDER_TARGET_PRESSURE] = DXGI_FORMAT_R16_FLOAT;
    RenderTargetFormats[RENDER_TARGET_COLOR] = DXGI_FORMAT_R16_FLOAT;
    RenderTargetFormats[RENDER_TARGET_OBSTACLES] = DXGI_FORMAT_R8_UNORM;
    RenderTargetFormats[RENDER_TARGET_OBSTVELOCITY] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    // RENDER_TARGET_TEMPSCALAR: for AdvectBFECC and for Jacobi (for pressure projection)
    RenderTargetFormats[RENDER_TARGET_TEMPSCALAR] = DXGI_FORMAT_R16_FLOAT;
    // RENDER_TARGET_TEMPVECTOR: for Advect2, Divergence, and Vorticity
    RenderTargetFormats[RENDER_TARGET_TEMPVECTOR] = DXGI_FORMAT_R16G16B16A16_FLOAT;
}

dx103DFluidManager::~dx103DFluidManager() { Destroy(); }
void dx103DFluidManager::Initialize(int width, int height, int depth)
{
    // if (strstr(Core.Params,"-no_volumetric_fog"))
    if (!RImplementation.o.volumetricfog)
        return;

    Destroy();

    m_iTextureWidth = width;
    m_iTextureHeight = height;
    m_iTextureDepth = depth;

    InitShaders();

    D3D_TEXTURE3D_DESC desc;
    desc.BindFlags = D3D_BIND_SHADER_RESOURCE | D3D_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MipLevels = 1;
    desc.MiscFlags = 0;
    desc.Usage = D3D_USAGE_DEFAULT;
    desc.Width = width;
    desc.Height = height;
    desc.Depth = depth;

    D3D_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ZeroMemory(&SRVDesc, sizeof(SRVDesc));
    SRVDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
    SRVDesc.Texture3D.MipLevels = 1;
    SRVDesc.Texture3D.MostDetailedMip = 0;

    for (size_t rtIndex = 0; rtIndex < NUM_RENDER_TARGETS; rtIndex++)
    {
        PrepareTexture(rtIndex);
        pRenderTargetViews[rtIndex] = 0;
    }

    for (size_t rtIndex = 0; rtIndex < NUM_OWN_RENDER_TARGETS; rtIndex++)
    {
        desc.Format = RenderTargetFormats[rtIndex];
        SRVDesc.Format = RenderTargetFormats[rtIndex];
        CreateRTTextureAndViews(rtIndex, desc);
    }

    Reset();

    m_pGrid = new dx103DFluidGrid();

    m_pGrid->Initialize(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);

    m_pRenderer = new dx103DFluidRenderer();
    m_pRenderer->Initialize(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);

    m_pObstaclesHandler = new dx103DFluidObstacles(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth, m_pGrid);

    m_pEmittersHandler = new dx103DFluidEmitters(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth, m_pGrid);

    m_bInited = true;

    //  Create and grid and renderer here
    // grid = new Grid( m_pD3DDevice );
    // renderer = new VolumeRenderer( m_pD3DDevice );
}

void dx103DFluidManager::Destroy()
{
    if (!m_bInited)
        return;

    //  Destroy grid and renderer here
    xr_delete(m_pEmittersHandler);
    xr_delete(m_pObstaclesHandler);
    xr_delete(m_pRenderer);
    xr_delete(m_pGrid);
    // grid = new Grid( m_pD3DDevice );
    // renderer = new VolumeRenderer( m_pD3DDevice );

    // for(size_t rtIndex=0; rtIndex<NUM_OWN_RENDER_TARGETS; rtIndex++)
    for (size_t rtIndex = 0; rtIndex < NUM_RENDER_TARGETS; rtIndex++)
        DestroyRTTextureAndViews(rtIndex);

    DestroyShaders();

    m_bInited = false;
}

void dx103DFluidManager::InitShaders()
{
    {
        CBlender_fluid_advect Blender;
        ref_shader shader;
        shader.create(&Blender, "null");
        for (size_t i = 0; i < 4; ++i)
            m_SimulationTechnique[SS_Advect + i] = shader->E[i];
    }

    {
        CBlender_fluid_advect_velocity Blender;
        ref_shader shader;
        shader.create(&Blender, "null");
        for (size_t i = 0; i < 2; ++i)
            m_SimulationTechnique[SS_AdvectVel + i] = shader->E[i];
    }

    {
        CBlender_fluid_simulate Blender;
        ref_shader shader;
        shader.create(&Blender, "null");
        for (size_t i = 0; i < 5; ++i)
            m_SimulationTechnique[SS_Vorticity + i] = shader->E[i];
    }
}

void dx103DFluidManager::DestroyShaders()
{
    for (size_t i = 0; i < SS_NumShaders; ++i)
    {
        //  Release shader's element.
        m_SimulationTechnique[i] = nullptr;
    }
}

void dx103DFluidManager::PrepareTexture(size_t rtIndex)
{
    pRTTextures[rtIndex] = RImplementation.Resources->_CreateTexture(m_pEngineTextureNames[rtIndex]);
}

void dx103DFluidManager::CreateRTTextureAndViews(size_t rtIndex, D3D_TEXTURE3D_DESC TexDesc)
{
    // Resources must be already released by Destroy().

    ID3DTexture3D* pRT;

    // Create the texture
    CHK_DX(HW.pDevice->CreateTexture3D(&TexDesc, NULL, &pRT));
    // Create the render target view
    D3D_RENDER_TARGET_VIEW_DESC DescRT;
    DescRT.Format = TexDesc.Format;
    DescRT.ViewDimension = D3D_RTV_DIMENSION_TEXTURE3D;
    DescRT.Texture3D.FirstWSlice = 0;
    DescRT.Texture3D.MipSlice = 0;
    DescRT.Texture3D.WSize = TexDesc.Depth;

    CHK_DX(HW.pDevice->CreateRenderTargetView(pRT, &DescRT, &pRenderTargetViews[rtIndex]));

    pRTTextures[rtIndex]->surface_set(pRT);

    // CTexture owns ID3DxxTexture3D interface
    pRT->Release();
}
void dx103DFluidManager::DestroyRTTextureAndViews(size_t rtIndex)
{
    // pRTTextures[rtIndex]->surface_set(0);
    pRTTextures[rtIndex] = nullptr;
    _RELEASE(pRenderTargetViews[rtIndex]);
}

void dx103DFluidManager::Reset()
{
    float color[4] = {0, 0, 0, 0};

    for (size_t rtIndex = 0; rtIndex < NUM_OWN_RENDER_TARGETS; rtIndex++)
    {
        HW.pContext->ClearRenderTargetView(pRenderTargetViews[rtIndex], color);
    }
}

void dx103DFluidManager::Update(dx103DFluidData& FluidData, float timestep)
{
    PIX_EVENT(simulate_fluid);

    const dx103DFluidData::Settings& VolumeSettings = FluidData.GetSettings();
    const bool bSimulateFire = (VolumeSettings.m_SimulationType == dx103DFluidData::ST_FIRE);

    AttachFluidData(FluidData);

    // All drawing will take place to a viewport with the dimensions of a 3D texture slice
    D3D_VIEWPORT rtViewport;
    rtViewport.TopLeftX = 0;
    rtViewport.TopLeftY = 0;
    rtViewport.MinDepth = 0.0f;
    rtViewport.MaxDepth = 1.0f;
#ifdef USE_DX11
    rtViewport.Width = (float)m_iTextureWidth;
    rtViewport.Height = (float)m_iTextureHeight;
#else // #ifdef USE_DX11
    rtViewport.Width = m_iTextureWidth;
    rtViewport.Height = m_iTextureHeight;
#endif // #ifdef USE_DX11
    HW.pContext->RSSetViewports(1, &rtViewport);

    RCache.set_ZB(0);

    /*
    // Update the obstacle velocity based on its movement
    {
        obstVelocity = (obstPos - obstPrevPos) / timestep;
        // Exagerate the velocity a bit to give more momentum to the smoke
        obstVelocity *= 1.5f;
        // Scale obstVelocity to voxel space
        obstVelocity.x *= grid->dim[0]; obstVelocity.y *= grid->dim[1]; obstVelocity.z *= grid->dim[2];
        pEffect->GetVariableByName("obstVelocity")->AsVector()->SetFloatVector(obstVelocity);
        obstPrevPos = obstPos;
    }

    // Hard-coded procedural obstacle box for simple testing
    if( mustUpdateObstacles ) {
        UpdateObstacles();
        mustUpdateObstacles = true;
    }
    */

    UpdateObstacles(FluidData, timestep);

    // Set vorticity confinment and decay parameters
    if (m_bUseBFECC)
    {
        if (bSimulateFire)
        {
            m_fConfinementScale = 0.03f;
            m_fDecay = 0.9995f;
        }
        else
        {
            m_fConfinementScale = 0.06f;
            m_fDecay = 0.994f;
        }
    }
    else
    {
        m_fConfinementScale = 0.12f;
        m_fDecay = 0.9995f;
    }

    m_fConfinementScale = VolumeSettings.m_fConfinementScale;
    m_fDecay = VolumeSettings.m_fDecay;

    if (m_bUseBFECC)
        AdvectColorBFECC(timestep, bSimulateFire);
    else
        AdvectColor(timestep, bSimulateFire);

    AdvectVelocity(timestep, VolumeSettings.m_fGravityBuoyancy);

    ApplyVorticityConfinement(timestep);

    ApplyExternalForces(FluidData, timestep);

    ComputeVelocityDivergence(timestep);

    ComputePressure(timestep);

    ProjectVelocity(timestep);

    DetachAndSwapFluidData(FluidData);

    //  Restore render state
    CRenderTarget* pTarget = RImplementation.Target;
    if (!RImplementation.o.dx10_msaa)
        pTarget->u_setrt(pTarget->rt_Generic_0, 0, 0, HW.pBaseZB); // LDR RT
    else
        pTarget->u_setrt(pTarget->rt_Generic_0_r, 0, 0, pTarget->rt_MSAADepth->pZRT); // LDR RT

    RImplementation.rmNormal();
    // RImplementation.Target->phase_scene_begin();
}

void dx103DFluidManager::AttachFluidData(dx103DFluidData& FluidData)
{
    PIX_EVENT(AttachFluidData);

    for (size_t i = 0; i < dx103DFluidData::VP_NUM_TARGETS; ++i)
    {
        ID3DTexture3D* pT = FluidData.GetTexture((dx103DFluidData::eVolumePrivateRT)i);
        pRTTextures[RENDER_TARGET_VELOCITY0 + i]->surface_set(pT);
        _RELEASE(pT);

        VERIFY(!pRenderTargetViews[RENDER_TARGET_VELOCITY0 + i]);
        pRenderTargetViews[RENDER_TARGET_VELOCITY0 + i] = FluidData.GetView((dx103DFluidData::eVolumePrivateRT)i);
    }
}

void dx103DFluidManager::DetachAndSwapFluidData(dx103DFluidData& FluidData)
{
    PIX_EVENT(DetachAndSwapFluidData);

    ID3DTexture3D* pTTarg = (ID3DTexture3D*)pRTTextures[RENDER_TARGET_COLOR]->surface_get();
    ID3DTexture3D* pTSrc = FluidData.GetTexture(dx103DFluidData::VP_COLOR);
    FluidData.SetTexture(dx103DFluidData::VP_COLOR, pTTarg);
    pRTTextures[RENDER_TARGET_COLOR]->surface_set(pTSrc);
    _RELEASE(pTTarg);
    _RELEASE(pTSrc);

    ID3DRenderTargetView* pV = FluidData.GetView(dx103DFluidData::VP_COLOR);
    FluidData.SetView(dx103DFluidData::VP_COLOR, pRenderTargetViews[RENDER_TARGET_COLOR]);
    _RELEASE(pRenderTargetViews[RENDER_TARGET_COLOR]);
    pRenderTargetViews[RENDER_TARGET_COLOR] = pV;

    for (size_t i = 0; i < dx103DFluidData::VP_NUM_TARGETS; ++i)
    {
        pRTTextures[RENDER_TARGET_VELOCITY0 + i]->surface_set(0);
        _RELEASE(pRenderTargetViews[RENDER_TARGET_VELOCITY0 + i]);
    }
}

void dx103DFluidManager::AdvectColorBFECC(float timestep, bool bTeperature)
{
    PIX_EVENT(AdvectColorBFECC);

    float color[4] = {0, 0, 0, 0};

    HW.pContext->ClearRenderTargetView(pRenderTargetViews[RENDER_TARGET_TEMPVECTOR], color);
    HW.pContext->ClearRenderTargetView(pRenderTargetViews[RENDER_TARGET_TEMPSCALAR], color);

    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_TEMPVECTOR]);
    if (bTeperature)
        RCache.set_Element(m_SimulationTechnique[SS_AdvectTemp]);
    else
        RCache.set_Element(m_SimulationTechnique[SS_Advect]);
    // Advect forward to get \phi^(n+1)
    // pShaderResourceVariables[RENDER_TARGET_TEMPVECTOR]->SetResource( NULL );
    // TimeStepShaderVariable->SetFloat(timestep);
    RCache.set_c(strTimeStep, timestep);
    // ModulateShaderVariable->SetFloat(1.0f);
    RCache.set_c(strModulate, 1.0f);
    // ForwardShaderVariable->SetFloat(1.0f);
    RCache.set_c(strForward, 1.0f);
    // SetRenderTarget( RENDER_TARGET_TEMPVECTOR );
    // TechniqueAdvect->GetPassByIndex(0)->Apply(0);
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_TEMPVECTOR]->SetResource(
    // pRenderTargetShaderViews[RENDER_TARGET_TEMPVECTOR] );

    // Advect back to get \bar{\phi}
    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_TEMPSCALAR]);
    ref_selement AdvectElement;
    if (bTeperature)
        AdvectElement = m_SimulationTechnique[SS_AdvectTemp];
    else
        AdvectElement = m_SimulationTechnique[SS_Advect];

    RCache.set_Element(AdvectElement);
    // pShaderResourceVariables[RENDER_TARGET_TEMPSCALAR]->SetResource( NULL );
    // pShaderResourceVariables[RENDER_TARGET_COLOR0]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_TEMPVECTOR]
    // );
    //  Overwrite RENDER_TARGET_COLOR0 with RENDER_TARGET_TEMPVECTOR
    //  Find texture index and patch texture manually using DirecX call!
    static shared_str strColorName(m_pEngineTextureNames[RENDER_TARGET_COLOR_IN]);
    STextureList* _T = &*(AdvectElement->passes[0]->T);
    u32 dwTextureStage = _T->find_texture_stage(strColorName);
    //  This will be overritten by the next technique.
    //  Otherwise we had to reset current texture list manually.
    pRTTextures[RENDER_TARGET_TEMPVECTOR]->bind(dwTextureStage);

    // TimeStepShaderVariable->SetFloat(timestep);
    RCache.set_c(strTimeStep, timestep);
    // ModulateShaderVariable->SetFloat(1.0f);
    RCache.set_c(strModulate, 1.0f);
    // ForwardShaderVariable->SetFloat(-1.0);
    RCache.set_c(strForward, -1.0f);
    // SetRenderTarget( RENDER_TARGET_TEMPSCALAR );
    // TechniqueAdvect->GetPassByIndex(0)->Apply(0);
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_TEMPSCALAR]->SetResource(
    // pRenderTargetShaderViews[RENDER_TARGET_TEMPSCALAR] );

    // Advect forward but use the BFECC advection shader which
    //  uses both \phi and \bar{\phi} as source quantity
    //  (specifically, (3/2)\phi^n - (1/2)\bar{\phi})
    // if(ColorTextureNumber == 0)
    //{
    //  pShaderResourceVariables[RENDER_TARGET_COLOR1]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_COLOR1] );
    //  SetRenderTarget( RENDER_TARGET_COLOR0 );
    //}
    // else
    //{
    //  pShaderResourceVariables[RENDER_TARGET_COLOR0]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_COLOR0] );
    //  SetRenderTarget( RENDER_TARGET_COLOR1 );
    //}
    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_COLOR]);
    if (bTeperature)
        RCache.set_Element(m_SimulationTechnique[SS_AdvectBFECCTemp]);
    else
        RCache.set_Element(m_SimulationTechnique[SS_AdvectBFECC]);

    // D3DXVECTOR3 halfVol( grid->dim[0]/2.0f, grid->dim[1]/2.0f, grid->dim[2]/2.0f );
    // HalfVolumeDimShaderVariable->SetFloatVector( (float*)&halfVol);
    Fvector4 halfVol;
    halfVol.set((float)m_iTextureWidth / 2.0f, (float)m_iTextureHeight / 2.0f, (float)m_iTextureDepth / 2.0f, 0.0f);
    RCache.set_c(strHalfVolumeDim, halfVol);
    // ModulateShaderVariable->SetFloat(decay);
    RCache.set_c(strModulate, m_fDecay);
    // ForwardShaderVariable->SetFloat(1.0);
    RCache.set_c(strForward, 1.0f);
    // TechniqueAdvectBFECC->GetPassByIndex(0)->Apply(0);
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_TEMPSCALAR]->SetResource( NULL );
    // Apply the technique again so that the RENDER_TARGET_TEMPSCALAR shader resource is unbound
    // TechniqueAdvectBFECC->GetPassByIndex(0)->Apply(0);*/
}

void dx103DFluidManager::AdvectColor(float timestep, bool bTeperature)
{
    PIX_EVENT(AdvectColor);
    // if(ColorTextureNumber == 0)
    //{
    //  pShaderResourceVariables[RENDER_TARGET_COLOR1]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_COLOR1] );
    //  SetRenderTarget( RENDER_TARGET_COLOR0 );
    //}
    // else
    //{
    //  pShaderResourceVariables[RENDER_TARGET_COLOR0]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_COLOR0] );
    //  SetRenderTarget( RENDER_TARGET_COLOR1 );
    //}

    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_COLOR]);

    if (bTeperature)
        RCache.set_Element(m_SimulationTechnique[SS_AdvectTemp]);
    else
        RCache.set_Element(m_SimulationTechnique[SS_Advect]);

    // TimeStepShaderVariable->SetFloat(timestep);
    RCache.set_c(strTimeStep, timestep);
    // ModulateShaderVariable->SetFloat(1.0f);
    RCache.set_c(strModulate, 1.0f);
    // ForwardShaderVariable->SetFloat(1.0);
    RCache.set_c(strForward, 1.0f);
    // ModulateShaderVariable->SetFloat(decay);
    RCache.set_c(strModulate, m_fDecay);
    // TechniqueAdvect->GetPassByIndex(0)->Apply(0);

    m_pGrid->DrawSlices();
}

void dx103DFluidManager::AdvectVelocity(float timestep, float fGravity)
{
    PIX_EVENT(AdvectVelocity);

    // pShaderResourceVariables[RENDER_TARGET_VELOCITY1]->SetResource( NULL );
    // SetRenderTarget(RENDER_TARGET_VELOCITY1);
    // Advect velocity by the fluid velocity
    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_VELOCITY1]);

    if (_abs(fGravity) < 0.000001)
        RCache.set_Element(m_SimulationTechnique[SS_AdvectVel]);
    else
    {
        RCache.set_Element(m_SimulationTechnique[SS_AdvectVelGravity]);
        RCache.set_c(strGravityBuoyancy, fGravity);
    }

    // TimeStepShaderVariable->SetFloat(timestep);
    RCache.set_c(strTimeStep, timestep);
    // ModulateShaderVariable->SetFloat(1.0 );
    RCache.set_c(strModulate, 1.0f);
    // ForwardShaderVariable->SetFloat(1.0);
    RCache.set_c(strForward, 1.0f);
    // TechniqueAdvectVel->GetPassByIndex(0)->Apply(0);
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_VELOCITY1]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_VELOCITY1]
    // );
}

void dx103DFluidManager::ApplyVorticityConfinement(float timestep)
{
    PIX_EVENT(ApplyVorticityConfinement);

    // Compute vorticity
    float color[4] = {0, 0, 0, 0};
    HW.pContext->ClearRenderTargetView(pRenderTargetViews[RENDER_TARGET_TEMPVECTOR], color);

    // pShaderResourceVariables[RENDER_TARGET_TEMPVECTOR]->SetResource( NULL );
    // SetRenderTarget( RENDER_TARGET_TEMPVECTOR );
    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_TEMPVECTOR]);
    // TechniqueVorticity->GetPassByIndex(0)->Apply(0);
    RCache.set_Element(m_SimulationTechnique[SS_Vorticity]);
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_TEMPVECTOR]->SetResource(
    // pRenderTargetShaderViews[RENDER_TARGET_TEMPVECTOR] );

    // Compute and apply vorticity confinement force
    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_VELOCITY1]);
    RCache.set_Element(m_SimulationTechnique[SS_Confinement]);
    // pShaderResourceVariables[RENDER_TARGET_VELOCITY1]->SetResource( NULL );
    // EpsilonShaderVariable->SetFloat(confinementScale);
    RCache.set_c(strEpsilon, m_fConfinementScale);
    // TimeStepShaderVariable->SetFloat(timestep);
    RCache.set_c(strTimeStep, timestep);
    // TechniqueConfinement->GetPassByIndex(0)->Apply(0);
    // SetRenderTarget( RENDER_TARGET_VELOCITY1 );
    // Add the confinement force to the rest of the forces
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_VELOCITY1]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_VELOCITY1]
    // );
}

void dx103DFluidManager::ApplyExternalForces(const dx103DFluidData& FluidData, float /*timestep*/)
{
    PIX_EVENT(ApplyExternalForces);

    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_COLOR]);
    m_pEmittersHandler->RenderDensity(FluidData);

    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_VELOCITY1]);
    m_pEmittersHandler->RenderVelocity(FluidData);
}

void dx103DFluidManager::ComputeVelocityDivergence(float /*timestep*/)
{
    PIX_EVENT(ComputeVelocityDivergence);

    float color[4] = {0, 0, 0, 0};
    HW.pContext->ClearRenderTargetView(pRenderTargetViews[RENDER_TARGET_TEMPVECTOR], color);

    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_TEMPVECTOR]);
    RCache.set_Element(m_SimulationTechnique[SS_Divergence]);

    // pShaderResourceVariables[RENDER_TARGET_TEMPVECTOR]->SetResource( NULL );
    // SetRenderTarget( RENDER_TARGET_TEMPVECTOR );
    // TechniqueDivergence->GetPassByIndex(0)->Apply(0);
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_TEMPVECTOR]->SetResource(
    // pRenderTargetShaderViews[RENDER_TARGET_TEMPVECTOR] );
}

void dx103DFluidManager::ComputePressure(float /*timestep*/)
{
    PIX_EVENT(ComputePressure);

    float color[4] = {0, 0, 0, 0};
    HW.pContext->ClearRenderTargetView(pRenderTargetViews[RENDER_TARGET_TEMPSCALAR], color);

    // ID3DxxTexture3D  *pTemp = (ID3DxxTexture3D*) pRTTextures[RENDER_TARGET_TEMPSCALAR]->surface_get();
    // ID3DxxTexture3D  *pPressure = (ID3DxxTexture3D*) pRTTextures[RENDER_TARGET_PRESSURE]->surface_get();

    // unbind this variable from the other technique that may have used it
    // pShaderResourceVariables[RENDER_TARGET_TEMPSCALAR]->SetResource( NULL );
    // TechniqueAdvectBFECC->GetPassByIndex(0)->Apply(0);
    RCache.set_RT(0);
    ref_selement CurrentTechnique = m_SimulationTechnique[SS_Jacobi];
    RCache.set_Element(CurrentTechnique);

    //  Find texture index and patch texture manually using DirecX call!
    static shared_str strPressureName(m_pEngineTextureNames[RENDER_TARGET_PRESSURE]);
    STextureList* _T = &*(CurrentTechnique->passes[0]->T);
    u32 dwTextureStage = _T->find_texture_stage(strPressureName);
    // VERIFY(dwTextureStage != 1);
    /*
    u32                 dwTextureStage  = 0;
    STextureList*       _T = &*(CurrentTechnique->passes[0]->T);

    STextureList::iterator  _it     = _T->begin ();
    STextureList::iterator  _end    = _T->end   ();
    for (; _it!=_end; _it++)
    {
        std::pair<u32,ref_texture>&     loader  =   *_it;

        //  Shadowmap texture always uses 0 texture unit
        if (loader.second->cName==strPressureName)
        {
            //  Assign correct texture
            dwTextureStage  = loader.first;
            break;
        }
    }

    VERIFY(_it!=_end);
    */

    for (int iteration = 0; iteration < m_nIterations / 2.0; iteration++)
    {
        // pShaderResourceVariables[RENDER_TARGET_PRESSURE]->SetResource(
        // pRenderTargetShaderViews[RENDER_TARGET_PRESSURE] );
        // TechniqueJacobi->GetPassByIndex(0)->Apply(0);
        // SetRenderTarget( RENDER_TARGET_TEMPSCALAR );
        RCache.set_RT(pRenderTargetViews[RENDER_TARGET_TEMPSCALAR]);
        pRTTextures[RENDER_TARGET_PRESSURE]->bind(dwTextureStage);
        m_pGrid->DrawSlices();
        // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
        // RCache.set_RT(0);

        // pShaderResourceVariables[RENDER_TARGET_PRESSURE]->SetResource(
        // pRenderTargetShaderViews[RENDER_TARGET_TEMPSCALAR] );
        // TechniqueJacobi->GetPassByIndex(0)->Apply(0);
        // SetRenderTarget( RENDER_TARGET_PRESSURE );
        RCache.set_RT(pRenderTargetViews[RENDER_TARGET_PRESSURE]);
        pRTTextures[RENDER_TARGET_TEMPSCALAR]->bind(dwTextureStage);
        m_pGrid->DrawSlices();
        // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
        // RCache.set_RT(0);
    }

    // pShaderResourceVariables[RENDER_TARGET_PRESSURE]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_PRESSURE]
    // );
    // TechniqueJacobi->GetPassByIndex(0)->Apply(0);
}

void dx103DFluidManager::ProjectVelocity(float /*timestep*/)
{
    PIX_EVENT(ProjectVelocity);

    // pShaderResourceVariables[RENDER_TARGET_VELOCITY0]->SetResource( NULL );
    // SetRenderTarget( RENDER_TARGET_VELOCITY0 );
    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_VELOCITY0]);
    RCache.set_Element(m_SimulationTechnique[SS_Project]);
    // ModulateShaderVariable->SetFloat(1.0f);
    RCache.set_c(strModulate, 1.0f);
    // TechniqueProject->GetPassByIndex(0)->Apply(0);
    m_pGrid->DrawSlices();
    // m_pD3DDevice->OMSetRenderTargets(0, NULL, NULL);
    // pShaderResourceVariables[RENDER_TARGET_VELOCITY0]->SetResource( pRenderTargetShaderViews[RENDER_TARGET_VELOCITY0]
    // );
}

void dx103DFluidManager::RenderFluid(dx103DFluidData& FluidData)
{
    //  return;
    PIX_EVENT(render_fluid);

    //  Bind input texture
    ID3DTexture3D* pT = FluidData.GetTexture(dx103DFluidData::VP_COLOR);
    pRTTextures[RENDER_TARGET_COLOR_IN]->surface_set(pT);
    _RELEASE(pT);

    //  Do rendering
    m_pRenderer->Draw(FluidData);

    //  Unbind input texture
    pRTTextures[RENDER_TARGET_COLOR_IN]->surface_set(0);

    //  Restore render state
    CRenderTarget* pTarget = RImplementation.Target;
    if (!RImplementation.o.dx10_msaa)
        pTarget->u_setrt(pTarget->rt_Generic_0, 0, 0, HW.pBaseZB); // LDR RT
    else
        pTarget->u_setrt(pTarget->rt_Generic_0_r, 0, 0, pTarget->rt_MSAADepth->pZRT); // LDR RT

    RImplementation.rmNormal();
}

void dx103DFluidManager::UpdateObstacles(const dx103DFluidData& FluidData, float timestep)
{
    PIX_EVENT(Fluid_update_obstacles);
    //  Reset data
    float color[4] = {0, 0, 0, 0};
    HW.pContext->ClearRenderTargetView(pRenderTargetViews[RENDER_TARGET_OBSTACLES], color);
    HW.pContext->ClearRenderTargetView(pRenderTargetViews[RENDER_TARGET_OBSTVELOCITY], color);

    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_OBSTACLES], 0);
    RCache.set_RT(pRenderTargetViews[RENDER_TARGET_OBSTVELOCITY], 1);

    m_pObstaclesHandler->ProcessObstacles(FluidData, timestep);

    //  Just reset render targets:
    //  later only rt 0 will be reassigned so rt1
    //  would be bound all the time
    //  Reset to avoid confusion.
    RCache.set_RT(0, 0);
    RCache.set_RT(0, 1);
}

//  Allow real-time config reload
#ifdef DEBUG
void dx103DFluidManager::RegisterFluidData(dx103DFluidData* pData, const xr_string& SectionName)
{
    const size_t iDataNum = m_lstFluidData.size();

    size_t i;

    for (i = 0; i < iDataNum; ++i)
    {
        if (m_lstFluidData[i] == pData)
            break;
    }

    if (iDataNum == i)
    {
        m_lstFluidData.push_back(pData);
        m_lstSectionNames.push_back(SectionName);
    }
    else
    {
        m_lstSectionNames[i] = SectionName;
    }
}

void dx103DFluidManager::DeregisterFluidData(dx103DFluidData* pData)
{
    const size_t iDataNum = m_lstFluidData.size();

    size_t i;

    for (i = 0; i < iDataNum; ++i)
    {
        if (m_lstFluidData[i] == pData)
            break;
    }

    if (i != iDataNum)
    {
        xr_vector<xr_string>::iterator it1 = m_lstSectionNames.begin();
        xr_vector<dx103DFluidData*>::iterator it2 = m_lstFluidData.begin();
        // it1.advance(i);
        it1 += i;
        it2 += i;

        m_lstSectionNames.erase(it1);
        m_lstFluidData.erase(it2);
    }
}

void dx103DFluidManager::UpdateProfiles()
{
    const size_t iDataNum = m_lstFluidData.size();

    for (size_t i = 0; i < iDataNum; ++i)
    {
        m_lstFluidData[i]->ReparseProfile(m_lstSectionNames[i]);
    }
}

#endif //   DEBUG
