#include "stdafx.h"
#include "dx103DFluidRenderer.h"
#include "dx103DFluidBlenders.h"
#include "Layers/xrRenderDX10/dx10BufferUtils.h"
#include "dx103DFluidData.h"

struct VsInput
{
    D3DXVECTOR3 pos;
};

namespace
{
// For render call
shared_str strZNear("ZNear");
shared_str strZFar("ZFar");
shared_str strGridScaleFactor("gridScaleFactor");
shared_str strEyeOnGrid("eyeOnGrid");
shared_str strWorldViewProjection("WorldViewProjection");
shared_str strInvWorldViewProjection("InvWorldViewProjection");
shared_str strRTWidth("RTWidth");
shared_str strRTHeight("RTHeight");

shared_str strDiffuseLight("DiffuseLight");
}

LPCSTR dx103DFluidRenderer::m_pRTNames[RRT_NumRT] = {
    "$user$rayDataTex", "$user$rayDataTexSmall", "$user$rayCastTex", "$user$edgeTex"};

LPCSTR dx103DFluidRenderer::m_pResourceRTNames[RRT_NumRT] = {"rayDataTex", "rayDataTexSmall", "rayCastTex", "edgeTex"};

dx103DFluidRenderer::dx103DFluidRenderer() : m_bInited(false)
{
    RTFormats[RRT_RayDataTex] = D3DFMT_A32B32G32R32F;
    RTFormats[RRT_RayDataTexSmall] = D3DFMT_A32B32G32R32F;
    RTFormats[RRT_RayCastTex] = D3DFMT_A32B32G32R32F;
    RTFormats[RRT_EdgeTex] = D3DFMT_R32F;
}

dx103DFluidRenderer::~dx103DFluidRenderer() { Destroy(); }
void dx103DFluidRenderer::Initialize(int gridWidth, int gridHeight, int gridDepth)
{
    Destroy();

    m_vGridDim[0] = float(gridWidth);
    m_vGridDim[1] = float(gridHeight);
    m_vGridDim[2] = float(gridDepth);

    m_fMaxDim = _max(_max(m_vGridDim[0], m_vGridDim[1]), m_vGridDim[2]);

    // Initialize the grid offset matrix
    {
        // Make a scale matrix to scale the unit-sided box to be unit-length on the
        //  side/s with maximum dimension
        D3DXMATRIX scaleM;
        D3DXMatrixIdentity(&scaleM);
        D3DXMatrixScaling(&scaleM, m_vGridDim[0] / m_fMaxDim, m_vGridDim[1] / m_fMaxDim, m_vGridDim[2] / m_fMaxDim);
        // offset grid to be centered at origin
        D3DXMATRIX translationM;
        D3DXMatrixTranslation(&translationM, -0.5, -0.5, -0.5);

        m_gridMatrix = translationM * scaleM;
        // m_gridMatrix.scale(m_vGridDim[0] / m_fMaxDim, m_vGridDim[1] / m_fMaxDim, m_vGridDim[2] / m_fMaxDim);
        // m_gridMatrix.translate_over(-0.5, -0.5, -0.5);
    }

    InitShaders();
    CreateGridBox();
    CreateScreenQuad();
    CreateJitterTexture();
    CreateHHGGTexture();

    m_bInited = true;
}

void dx103DFluidRenderer::Destroy()
{
    if (!m_bInited)
        return;

    m_JitterTexture = nullptr;
    m_HHGGTexture = nullptr;

    m_GeomQuadVertex = nullptr;
    _RELEASE(m_pQuadVertexBuffer);

    m_GeomGridBox = nullptr;
    _RELEASE(m_pGridBoxVertexBuffer);
    _RELEASE(m_pGridBoxIndexBuffer);

    DestroyShaders();
}

