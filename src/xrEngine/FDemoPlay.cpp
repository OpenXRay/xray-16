// CDemoPlay.cpp: implementation of the CDemoPlay class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "IGame_Level.h"
#include "FDemoPlay.h"
#include "XR_IOConsole.h"
#include "xrCore/Animation/Motion.hpp"
#include "Render.h"
#include "CameraManager.h"

#if !defined(XR_PLATFORM_LINUX)
#include "xrSASH.h"
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDemoPlay::CDemoPlay(const char* name, float ms, u32 cycles, float life_time)
    : CEffectorCam(cefDemo, life_time /*,false*/)
{
    Msg("*** Playing demo: %s", name);
    Console->Execute("hud_weapon 0");
#if !defined(XR_PLATFORM_LINUX)
    if (g_bBenchmark || g_SASH.IsRunning())
        Console->Execute("hud_draw 0");
#endif
    fSpeed = ms;
    dwCyclesLeft = cycles ? cycles : 1;

    m_pMotion = 0;
    m_MParam = 0;
    string_path nm, fn;
    xr_strcpy(nm, sizeof(nm), name);
    pstr extp = strext(nm);
    if (extp)
        xr_strcpy(nm, sizeof(nm) - (extp - nm), ".anm");

    if (FS.exist(fn, "$level$", nm) || FS.exist(fn, "$game_anims$", nm))
    {
        m_pMotion = xr_new<COMotion>();
        m_pMotion->LoadMotion(fn);
        m_MParam = xr_new<SAnimParams>();
        m_MParam->Set(m_pMotion);
        m_MParam->Play();
    }
    else
    {
        if (!FS.exist(name))
        {
            g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
            return;
        }
        IReader* fs = FS.r_open(name);
        u32 sz = fs->length();
        if (sz % sizeof(Fmatrix) != 0)
        {
            FS.r_close(fs);
            g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
            return;
        }

        seq.resize(sz / sizeof(Fmatrix));
        m_count = seq.size();
        CopyMemory(&*seq.begin(), fs->pointer(), sz);
        FS.r_close(fs);
        Log("~ Total key-frames: ", m_count);
    }
    stat_started = false;
    Device.PreCache(50, true, false);
}

CDemoPlay::~CDemoPlay()
{
    stat_Stop();
    xr_delete(m_pMotion);
    xr_delete(m_MParam);
    Console->Execute("hud_weapon 1");
#if !defined(XR_PLATFORM_LINUX)
    if (g_bBenchmark || g_SASH.IsRunning())
        Console->Execute("hud_draw 1");
#endif
}

void CDemoPlay::stat_Start()
{
    // if (stat_started) return;
    VERIFY(!stat_started);
    stat_started = true;
    Sleep(1);
    stat_StartFrame = Device.dwFrame;
    stat_Timer_frame.Start();
    stat_Timer_total.Start();
    stat_table.clear();
    stat_table.reserve(1024);
    fStartTime = 0;
}

extern string512 g_sBenchmarkName;

void CDemoPlay::stat_Stop()
{
    if (!stat_started)
        return;

    // g_SASH.EndBenchmark();

    stat_started = false;
    float stat_total = stat_Timer_total.GetElapsed_sec();

    float rfps_min, rfps_max, rfps_middlepoint, rfps_average;

    // total
    u32 dwFramesTotal = Device.dwFrame - stat_StartFrame;
    rfps_average = float(dwFramesTotal) / stat_total;

    // min/max/average
    rfps_min = flt_max;
    rfps_max = flt_min;
    rfps_middlepoint = 0;

    // Filtered FPS
    const u32 iAvgFPS = _max((u32)rfps_average, 10);
    const u32 WindowSize = _max(16, iAvgFPS / 2);

    if (stat_table.size() > WindowSize * 4)
    {
        for (u32 it = 2; it < stat_table.size() - WindowSize + 1; it++)
        {
            float fTime = 0;
            for (u32 i = 0; i < WindowSize; ++i)
                fTime += stat_table[it + i];
            float fps = WindowSize / fTime;
            if (fps < rfps_min)
                rfps_min = fps;
            if (fps > rfps_max)
                rfps_max = fps;
            rfps_middlepoint += fps;
        }

        rfps_middlepoint /= float(stat_table.size() - 1 - WindowSize + 1);
    }
    else
    {
        for (u32 it = 1; it < stat_table.size(); it++)
        {
            float fps = 1.f / stat_table[it];
            if (fps < rfps_min)
                rfps_min = fps;
            if (fps > rfps_max)
                rfps_max = fps;
            rfps_middlepoint += fps;
        }
        rfps_middlepoint /= float(stat_table.size() - 1);
    }

    /*
    for (u32 it=1; it<stat_table.size(); it++)
    {
    float fps = 1.f / stat_table[it];
    if (fps<rfps_min) rfps_min = fps;
    if (fps>rfps_max) rfps_max = fps;
    rfps_middlepoint += fps;
    }
    rfps_middlepoint /= float(stat_table.size()-1);
    */

    Msg("* [DEMO] FPS: average[%f], min[%f], max[%f], middle[%f]", rfps_average, rfps_min, rfps_max, rfps_middlepoint);

    if (g_bBenchmark)
    {
        string_path fname;

        if (xr_strlen(g_sBenchmarkName))
            xr_sprintf(fname, sizeof(fname), "%s.result", g_sBenchmarkName);
        else
            xr_strcpy(fname, sizeof(fname), "benchmark.result");

        FS.update_path(fname, "$app_data_root$", fname);
        CInifile res(fname, false, false, true);
        res.w_float("general", "renderer", float(GEnv.Render->GetGeneration()), "generation-level required");
        res.w_float("general", "min", rfps_min, "absolute minimum");
        res.w_float("general", "max", rfps_max, "absolute maximum");
        res.w_float("general", "average", rfps_average, "average for this run");
        res.w_float("general", "middle", rfps_middlepoint, "per-frame middle-point");
        for (u32 it = 1; it < stat_table.size(); it++)
        {
            string32 id;
            xr_sprintf(id, sizeof(id), "%7d", it);
            for (u32 c = 0; id[c]; c++)
                if (' ' == id[c])
                    id[c] = '0';
            res.w_float("per_frame_stats", id, 1.f / stat_table[it]);
        }

        Console->Execute("quit");
    }
}

