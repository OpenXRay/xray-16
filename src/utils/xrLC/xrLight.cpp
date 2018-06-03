#include "stdafx.h"
#include "build.h"
#include <random>

#include "utils/xrLC_Light/xrdeflector.h"
#include "utils/xrLCUtil/xrThread.hpp"
#include "utils/xrLC_Light/xrLC_GlobalData.h"
#include "utils/xrLC_Light/xrLightVertex.h"

#include "xrCore/Threading/Lock.hpp"
#include "net.h"

#include "utils/xrLC_Light/lcnet_task_manager.h"
#include "utils/xrLC_Light/mu_model_light.h"
Lock task_CS
#ifdef CONFIG_PROFILE_LOCKS
    (MUTEX_PROFILE_ID(task_C_S))
#endif // CONFIG_PROFILE_LOCKS
        ;

xr_vector<int> task_pool;

class CLMThread : public CThread
{
private:
    HASH H;
    CDB::COLLIDER DB;
    base_lighting LightsSelected;

public:
    CLMThread(u32 ID) : CThread(ID, ProxyMsg)
    {
        // thMonitor= TRUE;
        thMessages = FALSE;
    }

    virtual void Execute()
    {
        CDeflector* D = 0;

        for (;;)
        {
            // Get task
            task_CS.Enter();
            thProgress = 1.f - float(task_pool.size()) / float(lc_global_data()->g_deflectors().size());
            if (task_pool.empty())
            {
                task_CS.Leave();
                return;
            }

            D = lc_global_data()->g_deflectors()[task_pool.back()];
            task_pool.pop_back();
            task_CS.Leave();

            // Perform operation
            try
            {
                D->Light(&DB, &LightsSelected, H);
            }
            catch (...)
            {
                Logger.clMsg("* ERROR: CLMThread::Execute - light");
            }
        }
    }
};

void CBuild::LMapsLocal()
{
    FPU::m64r();

    mem_Compact();

// Randomize deflectors
#ifndef NET_CMP
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(lc_global_data()->g_deflectors().begin(), lc_global_data()->g_deflectors().end(), g);
#endif

#ifndef NET_CMP
    for (u32 dit = 0; dit < lc_global_data()->g_deflectors().size(); dit++)
        task_pool.push_back(dit);
#else
    task_pool.push_back(14);
    task_pool.push_back(16);
#endif

    // Main process (4 threads)
    Logger.Status("Lighting...");
    CThreadManager threads(ProxyStatus, ProxyProgress);
    CTimer start_time;
    start_time.Start();
    for (int L = 0; L < NUM_THREADS; L++)
        threads.start(new CLMThread(L));
    threads.wait(500);
    Logger.clMsg("%f seconds", start_time.GetElapsed_sec());
}

void CBuild::LMaps()
{
    //****************************************** Lmaps
    Logger.Phase("LIGHT: LMaps...");
// DeflectorsStats ();
#ifndef NET_CMP
    if (g_build_options.b_net_light)

        // net_light ();
        lc_net::net_lightmaps();
    else
    {
        LMapsLocal();
    }
#else
    create_net_task_manager();
    get_net_task_manager()->create_global_data_write(pBuild->path);
    LMapsLocal();
    get_net_task_manager()->run();
    destroy_net_task_manager();
// net_light ();
#endif
}
void XRLC_LIGHT_API ImplicitNetWait();
void CBuild::Light()
{
    //****************************************** Implicit
    {
        FPU::m64r();
        Logger.Phase("LIGHT: Implicit...");
        mem_Compact();
        ImplicitLighting();
    }

    LMaps();

    //****************************************** Vertex
    FPU::m64r();
    Logger.Phase("LIGHT: Vertex...");
    mem_Compact();

    LightVertex();

    //

    ImplicitNetWait();
    WaitMuModelsLocalCalcLightening();
    lc_net::get_task_manager().wait_all();
    //	get_task_manager().wait_all();
    lc_net::get_task_manager().release();
    //
    //****************************************** Merge LMAPS
    {
        FPU::m64r();
        Logger.Phase("LIGHT: Merging lightmaps...");
        mem_Compact();

        xrPhase_MergeLM();
    }
}

void CBuild::LightVertex() { ::LightVertex(!!g_build_options.b_net_light); }