void dx103DFluidRenderer::InitShaders()
{
    {
        CBlender_fluid_raydata Blender;
        ref_shader shader;
        shader.create(&Blender, "null");
        for (size_t i = 0; i < 3; ++i)
            m_RendererTechnique[RS_CompRayData_Back + i] = shader->E[i];
    }

    {
        CBlender_fluid_raycast Blender;
        ref_shader shader;
        shader.create(&Blender, "null");
        for (size_t i = 0; i < 5; ++i)
            m_RendererTechnique[RS_QuadEdgeDetect + i] = shader->E[i];
    }
}

void dx103DFluidRenderer::DestroyShaders()
{
    for (size_t i = 0; i < RS_NumShaders; ++i)
    {
        //	Release shader's element.
        m_RendererTechnique[i] = nullptr;
    }
}

void dx103DFluidRenderer::CreateGridBox()
{
    VsInput vertices[] = {
        {D3DXVECTOR3(0, 0, 0)}, {D3DXVECTOR3(0, 0, 1)}, {D3DXVECTOR3(0, 1, 0)}, {D3DXVECTOR3(0, 1, 1)},
        {D3DXVECTOR3(1, 0, 0)}, {D3DXVECTOR3(1, 0, 1)}, {D3DXVECTOR3(1, 1, 0)}, {D3DXVECTOR3(1, 1, 1)},
    };
    m_iGridBoxVertNum = sizeof(vertices) / sizeof(vertices[0]);

    CHK_DX(dx10BufferUtils::CreateVertexBuffer(&m_pGridBoxVertexBuffer, vertices, sizeof(vertices)));

    // Create index buffer
    u16 indices[] = {
        0, 4, 1, 1, 4, 5, 0, 1, 2, 2, 1, 3, 4, 6, 5, 6, 7, 5, 2, 3, 6, 3, 7, 6, 1, 5, 3, 3, 5, 7, 0, 2, 4, 2, 6, 4};
    m_iGridBoxFaceNum = (sizeof(indices) / sizeof(indices[0])) / 3;

    CHK_DX(dx10BufferUtils::CreateIndexBuffer(&m_pGridBoxIndexBuffer, indices, sizeof(indices)));
    HW.stats_manager.increment_stats(sizeof(indices), enum_stats_buffer_type_index, D3DPOOL_MANAGED);

    // Define the input layout
    static D3DVERTEXELEMENT9 layout[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};

    // Create the input layout
    m_GeomGridBox.create(layout, m_pGridBoxVertexBuffer, m_pGridBoxIndexBuffer);
}

void dx103DFluidRenderer::CreateScreenQuad()
{
    // Create our quad input layout
    static D3DVERTEXELEMENT9 quadlayout[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};

    // Create a screen quad for all render to texture operations
    VsInput svQuad[4];
    svQuad[0].pos = D3DXVECTOR3(-1.0f, 1.0f, 0.0f);
    svQuad[1].pos = D3DXVECTOR3(1.0f, 1.0f, 0.0f);
    svQuad[2].pos = D3DXVECTOR3(-1.0f, -1.0f, 0.0f);
    svQuad[3].pos = D3DXVECTOR3(1.0f, -1.0f, 0.0f);

    CHK_DX(dx10BufferUtils::CreateVertexBuffer(&m_pQuadVertexBuffer, svQuad, sizeof(svQuad)));
    m_GeomQuadVertex.create(quadlayout, m_pQuadVertexBuffer, 0);
}

void dx103DFluidRenderer::CreateJitterTexture()
{
    BYTE data[256 * 256];
    for (int i = 0; i < 256 * 256; i++)
    {
        data[i] = (unsigned char)(rand() / float(RAND_MAX) * 256);
    }

    D3D_TEXTURE2D_DESC desc;
    desc.Width = 256;
    desc.Height = 256;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D_USAGE_DEFAULT;
    desc.BindFlags = D3D_BIND_SHADER_RESOURCE;

    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D_SUBRESOURCE_DATA dataDesc;
    dataDesc.pSysMem = data;
    dataDesc.SysMemPitch = 256;

    ID3DTexture2D* NoiseTexture = nullptr;

    CHK_DX(HW.pDevice->CreateTexture2D(&desc, &dataDesc, &NoiseTexture));

    m_JitterTexture = RImplementation.Resources->_CreateTexture("$user$NVjitterTex");
    m_JitterTexture->surface_set(NoiseTexture);

    _RELEASE(NoiseTexture);
}

