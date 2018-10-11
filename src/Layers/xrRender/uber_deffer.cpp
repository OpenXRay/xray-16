#include "stdafx.h"
#include "uber_deffer.h"
void fix_texture_name(LPSTR fn);

void uber_deffer(CBlender_Compile& C, bool hq, LPCSTR _vspec, LPCSTR _pspec, BOOL _aref, LPCSTR _detail_replace,
    bool DO_NOT_FINISH)
{
    // Uber-parse
    string256 fname, fnameA, fnameB;
    xr_strcpy(fname, *C.L_textures[0]); //. andy if (strext(fname)) *strext(fname)=0;
    fix_texture_name(fname);
    ref_texture _t;
    _t.create(fname);
    bool bump = _t.bump_exist();

    // detect lmap
    bool lmap = true;
    if (C.L_textures.size() < 3)
        lmap = false;
    else
    {
        pcstr tex = C.L_textures[2].c_str();
        if (tex[0] == 'l' && tex[1] == 'm' && tex[2] == 'a' && tex[3] == 'p')
            lmap = true;
        else
            lmap = false;
    }

    string256 ps, vs, dt;
    strconcat(sizeof(vs), vs, "deffer_", _vspec, lmap ? "_lmh" : "");
    strconcat(sizeof(ps), ps, "deffer_", _pspec, lmap ? "_lmh" : "");
    xr_strcpy(dt, sizeof(dt), _detail_replace ? _detail_replace : (C.detail_texture ? C.detail_texture : ""));

    // detect detail bump
    string256 texDetailBump = {'\0'};
    string256 texDetailBumpX = {'\0'};
    bool bHasDetailBump = false;
    if (C.bDetail_Bump)
    {
        LPCSTR detail_bump_texture = RImplementation.Resources->m_textures_description.GetBumpName(dt).c_str();
        // Detect and use detail bump
        if (detail_bump_texture)
        {
            bHasDetailBump = true;
            xr_strcpy(texDetailBump, sizeof(texDetailBump), detail_bump_texture);
            xr_strcpy(texDetailBumpX, sizeof(texDetailBumpX), detail_bump_texture);
            VERIFY(xr_strlen(texDetailBump) > 2);
            VERIFY(xr_strlen(texDetailBumpX) > 2);
            xr_strcat(texDetailBumpX, "#");
        }
    }

    if (_aref)
    {
        xr_strcat(ps, "_aref");
    }

    if (!bump)
    {
        fnameA[0] = fnameB[0] = 0;
        xr_strcat(vs, "_flat");
        xr_strcat(ps, "_flat");
        if (hq && (C.bDetail_Diffuse || C.bDetail_Bump))
        {
            xr_strcat(vs, "_d");
            xr_strcat(ps, "_d");
        }
    }
    else
    {
        xr_strcpy(fnameA, _t.bump_get().c_str());
        strconcat(sizeof(fnameB), fnameB, fnameA, "#");
        xr_strcat(vs, "_bump");
        if (hq && C.bUseSteepParallax)
        {
            xr_strcat(ps, "_steep");
        }
        else
        {
            xr_strcat(ps, "_bump");
        }
        if (hq && (C.bDetail_Diffuse || C.bDetail_Bump))
        {
            xr_strcat(vs, "_d");
            if (bHasDetailBump)
                xr_strcat(ps, "_db"); // bump & detail & hq
            else
                xr_strcat(ps, "_d");
        }
    }

    // HQ
    if (bump && hq)
    {
        xr_strcat(vs, "-hq");
        xr_strcat(ps, "-hq");
    }

// Uber-construct
#if defined(USE_DX10) || defined(USE_DX11)
#ifdef USE_DX11
    if (bump && hq && RImplementation.o.dx11_enable_tessellation && C.TessMethod != 0)
    {
        char hs[256], ds[256]; // = "DX11" DELIMITER "tess", ds[256] = "DX11" DELIMITER "tess";
        char params[256] = "(";

        if (C.TessMethod == CBlender_Compile::TESS_PN || C.TessMethod == CBlender_Compile::TESS_PN_HM)
        {
            RImplementation.addShaderOption("TESS_PN", "1");
            xr_strcat(params, "TESS_PN,");
        }

        if (C.TessMethod == CBlender_Compile::TESS_HM || C.TessMethod == CBlender_Compile::TESS_PN_HM)
        {
            RImplementation.addShaderOption("TESS_HM", "1");
            xr_strcat(params, "TESS_HM,");
        }

        if (lmap)
        {
            RImplementation.addShaderOption("USE_LM_HEMI", "1");
            xr_strcat(params, "USE_LM_HEMI,");
        }

        if (C.bDetail_Diffuse)
        {
            RImplementation.addShaderOption("USE_TDETAIL", "1");
            xr_strcat(params, "USE_TDETAIL,");
        }

        if (C.bDetail_Bump)
        {
            RImplementation.addShaderOption("USE_TDETAIL_BUMP", "1");
            xr_strcat(params, "USE_TDETAIL_BUMP,");
        }

        xr_strcat(params, ")");

        strconcat(sizeof(vs), vs, "deffer_", _vspec, "_bump", params);
        strconcat(sizeof(ps), ps, "deffer_", _pspec, _aref ? "_aref" : "", "_bump", params);
        strconcat(sizeof(hs), hs, "DX11" DELIMITER "tess", params);
        strconcat(sizeof(ds), ds, "DX11" DELIMITER "tess", params);

        VERIFY(strstr(vs, "bump") != 0);
        VERIFY(strstr(ps, "bump") != 0);
        C.r_TessPass(vs, hs, ds, "null", ps, FALSE);
        RImplementation.clearAllShaderOptions();
        u32 stage = C.r_dx10Sampler("smp_bump_ds");
        if (stage != -1)
        {
            C.i_Address(stage, D3DTADDRESS_WRAP);
            C.i_dx10FilterAnizo(stage, TRUE);
        }
        if (ps_r2_ls_flags_ext.test(R2FLAGEXT_WIREFRAME))
            C.R().SetRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
        C.r_dx10Texture("s_tbump", fnameA);
        C.r_dx10Texture("s_tbumpX", fnameB); // should be before base bump
        if (bHasDetailBump)
        {
            C.r_dx10Texture("s_tdetailBumpX", texDetailBumpX);
        }
    }
    else
#endif
        C.r_Pass(vs, ps, FALSE);
    // C.r_Sampler("s_base", C.L_textures[0], false, D3DTADDRESS_WRAP,
    // D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
    // D3DTEXF_ANISOTROPIC);
    // C.r_Sampler("s_bumpX", fnameB, false, D3DTADDRESS_WRAP,
    // D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
    // D3DTEXF_ANISOTROPIC); // should be before base bump
    // C.r_Sampler("s_bump", fnameA, false, D3DTADDRESS_WRAP,
    // D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
    // D3DTEXF_ANISOTROPIC);
    // C.r_Sampler("s_bumpD", dt, false, D3DTADDRESS_WRAP,
    // D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,
    // D3DTEXF_ANISOTROPIC);
    // C.r_Sampler("s_detail", dt, false, D3DTADDRESS_WRAP,
    // D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
    // D3DTEXF_ANISOTROPIC);
    C.r_dx10Texture("s_base", C.L_textures[0]);
    C.r_dx10Texture("s_bumpX", fnameB); // should be before base bump
    C.r_dx10Texture("s_bump", fnameA);
    C.r_dx10Texture("s_bumpD", dt);
    C.r_dx10Texture("s_detail", dt);
    if (bHasDetailBump)
    {
        C.r_dx10Texture("s_detailBump", texDetailBump);
        C.r_dx10Texture("s_detailBumpX", texDetailBumpX);
        VERIFY(xr_strlen(texDetailBump) > 2);
        VERIFY(xr_strlen(texDetailBumpX) > 2);
    }
    C.r_dx10Sampler("smp_base");
    if (lmap)
    {
        // C.r_Sampler("s_hemi", C.L_textures[2], false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE,
        // D3DTEXF_LINEAR);
        C.r_dx10Texture("s_hemi", C.L_textures[2]);
        C.r_dx10Sampler("smp_rtlinear");
    }
#else //    USE_DX10
    C.r_Pass(vs, ps, FALSE);
    VERIFY(C.L_textures[0].size());
    if (bump)
    {
        VERIFY2(xr_strlen(fnameB), C.L_textures[0].c_str());
        VERIFY2(xr_strlen(fnameA), C.L_textures[0].c_str());
    }
    if (bHasDetailBump)
    {
        VERIFY2(xr_strlen(texDetailBump), C.L_textures[0].c_str());
        VERIFY2(xr_strlen(texDetailBumpX), C.L_textures[0].c_str());
    }
    C.r_Sampler(
        "s_base", C.L_textures[0], false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
    C.r_Sampler("s_bumpX", fnameB, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
        D3DTEXF_ANISOTROPIC); // should be before base bump
    C.r_Sampler("s_bump", fnameA, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
    C.r_Sampler("s_bumpD", dt, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
    C.r_Sampler("s_detail", dt, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC);
    if (bHasDetailBump)
    {
        C.r_Sampler("s_detailBump", texDetailBump, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
            D3DTEXF_ANISOTROPIC);
        C.r_Sampler("s_detailBumpX", texDetailBumpX, false, D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR,
            D3DTEXF_ANISOTROPIC);
    }
    if (lmap)
        C.r_Sampler("s_hemi", C.L_textures[2], false, D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR);
#endif // USE_DX10

    if (!DO_NOT_FINISH)
        C.r_End();
}

#ifdef USE_DX11
void uber_shadow(CBlender_Compile& C, LPCSTR _vspec)
{
    // Uber-parse
    string256 fname, fnameA, fnameB;
    xr_strcpy(fname, *C.L_textures[0]); //. andy if (strext(fname)) *strext(fname)=0;
    fix_texture_name(fname);
    ref_texture _t;
    _t.create(fname);
    bool bump = _t.bump_exist();

    // detect lmap
    bool lmap = true;
    if (C.L_textures.size() < 3)
        lmap = false;
    else
    {
        pcstr tex = C.L_textures[2].c_str();
        if (tex[0] == 'l' && tex[1] == 'm' && tex[2] == 'a' && tex[3] == 'p')
            lmap = true;
        else
            lmap = false;
    }

    string256 vs, dt;
    xr_strcpy(dt, sizeof(dt), C.detail_texture ? C.detail_texture : "");

    // detect detail bump
    string256 texDetailBump = {'\0'};
    string256 texDetailBumpX = {'\0'};
    bool bHasDetailBump = false;
    if (C.bDetail_Bump)
    {
        LPCSTR detail_bump_texture = RImplementation.Resources->m_textures_description.GetBumpName(dt).c_str();
        //  Detect and use detail bump
        if (detail_bump_texture)
        {
            bHasDetailBump = true;
            xr_strcpy(texDetailBump, sizeof(texDetailBump), detail_bump_texture);
            xr_strcpy(texDetailBumpX, sizeof(texDetailBumpX), detail_bump_texture);
            xr_strcat(texDetailBumpX, "#");
        }
    }

    if (!bump)
    {
        fnameA[0] = fnameB[0] = 0;
    }
    else
    {
        xr_strcpy(fnameA, _t.bump_get().c_str());
        strconcat(sizeof(fnameB), fnameB, fnameA, "#");
    }

    if (bump && RImplementation.o.dx11_enable_tessellation && C.TessMethod != 0)
    {
        char hs[256], ds[256]; // = "DX11" DELIMITER "tess", ds[256] = "DX11" DELIMITER "tess";
        char params[256] = "(";

        if (C.TessMethod == CBlender_Compile::TESS_PN || C.TessMethod == CBlender_Compile::TESS_PN_HM)
        {
            RImplementation.addShaderOption("TESS_PN", "1");
            xr_strcat(params, "TESS_PN,");
        }

        if (C.TessMethod == CBlender_Compile::TESS_HM || C.TessMethod == CBlender_Compile::TESS_PN_HM)
        {
            RImplementation.addShaderOption("TESS_HM", "1");
            xr_strcat(params, "TESS_HM,");
        }

        if (lmap)
        {
            RImplementation.addShaderOption("USE_LM_HEMI", "1");
            xr_strcat(params, "USE_LM_HEMI,");
        }

        if (C.bDetail_Diffuse)
        {
            RImplementation.addShaderOption("USE_TDETAIL", "1");
            xr_strcat(params, "USE_TDETAIL,");
        }

        if (C.bDetail_Bump)
        {
            RImplementation.addShaderOption("USE_TDETAIL_BUMP", "1");
            xr_strcat(params, "USE_TDETAIL_BUMP,");
        }

        xr_strcat(params, ")");

        strconcat(sizeof(vs), vs, "deffer_", _vspec, "_bump", params);
        strconcat(sizeof(hs), hs, "DX11" DELIMITER "tess", params);
        strconcat(sizeof(ds), ds, "DX11" DELIMITER "tess_shadow", params);

        C.r_TessPass(vs, hs, ds, "null", "dumb", FALSE, TRUE, TRUE, FALSE);
        RImplementation.clearAllShaderOptions();
        C.r_dx10Texture("s_base", C.L_textures[0]);
        C.r_dx10Texture("s_bumpX", fnameB); // should be before base bump
        C.r_dx10Texture("s_bump", fnameA);
        if (bHasDetailBump)
        {
            C.r_dx10Texture("s_detailBump", texDetailBump);
            C.r_dx10Texture("s_detailBumpX", texDetailBumpX);
        }
        u32 stage = C.r_dx10Sampler("smp_bump_ds");
        if (stage != -1)
        {
            C.i_Address(stage, D3DTADDRESS_WRAP);
            C.i_dx10FilterAnizo(stage, TRUE);
        }
        if (ps_r2_ls_flags_ext.test(R2FLAGEXT_WIREFRAME))
            C.R().SetRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    }
    else
        C.r_Pass("shadow_direct_base", "dumb", FALSE, TRUE, TRUE, FALSE);
}
#endif
