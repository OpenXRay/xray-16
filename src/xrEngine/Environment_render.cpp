#include "stdafx.h"
#pragma hdrstop

#include "Environment.h"
#ifndef _EDITOR
#include "Render.h"
#endif
#include "xr_efflensflare.h"
#include "Rain.h"
#include "thunderbolt.h"

#ifndef _EDITOR
#include "IGame_Level.h"
#endif

//-----------------------------------------------------------------------------
// Environment render
//-----------------------------------------------------------------------------

void CEnvironment::RenderSky()
{
#ifndef _EDITOR
    if (0 == g_pGameLevel)
        return;
#endif

    m_pRender->RenderSky(*this);
}

void CEnvironment::RenderClouds()
{
#ifndef _EDITOR
    if (0 == g_pGameLevel)
        return;
#endif
    // draw clouds
    if (fis_zero(CurrentEnv->clouds_color.w, EPS_L))
        return;

    m_pRender->RenderClouds(*this);
}

void CEnvironment::RenderFlares()
{
#ifndef _EDITOR
    if (0 == g_pGameLevel)
        return;
#endif
    // 1
    eff_LensFlare->Render(FALSE, TRUE, TRUE);
}

void CEnvironment::RenderLast()
{
#ifndef _EDITOR
    if (0 == g_pGameLevel)
        return;
#endif
    // 2
    eff_Rain->Render();
    eff_Thunderbolt->Render();
}

void CEnvironment::OnDeviceCreate()
{
    m_pRender->OnDeviceCreate();

    // weathers
    {
        auto _I = WeatherCycles.begin();
        auto _E = WeatherCycles.end();
        for (; _I != _E; _I++)
            for (auto it = _I->second.begin(); it != _I->second.end(); it++)
                (*it)->on_device_create();
    }
    // effects
    {
        auto _I = WeatherFXs.begin();
        auto _E = WeatherFXs.end();
        for (; _I != _E; _I++)
            for (auto it = _I->second.begin(); it != _I->second.end(); it++)
                (*it)->on_device_create();
    }

    Invalidate();
    OnFrame();
}

void CEnvironment::OnDeviceDestroy()
{
    m_pRender->OnDeviceDestroy();

    // weathers
    {
        auto _I = WeatherCycles.begin();
        auto _E = WeatherCycles.end();
        for (; _I != _E; _I++)
            for (auto it = _I->second.begin(); it != _I->second.end(); it++)
                (*it)->on_device_destroy();
    }
    // effects
    {
        auto _I = WeatherFXs.begin();
        auto _E = WeatherFXs.end();
        for (; _I != _E; _I++)
            for (auto it = _I->second.begin(); it != _I->second.end(); it++)
                (*it)->on_device_destroy();
    }
    CurrentEnv->destroy();
}

#ifdef _EDITOR
void CEnvironment::ED_Reload()
{
    OnDeviceDestroy();
    OnDeviceCreate();
}
#endif