namespace
{
// cubic b-spline
float bsW0(float a) { return (1.0f / 6.0f * (-(a * a * a) + (3.0f * a * a) - (3.0f * a) + 1.0f)); }
float bsW1(float a) { return (1.0f / 6.0f * ((3.0f * a * a * a) - (6.0f * a * a) + 4.0f)); }
float bsW2(float a) { return (1.0f / 6.0f * (-(3.0f * a * a * a) + (3.0f * a * a) + (3.0f * a) + 1.0f)); }
float bsW3(float a) { return (1.0f / 6.0f * a * a * a); }
float g0(float a) { return (bsW0(a) + bsW1(a)); }
float g1(float a) { return (bsW2(a) + bsW3(a)); }
float h0texels(float a) { return (1.0f + a - (bsW1(a) / (bsW0(a) + bsW1(a)))); }
float h1texels(float a) { return (1.0f - a + (bsW3(a) / (bsW2(a) + bsW3(a)))); }
}

void dx103DFluidRenderer::CreateHHGGTexture()
{
    static const int iNumSamples = 16;
    float data[4 * iNumSamples];
    D3DXFLOAT16 converted[4 * iNumSamples];

    for (int i = 0; i < iNumSamples; i++)
    {
        float a = i / (float)(iNumSamples - 1);
        data[4 * i] = -h0texels(a);
        data[4 * i + 1] = h1texels(a);
        data[4 * i + 2] = 1.0f - g0(a);
        data[4 * i + 3] = g0(a);
    }

    //	Min value is -1
    //	Max value is +1
    D3DXFloat32To16Array(converted, data, 4 * iNumSamples);

    D3D_TEXTURE1D_DESC desc;
    desc.Width = iNumSamples;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.Usage = D3D_USAGE_DEFAULT;
    desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D_SUBRESOURCE_DATA dataDesc;
    dataDesc.pSysMem = converted;
    dataDesc.SysMemPitch = sizeof(converted);

    ID3DTexture1D* HHGGTexture = nullptr;

    CHK_DX(HW.pDevice->CreateTexture1D(&desc, &dataDesc, &HHGGTexture));

    m_HHGGTexture = RImplementation.Resources->_CreateTexture("$user$NVHHGGTex");
    m_HHGGTexture->surface_set(HHGGTexture);

    _RELEASE(HHGGTexture);
}

void dx103DFluidRenderer::SetScreenSize(int width, int height) { CreateRayDataResources(width, height); }
void dx103DFluidRenderer::CalculateRenderTextureSize(int screenWidth, int screenHeight)
{
    int maxProjectedSide = int(3.0 * _sqrt(3.0) * m_fMaxDim);
    int maxScreenDim = _max(screenWidth, screenHeight);

    float screenAspectRatio = ((float)screenWidth) / screenHeight;

    if (maxScreenDim > maxProjectedSide)
    {
        if (screenHeight > screenWidth)
        {
            m_iRenderTextureHeight = maxProjectedSide;
            m_iRenderTextureWidth = (int)(screenAspectRatio * maxProjectedSide);
        }
        else
        {
            m_iRenderTextureWidth = maxProjectedSide;
            m_iRenderTextureHeight = (int)((1.0f / screenAspectRatio) * maxProjectedSide);
        }
    }
    else
    {
        m_iRenderTextureWidth = screenWidth;
        m_iRenderTextureHeight = screenHeight;
    }
}

