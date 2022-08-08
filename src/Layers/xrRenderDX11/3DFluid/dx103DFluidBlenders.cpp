#include "stdafx.h"
#include "dx103DFluidBlenders.h"

#include "dx103DFluidManager.h"
#include "dx103DFluidRenderer.h"

namespace
{
// Volume texture width
class cl_textureWidth : public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        float tW = (float)FluidManager.GetTextureWidth();
        RCache.set_c(C, tW);
    }
};
static cl_textureWidth binder_textureWidth;

// Volume texture height
class cl_textureHeight : public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        float tH = (float)FluidManager.GetTextureHeight();
        RCache.set_c(C, tH);
    }
};
static cl_textureHeight binder_textureHeight;

// Volume texture depth
class cl_textureDepth : public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        float tD = (float)FluidManager.GetTextureDepth();
        RCache.set_c(C, tD);
    }
};
static cl_textureDepth binder_textureDepth;

class cl_gridDim : public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        float tW = (float)FluidManager.GetTextureWidth();
        float tH = (float)FluidManager.GetTextureHeight();
        float tD = (float)FluidManager.GetTextureDepth();
        RCache.set_c(C, tW, tH, tD, 0.0f);
    }
};
static cl_gridDim binder_gridDim;

class cl_recGridDim : public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        float tW = (float)FluidManager.GetTextureWidth();
        float tH = (float)FluidManager.GetTextureHeight();
        float tD = (float)FluidManager.GetTextureDepth();
        RCache.set_c(C, 1.0f / tW, 1.0f / tH, 1.0f / tD, 0.0f);
    }
};
static cl_recGridDim binder_recGridDim;

class cl_maxDim : public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        int tW = FluidManager.GetTextureWidth();
        int tH = FluidManager.GetTextureHeight();
        int tD = FluidManager.GetTextureDepth();
        float tMax = (float)_max(tW, _max(tH, tD));
        RCache.set_c(C, (float)tMax);
    }
};
static cl_maxDim binder_maxDim;

/*
//  decay simulation option
class cl_decay		: public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        float fDecay = FluidManager.GetDecay();
        RCache.set_c( C, fDecay );
    }
};
static cl_decay		binder_decay;

//  decay simulation ImpulseSize
class cl_impulseSize		: public R_constant_setup
{
    virtual void setup(R_constant* C)
    {
        float fIS = FluidManager.GetImpulseSize();
        RCache.set_c( C, fIS );
    }
};
static cl_impulseSize		binder_impulseSize;
*/

