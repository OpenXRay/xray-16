#include "stdafx.h"
#pragma hdrstop

#include "xr_efflensflare.h"

#include "IGame_Persistent.h"
#include "Environment.h"

// Instead of SkeletonCustom:
#include "xrCore/Animation/Bone.hpp"
#include "Include/xrRender/Kinematics.h"
#include "xrCDB/Intersect.hpp"
#include "Common/object_broker.h"
#include "xrMaterialSystem/GameMtlLib.h"

#ifdef _EDITOR
#include "ui_toolscustom.h"
#include "ui_main.h"
#else
#include "xr_object.h"
#include "IGame_Level.h"
#endif

#define FAR_DIST g_pGamePersistent->Environment().CurrentEnv.far_plane

//#define MAX_Flares 24
//////////////////////////////////////////////////////////////////////////////
// Globals ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define BLEND_INC_SPEED 8.0f
#define BLEND_DEC_SPEED 4.0f

int ps_disable_lens_flare = 0;

//------------------------------------------------------------------------------
void CLensFlareDescriptor::SetSource(float fRadius, bool ign_color, pcstr tex_name, pcstr sh_name)
{
    m_Source.fRadius = fRadius;
    m_Source.shader = sh_name;
    m_Source.texture = tex_name;
    m_Source.ignore_color = ign_color;
}

void CLensFlareDescriptor::SetGradient(float fMaxRadius, float fOpacity, pcstr tex_name, pcstr sh_name)
{
    m_Gradient.fRadius = fMaxRadius;
    m_Gradient.fOpacity = fOpacity;
    m_Gradient.shader = sh_name;
    m_Gradient.texture = tex_name;
}

void CLensFlareDescriptor::AddFlare(float fRadius, float fOpacity, float fPosition, pcstr tex_name, pcstr sh_name)
{
    SFlare F;
    F.fRadius = fRadius;
    F.fOpacity = fOpacity;
    F.fPosition = fPosition;
    F.shader = sh_name;
    F.texture = tex_name;
    m_Flares.push_back(F);
}

struct FlareDescriptorFields
{
    pcstr line;
    pcstr shader;
    pcstr texture;
    pcstr radius;
    pcstr ignore_color;
};

FlareDescriptorFields SourceFields =
{
    "source", "source_shader", "source_texture", "source_radius", "source_ignore_color"
};

FlareDescriptorFields SunFields =
{
    "sun", "sun_shader", "sun_texture", "sun_radius", "sun_ignore_color"
};

CLensFlareDescriptor::CLensFlareDescriptor(shared_str sect, CInifile const* pIni)
{
    section = sect;

    const auto read = [&](FlareDescriptorFields f)
    {
        m_Flags.set(flSource, pIni->r_bool(sect, f.line));
        if (m_Flags.is(flSource))
        {
            pcstr s = pIni->r_string(sect, f.shader);
            pcstr t = pIni->r_string(sect, f.texture);
            float r = pIni->r_float(sect, f.radius);
            bool i = pIni->r_bool(sect, f.ignore_color);
            SetSource(r, i, t, s);
        }
    };

    if (pIni->line_exist(sect, SourceFields.line))
    {
        read(SourceFields);
        // What if someone adapted SOC configs and didn't deleted "source" field?
        // Try to read "sun" optional overriding values.
        if (pIni->line_exist(sect, SunFields.line))
            read(SunFields);
    }
    else
        read(SunFields);

    m_Flags.set(flFlare, pIni->r_bool(sect, "flares"));
    if (m_Flags.is(flFlare))
    {
        pcstr S = pIni->r_string(sect, "flare_shader");
        pcstr T = pIni->r_string(sect, "flare_textures");
        pcstr R = pIni->r_string(sect, "flare_radius");
        pcstr O = pIni->r_string(sect, "flare_opacity");
        pcstr P = pIni->r_string(sect, "flare_position");
        u32 tcnt = _GetItemCount(T);
        m_Flares.reserve(tcnt);
        string256 name;
        for (u32 i = 0; i < tcnt; ++i)
        {
            _GetItem(R, i, name);
            float r = (float)atof(name);
            _GetItem(O, i, name);
            float o = (float)atof(name);
            _GetItem(P, i, name);
            float p = (float)atof(name);
            _GetItem(T, i, name);
            AddFlare(r, o, p, name, S);
        }
    }
    m_Flags.set(flGradient, CInifile::isBool(pIni->r_string(sect, "gradient")));
    if (m_Flags.is(flGradient))
    {
        pcstr S = pIni->r_string(sect, "gradient_shader");
        pcstr T = pIni->r_string(sect, "gradient_texture");
        float r = pIni->r_float(sect, "gradient_radius");
        float o = pIni->r_float(sect, "gradient_opacity");
        SetGradient(r, o, T, S);
    }
    m_StateBlendUpSpeed = 1.f / (_max(pIni->r_float(sect, "blend_rise_time"), 0.f) + EPS_S);
    m_StateBlendDnSpeed = 1.f / (_max(pIni->r_float(sect, "blend_down_time"), 0.f) + EPS_S);

    OnDeviceCreate();
}