void dx103DFluidRenderer::CreateRayDataResources(int width, int height)
{
    // find a good resolution for raycasting purposes
    CalculateRenderTextureSize(width, height);

    RT[0] = nullptr;
    RT[0].create(m_pRTNames[0], width, height, RTFormats[0]);

    for (size_t i = 1; i < RRT_NumRT; ++i)
    {
        RT[i] = nullptr;
        RT[i].create(m_pRTNames[i], m_iRenderTextureWidth, m_iRenderTextureHeight, RTFormats[i]);
    }
}

void dx103DFluidRenderer::Draw(const dx103DFluidData& FluidData)
{
    //	We don't need ZB anyway
    RCache.set_ZB(nullptr);

    CRenderTarget* pTarget = RImplementation.Target;
    const dx103DFluidData::Settings& VolumeSettings = FluidData.GetSettings();
    const bool bRenderFire = (VolumeSettings.m_SimulationType == dx103DFluidData::ST_FIRE);

    FogLighting LightData;

    CalculateLighting(FluidData, LightData);

    //	Set shader element to set up all necessary constants to constant buffer
    //	If you change constant buffer layout make sure this hack works ok.
    RCache.set_Element(m_RendererTechnique[RS_CompRayData_Back]);

    // Ray cast and render to a temporary buffer
    //=========================================================================

    // Compute the ray data required by the raycasting pass below.
    //  This function will render to a buffer of float4 vectors, where
    //  xyz is starting position of the ray in grid space
    //  w is the length of the ray in view space
    ComputeRayData(FluidData);

    // Do edge detection on this image to find any
    //  problematic areas where we need to raycast at higher resolution
    ComputeEdgeTexture(FluidData);

    // Raycast into the temporary render target:
    //  raycasting is done at the smaller resolution, using a fullscreen quad
    FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    HW.pContext->ClearRenderTargetView(RT[RRT_RayCastTex]->pRT, ColorRGBA);

    pTarget->u_setrt(RT[RRT_RayCastTex], nullptr, nullptr, nullptr); // LDR RT

    RImplementation.rmNormal();

    if (bRenderFire)
        RCache.set_Element(m_RendererTechnique[RS_QuadRaycastFire]);
    else
        RCache.set_Element(m_RendererTechnique[RS_QuadRaycastFog]);

    PrepareCBuffer(FluidData, m_iRenderTextureWidth, m_iRenderTextureHeight);
    DrawScreenQuad();

    // Render to the back buffer sampling from the raycast texture that we just created
    //  If and edge was detected at the current pixel we will raycast again to avoid
    //  smoke aliasing artifacts at scene edges
    if (!RImplementation.o.dx10_msaa)
        pTarget->u_setrt(pTarget->rt_Generic_0, nullptr, nullptr, HW.pBaseZB); // LDR RT
    else
        pTarget->u_setrt(pTarget->rt_Generic_0_r, nullptr, nullptr, pTarget->rt_MSAADepth->pZRT); // LDR RT

    if (bRenderFire)
        RCache.set_Element(m_RendererTechnique[RS_QuadRaycastCopyFire]);
    else
        RCache.set_Element(m_RendererTechnique[RS_QuadRaycastCopyFog]);

    RImplementation.rmNormal();

    PrepareCBuffer(FluidData, Device.dwWidth, Device.dwHeight);
    RCache.set_c(strDiffuseLight, LightData.m_vLightIntencity.x, LightData.m_vLightIntencity.y,
        LightData.m_vLightIntencity.z, 1.0f);

    DrawScreenQuad();
}

