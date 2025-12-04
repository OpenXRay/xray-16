#include "stdafx.h"
#include "dx113DFluidManager.h"
#include "dx113DFluidBlenders.h"
#include "dx113DFluidData.h"
#include "dx113DFluidGrid.h"
#include "dx113DFluidRenderer.h"
#include "dx113DFluidObstacles.h"
#include "dx113DFluidEmitters.h"

namespace xray::render::RENDER_NAMESPACE
{
dx113DFluidManager FluidManager;

LPCSTR dx113DFluidManager::m_pEngineTextureNames[NUM_RENDER_TARGETS] = {
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

LPCSTR dx113DFluidManager::m_pShaderTextureNames[NUM_RENDER_TARGETS] = {
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

dx113DFluidManager::dx113DFluidManager()
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

dx113DFluidManager::~dx113DFluidManager() { Destroy(); }
void dx113DFluidManager::Initialize(int width, int height, int depth)
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

    m_pGrid = xr_new<dx113DFluidGrid>();

    m_pGrid->Initialize(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);

    m_pRenderer = xr_new<dx113DFluidRenderer>();
    m_pRenderer->Initialize(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth);

    m_pObstaclesHandler = xr_new<dx113DFluidObstacles>(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth, m_pGrid);

    m_pEmittersHandler = xr_new<dx113DFluidEmitters>(m_iTextureWidth, m_iTextureHeight, m_iTextureDepth, m_pGrid);

    m_bInited = true;

    //  Create and grid and renderer here
    // grid = new Grid( m_pD3DDevice );
    // renderer = new VolumeRenderer( m_pD3DDevice );
}

void dx113DFluidManager::Destroy()
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

void dx113DFluidManager::InitShaders()
{
}

void dx113DFluidManager::DestroyShaders()
{
}

void dx113DFluidManager::PrepareTexture(size_t rtIndex)
{
}

void dx113DFluidManager::CreateRTTextureAndViews(size_t rtIndex, D3D_TEXTURE3D_DESC TexDesc)
{
}
void dx113DFluidManager::DestroyRTTextureAndViews(size_t rtIndex)
{
}

void dx113DFluidManager::Reset()
{
}

void dx113DFluidManager::Update(dx113DFluidData& FluidData, float timestep)
{
}

void dx113DFluidManager::AttachFluidData(dx113DFluidData& FluidData)
{
}

void dx113DFluidManager::DetachAndSwapFluidData(dx113DFluidData& FluidData)
{
}

void dx113DFluidManager::AdvectColorBFECC(float timestep, bool bTeperature)
{
}

void dx113DFluidManager::AdvectColor(float timestep, bool bTeperature)
{
}

void dx113DFluidManager::AdvectVelocity(float timestep, float fGravity)
{
}

void dx113DFluidManager::ApplyVorticityConfinement(float timestep)
{
}

void dx113DFluidManager::ApplyExternalForces(const dx113DFluidData& FluidData, float /*timestep*/)
{
}

void dx113DFluidManager::ComputeVelocityDivergence(float /*timestep*/)
{
}

void dx113DFluidManager::ComputePressure(float /*timestep*/)
{
}

void dx113DFluidManager::ProjectVelocity(float /*timestep*/)
{
}

void dx113DFluidManager::RenderFluid(dx113DFluidData& FluidData)
{
}

void dx113DFluidManager::UpdateObstacles(const dx113DFluidData& FluidData, float timestep)
{
}

#ifndef MASTER_GOLD
// Allow real-time config reload
void dx113DFluidManager::RegisterFluidData(dx113DFluidData* pData, const xr_string& SectionName)
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

void dx113DFluidManager::DeregisterFluidData(dx113DFluidData* pData)
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
        xr_vector<dx113DFluidData*>::iterator it2 = m_lstFluidData.begin();
        // it1.advance(i);
        it1 += i;
        it2 += i;

        m_lstSectionNames.erase(it1);
        m_lstFluidData.erase(it2);
    }
}

void dx113DFluidManager::UpdateProfiles()
{
    const size_t iDataNum = m_lstFluidData.size();

    for (size_t i = 0; i < iDataNum; ++i)
    {
        m_lstFluidData[i]->ReparseProfile(m_lstSectionNames[i]);
    }
}

#endif // !MASTER_GOLD
} // namespace xray::render::RENDER_NAMESPACE