#define FIX(a)           \
    while (a >= m_count) \
    a -= m_count
void spline1(float t, Fvector* p, Fvector* ret)
{
    float t2 = t * t;
    float t3 = t2 * t;
    float m[4];

    ret->x = 0.0f;
    ret->y = 0.0f;
    ret->z = 0.0f;
    m[0] = (0.5f * ((-1.0f * t3) + (2.0f * t2) + (-1.0f * t)));
    m[1] = (0.5f * ((3.0f * t3) + (-5.0f * t2) + (0.0f * t) + 2.0f));
    m[2] = (0.5f * ((-3.0f * t3) + (4.0f * t2) + (1.0f * t)));
    m[3] = (0.5f * ((1.0f * t3) + (-1.0f * t2) + (0.0f * t)));

    for (int i = 0; i < 4; i++)
    {
        ret->x += p[i].x * m[i];
        ret->y += p[i].y * m[i];
        ret->z += p[i].z * m[i];
    }
}

bool CDemoPlay::ProcessCam(SCamEffectorInfo& info)
{
    // skeep a few frames before counting
    if (Device.dwPrecacheFrame)
        return true;

    if (stat_started)
    {
        // g_SASH.DisplayFrame(Device.fTimeGlobal);
    }
    else
    {
        // g_SASH.StartBenchmark();
        stat_Start();
    }

    // Per-frame statistics
    {
        stat_table.push_back(stat_Timer_frame.GetElapsed_sec());
        stat_Timer_frame.Start();
    }

    // Process motion
    if (m_pMotion)
    {
        Fvector R;
        Fmatrix mRotate;
        m_pMotion->_Evaluate(m_MParam->Frame(), info.p, R);
        m_MParam->Update(Device.fTimeDelta, 1.f, true);
        fLifeTime -= Device.fTimeDelta;
        if (m_MParam->bWrapped)
        {
            stat_Stop();
            stat_Start();
        }
        mRotate.setXYZi(R.x, R.y, R.z);
        info.d.set(mRotate.k);
        info.n.set(mRotate.j);
    }
    else
    {
        if (seq.empty())
        {
            g_pGameLevel->Cameras().RemoveCamEffector(cefDemo);
            return true;
        }

        fStartTime += Device.fTimeDelta;

        float ip;
        float p = fStartTime / fSpeed;
        float t = modff(p, &ip);
        int frame = iFloor(ip);
        VERIFY(t >= 0);

        if (frame >= m_count)
        {
            dwCyclesLeft--;
            if (0 == dwCyclesLeft)
                return false;
            fStartTime = 0;
            // just continue
            // stat_Stop ();
            // stat_Start ();
        }

        int f1 = frame;
        FIX(f1);
        int f2 = f1 + 1;
        FIX(f2);
        int f3 = f2 + 1;
        FIX(f3);
        int f4 = f3 + 1;
        FIX(f4);

        Fmatrix *m1, *m2, *m3, *m4;
        Fvector v[4];
        m1 = (Fmatrix*)&seq[f1];
        m2 = (Fmatrix*)&seq[f2];
        m3 = (Fmatrix*)&seq[f3];
        m4 = (Fmatrix*)&seq[f4];

        for (int i = 0; i < 4; i++)
        {
            v[0].x = m1->m[i][0];
            v[0].y = m1->m[i][1];
            v[0].z = m1->m[i][2];
            v[1].x = m2->m[i][0];
            v[1].y = m2->m[i][1];
            v[1].z = m2->m[i][2];
            v[2].x = m3->m[i][0];
            v[2].y = m3->m[i][1];
            v[2].z = m3->m[i][2];
            v[3].x = m4->m[i][0];
            v[3].y = m4->m[i][1];
            v[3].z = m4->m[i][2];
            spline1(t, &(v[0]), (Fvector*)&(Device.mView.m[i][0]));
        }

        Fmatrix mInvCamera;
        mInvCamera.invert(Device.mView);
        info.n.set(mInvCamera._21, mInvCamera._22, mInvCamera._23);
        info.d.set(mInvCamera._31, mInvCamera._32, mInvCamera._33);
        info.p.set(mInvCamera._41, mInvCamera._42, mInvCamera._43);

        fLifeTime -= Device.fTimeDelta;
    }
    return true;
}