void dx103DFluidRenderer::ComputeRayData(const dx103DFluidData &FluidData)
{
    // Clear the color buffer to 0
    FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    HW.pContext->ClearRenderTargetView(RT[RRT_RayDataTex]->pRT, ColorRGBA);

    CRenderTarget* pTarget = RImplementation.Target;
    pTarget->u_setrt(RT[RRT_RayDataTex], nullptr, nullptr, nullptr); // LDR RT
    RCache.set_Element(m_RendererTechnique[RS_CompRayData_Back]);

    RImplementation.rmNormal();

    PrepareCBuffer(FluidData, Device.dwWidth, Device.dwHeight);

    // Render volume back faces
    // We output xyz=(0,-1,0) and w=min(sceneDepth, boxDepth)
    DrawBox();

    // Render volume front faces using subtractive blending
    // We output xyz="position in grid space" and w=boxDepth,
    //  unless the pixel is occluded by the scene, in which case we output xyzw=(1,0,0,0)
    pTarget->u_setrt(RT[RRT_RayDataTex], nullptr, nullptr, nullptr); // LDR RT
    RCache.set_Element(m_RendererTechnique[RS_CompRayData_Front]);
    PrepareCBuffer(FluidData, Device.dwWidth, Device.dwHeight);

    // Render
    DrawBox();
}

void dx103DFluidRenderer::ComputeEdgeTexture(const dx103DFluidData &FluidData)
{
    CRenderTarget* pTarget = RImplementation.Target;
    pTarget->u_setrt(RT[RRT_RayDataTexSmall], nullptr, nullptr, nullptr); // LDR RT
    RCache.set_Element(m_RendererTechnique[RS_QuadDownSampleRayDataTexture]);

    // First setup viewport to match the size of the destination low-res texture
    RImplementation.rmNormal();

    PrepareCBuffer(FluidData, m_iRenderTextureWidth, m_iRenderTextureHeight);

    // Downsample the rayDataTexture to a new small texture, simply using point sample (no filtering)
    DrawScreenQuad();

    // Create an edge texture, performing edge detection on 'rayDataTexSmall'
    pTarget->u_setrt(RT[RRT_EdgeTex], nullptr, nullptr, nullptr); // LDR RT
    RCache.set_Element(m_RendererTechnique[RS_QuadEdgeDetect]);
    PrepareCBuffer(FluidData, m_iRenderTextureWidth, m_iRenderTextureHeight);

    // Render
    DrawScreenQuad();
}

void dx103DFluidRenderer::DrawScreenQuad()
{
    RCache.set_Geometry(m_GeomQuadVertex);
    RCache.Render(D3DPT_TRIANGLESTRIP, 0, 2);
}

void dx103DFluidRenderer::DrawBox()
{
    RCache.set_Geometry(m_GeomGridBox);
    RCache.Render(D3DPT_TRIANGLELIST, 0, 0, m_iGridBoxVertNum, 0, m_iGridBoxFaceNum);
}

void dx103DFluidRenderer::CalculateLighting(const dx103DFluidData& FluidData, FogLighting& LightData)
{
    m_lstRenderables.clear();

    LightData.Reset();

    const dx103DFluidData::Settings& VolumeSettings = FluidData.GetSettings();

    Fvector4 hemi_color = g_pGamePersistent->Environment().CurrentEnv->hemi_color;
    // hemi_color.mul(0.2f);
    hemi_color.mul(VolumeSettings.m_fHemi);
    LightData.m_vLightIntencity.set(hemi_color.x, hemi_color.y, hemi_color.z);
    LightData.m_vLightIntencity.add(g_pGamePersistent->Environment().CurrentEnv->ambient);

    const Fmatrix& Transform = FluidData.GetTransform();

    Fbox box;
    box.vMin = Fvector3().set(-0.5f, -0.5f, -0.5f);
    box.vMax = Fvector3().set(0.5f, 0.5f, 0.5f);
    box.xform(Transform);
    Fvector3 center;
    Fvector3 size;
    box.getcenter(center);
    box.getradius(size);

    // Traverse object database
    g_SpatialSpace->q_box(m_lstRenderables,
        0, // ISpatial_DB::O_ORDERED,
        STYPE_LIGHTSOURCE, center, size);

    // Determine visibility for dynamic part of scene
    for (ISpatial* spatial : m_lstRenderables)
    {
        // Light
        light* pLight = (light*)spatial->dcast_Light();
        VERIFY(pLight);

        if (pLight->flags.bStatic)
            continue;

        float d = pLight->position.distance_to(Transform.c);

        float R = pLight->range + _max(size.x, _max(size.y, size.z));
        if (d >= R)
            continue;

        Fvector3 LightIntencity;

        LightIntencity.set(pLight->color.r, pLight->color.g, pLight->color.b);

        // LightIntencity.mul(0.5f);

        // if (!pLight->flags.bStatic)
        //	LightIntencity.mul(0.5f);

        float r = pLight->range;
        float a = clampr(1.f - d / (r + EPS), 0.f, 1.f) * (pLight->flags.bStatic ? 1.f : 2.f);

        LightIntencity.mul(a);

        LightData.m_vLightIntencity.add(LightIntencity);
    }

    // LightData.m_vLightIntencity.set( 1.0f, 0.5f, 0.0f);
    // LightData.m_vLightIntencity.set( 1.0f, 1.0f, 1.0f);
}

