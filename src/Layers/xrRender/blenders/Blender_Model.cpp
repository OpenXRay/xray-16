#include "stdafx.h"
#pragma hdrstop

#include "Blender_Model.h"

CBlender_Model::CBlender_Model()
{
    description.CLS = B_MODEL;
    description.version = 2;
    oTessellation.Count = 4;
    oTessellation.IDselected = 0;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
}

void CBlender_Model::Save(IWriter& fs)
{
    IBlender::Save(fs);
    xrPWRITE_PROP(fs, "Use alpha-channel", xrPID_BOOL, oBlend);
    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
    xrP_TOKEN::Item I;
    xrPWRITE_PROP(fs, "Tessellation", xrPID_TOKEN, oTessellation);
    I.ID = 0;
    xr_strcpy(I.str, "NO_TESS");
    fs.w(&I, sizeof(I));
    I.ID = 1;
    xr_strcpy(I.str, "TESS_PN");
    fs.w(&I, sizeof(I));
    I.ID = 2;
    xr_strcpy(I.str, "TESS_HM");
    fs.w(&I, sizeof(I));
    I.ID = 3;
    xr_strcpy(I.str, "TESS_PN+HM");
    fs.w(&I, sizeof(I));
}

void CBlender_Model::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    switch (version)
    {
    case 0:
        oAREF.value = 32;
        oAREF.min = 0;
        oAREF.max = 255;
        oBlend.value = FALSE;
        break;
    case 1:
    default:
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
        xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);
        break;
    }
    if (version > 1)
    {
        xrPREAD_PROP(fs, xrPID_TOKEN, oTessellation);
    }
}

LPCSTR CBlender_Model::getComment()
{
    return "MODEL: Default";
}

void CBlender_Model::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bFFP)
        CompileFFP(C);
    else
        CompileProgrammable(C);
}

void CBlender_Model::CompileFFP(CBlender_Compile& C) const
{
    if (!ps_r1_flags.is_any(R1FLAG_FFP_LIGHTMAPS | R1FLAG_DLIGHTS))
    {
        C.PassBegin();
        {
            C.PassSET_ZB(TRUE, oBlend.value && (oAREF.value < 200) ? FALSE : TRUE);
            if (oBlend.value)
                C.PassSET_Blend_BLEND(TRUE, oAREF.value);
            else
                C.PassSET_Blend_SET();
            C.PassSET_LightFog(TRUE, TRUE);
            C.StageBegin();
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
            C.StageSET_TMC(oT_Name, "$null", "$null", 0);
            C.StageEnd();
        }
        C.PassEnd();
    }
    else
    {
        switch (C.iElement)
        {
        case SE_R1_NORMAL_HQ:
        {
            C.PassBegin();
            {
                C.PassSET_ZB(TRUE, TRUE);
                C.PassSET_Blend_SET();
                C.PassSET_LightFog(TRUE, TRUE);

                //float3 norm_w = normalize(mul(m_W,v.norm)); 
                //float3	calc_model_lq_lighting	(float3 norm_w) 
                //{     
                //     return calc_model_hemi(norm_w) + L_ambient + L_dynamic_props.xyz*calc_sun(norm_w); 	
                //}
                //
                //expands to:
                //
                //float3	calc_model_lq_lighting	(float3 norm_w) 
                //{ 
                //     auto calculated_hemi = (norm_w.y*0.5+0.5) * L_dynamic_props.w*L_hemi_color);
                //     auto calculated_sun = L_sun_color * max(dot((norm_w),-L_sun_dir_w), 0);
                //     return (calculated_hemi + L_ambient + L_dynamic_props.xyz * calculated_sun); 
                //}
                //
                //I.c0 = calc_sun(norm_w); //expands to: L_sun_color * max(dot((norm_w),-L_sun_dir_w), 0)
                //I.c1 = float4(calc_model_lq_lighting(norm_w), m_plmap_clamp[0].w); // lq-color
                // 
                // target output: lerp(l_base + l_sun, I.c1, I.c1.w) * t_base * 2
                // 
                // output: l_base
                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
                C.StageSET_TMC("$user$projector", "$user$projector", "$null", 0);
                C.StageEnd();

                // output: l_base * t_base * 2 
                /*C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_CURRENT);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
                C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
                C.StageEnd();*/
            }
            C.PassEnd();
            break;
        }
        case SE_R1_NORMAL_LQ:
        {
            C.PassBegin();
            {
                C.PassSET_ZB(TRUE, TRUE);
                C.PassSET_Blend_SET();
                C.PassSET_LightFog(TRUE, TRUE);

                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
                C.StageSET_TMC("$user$projector", "$user$projector", "$null", 0);
                C.StageEnd();

                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_BLENDTEXTUREALPHA, D3DTA_CURRENT);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG2, D3DTA_CURRENT);
                C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
                C.StageEnd();
            }
            C.PassEnd();
            break;
        }
        case SE_R1_LSPOT:
            auto lighttex = "internal" DELIMITER "internal_light_att";
            auto lighttex2 = TEX_SPOT_ATT;

            C.PassBegin();
            {
                C.PassSET_ZB(TRUE, TRUE);
                C.PassSET_Blend_SET();
                C.PassSET_LightFog(TRUE, TRUE);

                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_CURRENT);
                C.StageSET_TMC(lighttex, "$null", "$null", 0);
                C.StageEnd();

                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_ADD, D3DTA_CURRENT);
                C.StageSET_TMC(lighttex2, "$null", "$null", 0);
                C.StageEnd();


                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_BLENDTEXTUREALPHA, D3DTA_CURRENT);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG2, D3DTA_CURRENT);
                C.StageSET_TMC(oT_Name, oT_xform, "$null", 0);
                C.StageEnd();
            }
            C.PassEnd();

            break;
        } // switch (C.iElement)
    }
}

void CBlender_Model::CompileProgrammable(CBlender_Compile& C) const
{
    LPCSTR vsname = nullptr;
    LPCSTR psname = nullptr;
    switch (C.iElement)
    {
    case SE_R1_NORMAL_HQ:
        vsname = psname = "model_def_hq";
        if (oBlend.value)
            C.r_Pass(
                vsname, psname, TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", "$user$projector", true);
        C.r_End();
        break;
    case SE_R1_NORMAL_LQ:
        vsname = psname = "model_def_lq";
        if (oBlend.value)
            C.r_Pass(
                vsname, psname, TRUE, TRUE, TRUE, TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_End();
        break;
    case SE_R1_LPOINT:
        vsname = "model_def_point";
        psname = "add_point";
        if (oBlend.value)
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", TEX_POINT_ATT);
        C.r_Sampler_clf("s_att", TEX_POINT_ATT);
        C.r_End();
        break;
    case SE_R1_LSPOT:
        vsname = "model_def_spot";
        psname = "add_spot";
        if (oBlend.value)
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE, oAREF.value);
        else
            C.r_Pass(vsname, psname, FALSE, TRUE, FALSE, TRUE, D3DBLEND_ONE, D3DBLEND_ONE, TRUE);
        C.r_Sampler("s_base", C.L_textures[0]);
        C.r_Sampler_clf("s_lmap", "internal" DELIMITER "internal_light_att", true);
        C.r_Sampler_clf("s_att", TEX_SPOT_ATT);
        C.r_End();
        break;
    case SE_R1_LMODELS:
        vsname = "model_def_shadow";
        psname = "model_shadow";
        C.r_Pass(vsname, psname, FALSE, FALSE, FALSE, TRUE, D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, FALSE, 0);
        C.r_End();
        break;
    }
}