void CLensFlareDescriptor::OnDeviceCreate()
{
    // shaders
    m_Gradient.m_pRender->CreateShader(m_Gradient.shader.c_str(), m_Gradient.texture.c_str());
    m_Source.m_pRender->CreateShader(m_Source.shader.c_str(), m_Source.texture.c_str());
    for (const auto& flare : m_Flares)
        flare.m_pRender->CreateShader(flare.shader.c_str(), flare.texture.c_str());
}

void CLensFlareDescriptor::OnDeviceDestroy()
{
    // shaders
    m_Gradient.m_pRender->DestroyShader();
    m_Source.m_pRender->DestroyShader();
    for (const auto& flare : m_Flares)
        flare.m_pRender->DestroyShader();
}

//------------------------------------------------------------------------------
CLensFlare::CLensFlare()
{
    // Device
    dwFrame = 0xfffffffe;

    fBlend = 0.f;

    LightColor.set(0xFFFFFFFF);
    fGradientValue = 0.f;

    m_Current = 0;

    m_State = lfsNone;
    m_StateBlend = 0.f;

    string_path filePath;
    if (FS.exist(filePath, "$game_config$", "environment\\suns.ltx"))
        m_suns_config = xr_new<CInifile>(filePath, true, true, false);

    OnDeviceCreate();
}

CLensFlare::~CLensFlare()
{
    OnDeviceDestroy();
    delete_data(m_Palette);
    CInifile::Destroy(m_suns_config);
    m_suns_config = nullptr;
}

IC void blend_lerp(float& cur, float tgt, float speed, float dt)
{
    float diff = tgt - cur;
    float diff_a = _abs(diff);
    if (diff_a < EPS_S)
        return;
    float mot = speed * dt;
    if (mot > diff_a)
        mot = diff_a;
    cur += (diff / diff_a) * mot;
}

#if 0
static pcstr state_to_string(const CLensFlare::LFState& state)
{
    switch (state)
    {
    case CLensFlare::lfsNone:
        return("none");
    case CLensFlare::lfsIdle:
        return("idle");
    case CLensFlare::lfsHide:
        return("hide");
    case CLensFlare::lfsShow:
        return("show");
    default:
        NODEFAULT;
    }
#ifdef DEBUG
    return (0);
#endif // DEBUG
}
#endif