void dx103DFluidRenderer::PrepareCBuffer(const dx103DFluidData &FluidData, u32 RTWidth, u32 RTHeight)
{
    const Fmatrix &transform = FluidData.GetTransform();
    RCache.set_xform_world(transform);

    // The near and far planes are used to unproject the scene's z-buffer values
    RCache.set_c(strZNear, VIEWPORT_NEAR);
    RCache.set_c(strZFar, g_pGamePersistent->Environment().CurrentEnv->far_plane);

    D3DXMATRIX gridWorld;
    gridWorld = *(D3DXMATRIX*)&transform;
    D3DXMATRIX View;
    View = *(D3DXMATRIX*)&RCache.xforms.m_v;
    D3DXMATRIX WorldView = gridWorld * View;

    // The length of one of the axis of the worldView matrix is the length of longest side of the box
    //  in view space. This is used to convert the length of a ray from view space to grid space.
    D3DXVECTOR3 worldXaxis = D3DXVECTOR3(WorldView._11, WorldView._12, WorldView._13);
    float worldScale = D3DXVec3Length(&worldXaxis);
    RCache.set_c(strGridScaleFactor, worldScale);

    // We prepend the current world matrix with this other matrix which adds an offset (-0.5, -0.5, -0.5)
    //  and scale factors to account for unequal number of voxels on different sides of the volume box. 
    // This is because we want to preserve the aspect ratio of the original simulation grid when 
    //  raytracing through it.
    WorldView = m_gridMatrix * WorldView;

    // worldViewProjection is used to transform the volume box to screen space
    D3DXMATRIX WorldViewProjection;
    D3DXMATRIX Projection;
    Projection = *(D3DXMATRIX*)&RCache.xforms.m_p;
    WorldViewProjection = WorldView * Projection;
    RCache.set_c(strWorldViewProjection, *(Fmatrix*)&WorldViewProjection);

    // invWorldViewProjection is used to transform positions in the "near" plane into grid space
    D3DXMATRIX InvWorldViewProjection;
    D3DXMatrixInverse((D3DXMATRIX*)&InvWorldViewProjection, nullptr, (D3DXMATRIX*)&WorldViewProjection);
    RCache.set_c(strInvWorldViewProjection, *(Fmatrix*)&InvWorldViewProjection);

    // Compute the inverse of the worldView matrix 
    D3DXMATRIX WorldViewInv;
    D3DXMatrixInverse((D3DXMATRIX*)&WorldViewInv, nullptr, (D3DXMATRIX*)&WorldView);
    // Compute the eye's position in "grid space" (the 0-1 texture coordinate cube)
    D3DXVECTOR4 EyeInGridSpace;
    D3DXVECTOR3 Origin(0,0,0);
    D3DXVec3Transform((D3DXVECTOR4*)&EyeInGridSpace, (D3DXVECTOR3*)&Origin, (D3DXMATRIX*)&WorldViewInv);
    RCache.set_c(strEyeOnGrid, *(Fvector4*)&EyeInGridSpace);

    RCache.set_c(strRTWidth, (float)RTWidth);
    RCache.set_c(strRTHeight, (float)RTHeight);
}