void BindConstants(CBlender_Compile& C)
{
    //	Bind constants here

    //	TextureWidthShaderVariable = pEffect->GetVariableByName( "textureWidth")->AsScalar();
    C.r_Constant("textureWidth", &binder_textureWidth);
    //	TextureHeightShaderVariable = pEffect->GetVariableByName( "textureHeight")->AsScalar();
    C.r_Constant("textureHeight", &binder_textureHeight);
    //	TextureDepthShaderVariable = pEffect->GetVariableByName( "textureDepth")->AsScalar();
    C.r_Constant("textureDepth", &binder_textureDepth);

    //	Renderer constants
    // D3DXVECTOR3 recGridDim(1.0f/gridDim[0], 1.0f/gridDim[1], 1.0f/gridDim[2]);
    // pEffect->GetVariableByName("gridDim")->AsVector()->SetFloatVector(gridDim);
    C.r_Constant("gridDim", &binder_gridDim);
    // pEffect->GetVariableByName("recGridDim")->AsVector()->SetFloatVector(recGridDim);
    C.r_Constant("recGridDim", &binder_recGridDim);
    // pEffect->GetVariableByName("maxGridDim")->AsScalar()->SetFloat(maxDim);
    C.r_Constant("maxGridDim", &binder_maxDim);

    //	Each technique should set up these variables itself
    /*
    // For project, advect
    //ModulateShaderVariable = pEffect->GetVariableByName( "modulate")->AsScalar();
    //C.r_Constant( "modulate",		&binder_decay);

    // For gaussian
    // Used to apply external impulse
    //ImpulseSizeShaderVariable = pEffect->GetVariableByName( "size")->AsScalar();
    //C.r_Constant( "size",		&binder_impulseSize);
    //	Setup manually by technique
    //ImpulseCenterShaderVariable = pEffect->GetVariableByName( "center")->AsVector();
    //SplatColorShaderVariable = pEffect->GetVariableByName( "splatColor")->AsVector();

    // For confinement
    EpsilonShaderVariable = pEffect->GetVariableByName( "epsilon")->AsScalar();
    // For confinement, advect
    TimeStepShaderVariable = pEffect->GetVariableByName( "timestep")->AsScalar();
    // For advect BFECC
    ForwardShaderVariable = pEffect->GetVariableByName( "forward")->AsScalar();
    HalfVolumeDimShaderVariable = pEffect->GetVariableByName( "halfVolumeDim")->AsVector();


    // For render call
    //DrawTextureShaderVariable = pEffect->GetVariableByName( "textureNumber")->AsScalar();
    */
}
void SetupSamplers(CBlender_Compile& C)
{
    int smp = C.r_dx10Sampler("samPointClamp");
    if (smp != u32(-1))
    {
        C.i_Address(smp, D3DTADDRESS_CLAMP);
        C.i_Filter(smp, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT);
    }

    smp = C.r_dx10Sampler("samLinear");
    if (smp != u32(-1))
    {
        C.i_Address(smp, D3DTADDRESS_CLAMP);
        C.i_Filter(smp, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }

    smp = C.r_dx10Sampler("samLinearClamp");
    if (smp != u32(-1))
    {
        C.i_Address(smp, D3DTADDRESS_CLAMP);
        C.i_Filter(smp, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }

    smp = C.r_dx10Sampler("samRepeat");
    if (smp != u32(-1))
    {
        C.i_Address(smp, D3DTADDRESS_WRAP);
        C.i_Filter(smp, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR);
    }
}
void SetupTextures(CBlender_Compile& C)
{
    LPCSTR* TNames = FluidManager.GetEngineTextureNames();
    LPCSTR* RNames = FluidManager.GetShaderTextureNames();

    for (int i = 0; i < dx103DFluidManager::NUM_RENDER_TARGETS; ++i)
        C.r_dx10Texture(RNames[i], TNames[i]);

    //	Renderer
    C.r_dx10Texture("sceneDepthTex", r2_RT_P);
    // C.r_dx10Texture("colorTex", "Texture_color");
    C.r_dx10Texture("colorTex", TNames[dx103DFluidManager::RENDER_TARGET_COLOR_IN]);
    C.r_dx10Texture("jitterTex", "$user$NVjitterTex");

    C.r_dx10Texture("HHGGTex", "$user$NVHHGGTex");

    C.r_dx10Texture("fireTransferFunction", "internal\\internal_fireTransferFunction");

    TNames = dx103DFluidRenderer::GetRTNames();
    RNames = dx103DFluidRenderer::GetResourceRTNames();

    for (int i = 0; i < dx103DFluidRenderer::RRT_NumRT; ++i)
        C.r_dx10Texture(RNames[i], TNames[i]);
}
} //	namespace

void CBlender_fluid_advect::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // Advect
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect", false, FALSE, FALSE, FALSE);
        break;
    case 1: // AdvectBFECC
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_bfecc", false, FALSE, FALSE, FALSE);
        break;
    case 2: // AdvectTemp
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_temp", false, FALSE, FALSE, FALSE);
        break;
    case 3: // AdvectBFECCTemp
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_bfecc_temp", false, FALSE, FALSE, FALSE);
        break;
    case 4: // AdvectVel
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_vel", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound befor r_End()
    C.r_End();
}

void CBlender_fluid_advect_velocity::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // AdvectVel
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_vel", false, FALSE, FALSE, FALSE);
        break;
    case 1: // AdvectVelGravity
        C.r_Pass("fluid_grid", "fluid_array", "fluid_advect_vel_g", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound befor r_End()
    C.r_End();
}