void CLensFlare::OnFrame(const CEnvDescriptorMixer& currentEnv, float time_factor)
{
    if (dwFrame == Device.dwFrame)
        return;
#ifndef _EDITOR
    if (!g_pGameLevel)
        return;
#endif
    ZoneScoped;

    dwFrame = Device.dwFrame;

    R_ASSERT(_valid(currentEnv.sun_dir));
    vSunDir.mul(currentEnv.sun_dir, -1);
    R_ASSERT(_valid(vSunDir));

    // color
    {
        const auto& [x, y, z] = currentEnv.sun_color;
        LightColor.set(x, y, z, 1.f);
    }

    auto flare = currentEnv.lens_flare;

    // LFState previous_state = m_State;
    switch (m_State)
    {
    case lfsNone:
        m_State = lfsShow;
        m_Current = flare;
        break;
    case lfsIdle:
        if (flare != m_Current)
            m_State = lfsHide;
        break;
    case lfsShow:
        m_StateBlend = m_Current ? (m_StateBlend + m_Current->m_StateBlendUpSpeed * Device.fTimeDelta * time_factor) : 1.f + EPS;
        if (m_StateBlend >= 1.f)
            m_State = lfsIdle;
        break;
    case lfsHide:
        m_StateBlend = m_Current ? (m_StateBlend - m_Current->m_StateBlendDnSpeed * Device.fTimeDelta * time_factor) : 0.f - EPS;
        if (m_StateBlend <= 0.f)
        {
            m_State = lfsShow;
            m_Current = flare;
            m_StateBlend = m_Current ? m_Current->m_StateBlendUpSpeed * Device.fTimeDelta * time_factor : 0;
        }
        break;
    }
    // Msg ("%6d : [%s] -> [%s]", Device.dwFrame, state_to_string(previous_state), state_to_string(m_State));
    clamp(m_StateBlend, 0.f, 1.f);

    if (!m_Current || LightColor.magnitude_rgb() == 0.f)
    {
        bRender = false;
        return;
    }

    //
    // Compute center and axis of flares
    //
    float fDot;

    Fvector vecPos;

    Fmatrix matEffCamPos;
    matEffCamPos.identity();
    // Calculate our position and direction

    matEffCamPos.i.set(Device.vCameraRight);
    matEffCamPos.j.set(Device.vCameraTop);
    matEffCamPos.k.set(Device.vCameraDirection);
    vecPos.set(Device.vCameraPosition);

    vecDir.set(0.0f, 0.0f, 1.0f);
    matEffCamPos.transform_dir(vecDir);
    vecDir.normalize();

    // Figure out of light (or flare) might be visible
    vecLight.set(vSunDir);
    vecLight.normalize();

    fDot = vecLight.dotproduct(vecDir);

    if (fDot <= 0.01f)
    {
        bRender = false;
        return;
    }
    else
        bRender = true;

    // Calculate the point directly in front of us, on the far clip plane
    float fDistance = FAR_DIST * 0.75f;
    vecCenter.mul(vecDir, fDistance);
    vecCenter.add(vecPos);
    // Calculate position of light on the far clip plane
    vecLight.mul(fDistance / fDot);
    vecLight.add(vecPos);
    // Compute axis which goes from light through the center of the screen
    vecAxis.sub(vecLight, vecCenter);

    //
    // Figure out if light is behind something else
    vecX.set(1.0f, 0.0f, 0.0f);
    matEffCamPos.transform_dir(vecX);
    vecX.normalize();
    R_ASSERT(_valid(vecX));

    vecY.crossproduct(vecX, vecDir);
    R_ASSERT(_valid(vecY));

#ifdef _EDITOR
    float dist = UI->ZFar();
    if (Tools->RayPick(Device.m_Camera.GetPosition(), vSunDir, dist))
        fBlend = fBlend - BLEND_DEC_SPEED * Device.fTimeDelta;
    else
        fBlend = fBlend + BLEND_INC_SPEED * Device.fTimeDelta;
#else

    blend_lerp(fBlend, 1.0f, BLEND_DEC_SPEED, Device.fTimeDelta);
#endif
    clamp(fBlend, 0.0f, 1.0f);

    // gradient
    if (m_Current->m_Flags.is(CLensFlareDescriptor::flGradient))
    {
        Fvector scr_pos;
        Device.mFullTransform.transform(scr_pos, vecLight);
        float kx = 1, ky = 1;
        float sun_blend = 0.5f;
        float sun_max = 2.5f;
        scr_pos.y *= -1;

        if (_abs(scr_pos.x) > sun_blend)
            kx = ((sun_max - (float)_abs(scr_pos.x))) / (sun_max - sun_blend);
        if (_abs(scr_pos.y) > sun_blend)
            ky = ((sun_max - (float)_abs(scr_pos.y))) / (sun_max - sun_blend);

        if (!((_abs(scr_pos.x) > sun_max) || (_abs(scr_pos.y) > sun_max)))
        {
            float op = m_StateBlend * m_Current->m_Gradient.fOpacity;
            fGradientValue = kx * ky * op * fBlend;
        }
        else
            fGradientValue = 0;
    }
}

void CLensFlare::Render(bool bSun, bool bFlares, bool bGradient)
{
    if (ps_disable_lens_flare)
    {
        bFlares = false;
    }
    if (!bRender)
        return;
    if (!m_Current)
        return;
    VERIFY(m_Current);

    m_pRender->Render(*this, bSun, bFlares, bGradient);
}

CLensFlareDescriptor* CLensFlare::AppendDef(shared_str sect)
{
    if (!sect || (0 == sect[0]))
        return nullptr;

    for (CLensFlareDescriptor* flare : m_Palette)
        if (flare->section == sect)
            return flare;

    const auto descriptor = xr_new<CLensFlareDescriptor>(sect,
        m_suns_config ? m_suns_config : pSettings
    );
    return m_Palette.emplace_back(descriptor);
}

void CLensFlare::OnDeviceCreate()
{
    // VS
    m_pRender->OnDeviceCreate();

    // palette
    for (auto& descr : m_Palette)
        descr->OnDeviceCreate();
}

void CLensFlare::OnDeviceDestroy()
{
    // palette
    for (auto& descr : m_Palette)
        descr->OnDeviceDestroy();

    // VS
    m_pRender->OnDeviceDestroy();
}
