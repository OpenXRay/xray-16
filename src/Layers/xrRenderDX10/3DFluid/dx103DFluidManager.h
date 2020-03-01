#ifndef dx103DFluidManager_included
#define dx103DFluidManager_included
#pragma once

class dx103DFluidData;
class dx103DFluidGrid;
class dx103DFluidObstacles;
class dx103DFluidEmitters;

#include "dx103DFluidRenderer.h"

class dx103DFluidManager
{
public:
    enum RENDER_TARGET
    {
        RENDER_TARGET_VELOCITY1 = 0,
        RENDER_TARGET_COLOR, //	Swap with object's
        RENDER_TARGET_OBSTACLES,
        RENDER_TARGET_OBSTVELOCITY,
        RENDER_TARGET_TEMPSCALAR,
        RENDER_TARGET_TEMPVECTOR,
        NUM_OWN_RENDER_TARGETS, //	Owns render targets only upt to this id.
        RENDER_TARGET_VELOCITY0 = NUM_OWN_RENDER_TARGETS, //	For textures generated from local data
        RENDER_TARGET_PRESSURE,
        RENDER_TARGET_COLOR_IN,
        NUM_RENDER_TARGETS
    };

public:
    dx103DFluidManager();
    ~dx103DFluidManager();

    //		Manager setup
    void Initialize(int width, int height, int depth);
    void Destroy();
    void SetScreenSize(int width, int height)
    {
        if (m_bInited)
            m_pRenderer->SetScreenSize(width, height);
    }

    //		Interface for fluid volume
    void Update(dx103DFluidData& FluidData, float timestep);
    void RenderFluid(dx103DFluidData& FluidData);

    //		Interface for blenders
    int GetTextureWidth() const { return m_iTextureWidth; }
    int GetTextureHeight() const { return m_iTextureHeight; }
    int GetTextureDepth() const { return m_iTextureDepth; }
    //	float	GetDecay() { return m_fDecay; }
    float GetImpulseSize() const { return m_fImpulseSize; }
    static LPCSTR* GetEngineTextureNames() { return m_pEngineTextureNames; }
    static LPCSTR* GetShaderTextureNames() { return m_pShaderTextureNames; }
//	Allow real-time config reload
#ifdef DEBUG
    void RegisterFluidData(dx103DFluidData* pData, const xr_string& SectionName);
    void DeregisterFluidData(dx103DFluidData* pData);
    void UpdateProfiles();
#endif //	DEBUG

private:
    enum SimulationShader
    {
        SS_Advect = 0,
        SS_AdvectBFECC,
        SS_AdvectTemp,
        SS_AdvectBFECCTemp,

        SS_AdvectVel,
        SS_AdvectVelGravity,

        SS_Vorticity,
        SS_Confinement,
        SS_Divergence,
        SS_Jacobi,
        SS_Project,

        SS_NumShaders
    };

private:
    //		Initialization
    void InitShaders();
    void DestroyShaders();

    void PrepareTexture(size_t rtIndex);
    void CreateRTTextureAndViews(size_t rtIndex, D3D_TEXTURE3D_DESC TexDesc);
    void DestroyRTTextureAndViews(size_t rtIndex);

    void Reset();

    //		Simlulation data initialisation
    void AttachFluidData(dx103DFluidData& FluidData);
    void DetachAndSwapFluidData(dx103DFluidData& FluidData);

    //	Simulation code
    void AdvectColorBFECC(float timestep, bool bTeperature);
    void AdvectColor(float timestep, bool bTeperature);
    void AdvectVelocity(float timestep, float fGravity);
    void ApplyVorticityConfinement(float timestep);
    void ApplyExternalForces(const dx103DFluidData& FluidData, float timestep);
    void ComputeVelocityDivergence(float timestep);
    void ComputePressure(float timestep);
    void ProjectVelocity(float timestep);
    void UpdateObstacles(const dx103DFluidData& FluidData, float timestep);

private:
    bool m_bInited;

    DXGI_FORMAT RenderTargetFormats[NUM_RENDER_TARGETS];
    ID3DRenderTargetView* pRenderTargetViews[NUM_RENDER_TARGETS];
    ref_texture pRTTextures[NUM_RENDER_TARGETS];
    static LPCSTR m_pEngineTextureNames[NUM_RENDER_TARGETS];
    static LPCSTR m_pShaderTextureNames[NUM_RENDER_TARGETS];

    ref_selement m_SimulationTechnique[SS_NumShaders];

    //
    dx103DFluidGrid* m_pGrid;
    dx103DFluidRenderer* m_pRenderer;
    dx103DFluidObstacles* m_pObstaclesHandler;
    dx103DFluidEmitters* m_pEmittersHandler;

    //	Simulation options
    int m_nIterations;
    bool m_bUseBFECC;
    float m_fSaturation;
    bool m_bAddDensity;
    float m_fImpulseSize;
    float m_fConfinementScale;
    float m_fDecay;

    //	Volume textures dimensions
    int m_iTextureWidth;
    int m_iTextureHeight;
    int m_iTextureDepth;

//	Allow real-time config reload
#ifdef DEBUG
    xr_vector<xr_string> m_lstSectionNames;
    xr_vector<dx103DFluidData*> m_lstFluidData;
#endif //	DEBUG

//	Allow real-time config reload
#ifdef DEBUG
#endif //	DEBUG
};

extern dx103DFluidManager FluidManager;

#endif //	dx103DFluidManager_included