void CBlender_fluid_simulate::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // Vorticity
        C.r_Pass("fluid_grid", "fluid_array", "fluid_vorticity", false, FALSE, FALSE, FALSE);
        break;
    case 1: // Confinement
        //	Use additive blending
        C.r_Pass(
            "fluid_grid", "fluid_array", "fluid_confinement", false, FALSE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE);
        break;
    case 2: // Divergence
        C.r_Pass("fluid_grid", "fluid_array", "fluid_divergence", false, FALSE, FALSE, FALSE);
        break;
    case 3: // Jacobi
        C.r_Pass("fluid_grid", "fluid_array", "fluid_jacobi", false, FALSE, FALSE, FALSE);
        break;
    case 4: // Project
        C.r_Pass("fluid_grid", "fluid_array", "fluid_project", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_obst::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // ObstStaticBox
        //	AABB
        // C.r_Pass	("fluid_grid", "fluid_array", "fluid_obststaticbox", false,FALSE,FALSE,FALSE);
        //	OOBB
        C.r_Pass("fluid_grid_oobb", "fluid_array_oobb", "fluid_obst_static_oobb", false, FALSE, FALSE, FALSE);
        break;
    case 1: // ObstDynBox
        //	OOBB
        C.r_Pass("fluid_grid_dyn_oobb", "fluid_array_dyn_oobb", "fluid_obst_dynamic_oobb", false, FALSE, FALSE, FALSE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_emitter::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // ET_SimpleGausian
        C.r_Pass("fluid_grid", "fluid_array", "fluid_gaussian", false, FALSE, FALSE, TRUE, D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA);
        C.RS.SetRS(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);
        C.RS.SetRS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
        break;
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_obstdraw::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // DrawTexture
        C.r_Pass("fluid_grid", "null", "fluid_draw_texture", false, FALSE, FALSE, FALSE);
        break;
        //		TechniqueDrawWhiteTriangles = pEffect->GetTechniqueByName( "DrawWhiteTriangles" );
        //		TechniqueDrawWhiteLines = pEffect->GetTechniqueByName( "DrawWhiteLines" );
        //		TechniqueDrawBox = pEffect->GetTechniqueByName( "DrawBox" );
    }

    C.r_CullMode(D3DCULL_NONE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_raydata::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // CompRayData_Back
        C.r_Pass("fluid_raydata_back", "null", "fluid_raydata_back", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CW); //	Front
        // C.r_CullMode(D3DCULL_CCW);	//	Front
        break;
    case 1: // CompRayData_Front
        C.r_Pass("fluid_raydata_front", "null", "fluid_raydata_front", false, FALSE, FALSE, TRUE, D3DBLEND_ONE,
            D3DBLEND_ONE);
        // RS.SetRS(D3DRS_SRCBLENDALPHA,		bABlend?abSRC:D3DBLEND_ONE	);
        //	We need different blend arguments for color and alpha
        //	One Zero for color
        //	One One for alpha
        //	so patch dest color.
        //	Note: You can't set up dest blend to zero in r_pass
        //	since r_pass would disable blend if src=one and blend - zero.
        C.RS.SetRS(D3DRS_DESTBLEND, D3DBLEND_ZERO);

        C.RS.SetRS(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT); // DST - SRC
        C.RS.SetRS(D3DRS_BLENDOPALPHA, D3DBLENDOP_REVSUBTRACT); // DST - SRC

        C.r_CullMode(D3DCULL_CCW); //	Back
        // C.r_CullMode(D3DCULL_CW);	//	Back
        break;
    case 2: // QuadDownSampleRayDataTexture
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raydatacopy_quad", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    }

    // C.PassSET_ZB(FALSE,FALSE);

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}

void CBlender_fluid_raycast::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    switch (C.iElement)
    {
    case 0: // QuadEdgeDetect
        C.r_Pass("fluid_edge_detect", "null", "fluid_edge_detect", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_NONE); //	Back
        break;
    case 1: // QuadRaycastFog
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycast_quad", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    case 2: // QuadRaycastCopyFog
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycastcopy_quad", false, FALSE, FALSE, TRUE, D3DBLEND_SRCALPHA,
            D3DBLEND_INVSRCALPHA);
        C.r_ColorWriteEnable(true, true, true, false);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    case 3: // QuadRaycastFire
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycast_quad_fire", false, FALSE, FALSE, FALSE);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    case 4: // QuadRaycastCopyFire
        C.r_Pass("fluid_raycast_quad", "null", "fluid_raycastcopy_quad_fire", false, FALSE, FALSE, TRUE,
            D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
        C.r_ColorWriteEnable(true, true, true, false);
        C.r_CullMode(D3DCULL_CCW); //	Back
        break;
    }

    BindConstants(C);
    SetupSamplers(C);
    SetupTextures(C);

    //	Constants must be bound before r_End()
    C.r_End();
}
