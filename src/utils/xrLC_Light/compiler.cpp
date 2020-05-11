#include "stdafx.h"
#include "Common/LevelStructure.hpp"
#include "utils/xrLCUtil/xrThread.hpp"

#include "global_calculation_data.h"
#include "lightthread.h"
#include "xrLightDoNet.h"

void xrLight()
{
    u32 range = gl_data.slots_data.size_z();

    // Start threads, wait, continue --- perform all the work
    CThreadManager Threads(ProxyStatus, ProxyProgress);
    CTimer start_time;
    start_time.Start();
    u32 stride = range / NUM_THREADS;
    u32 last = range - stride * (NUM_THREADS - 1);
    for (u32 thID = 0; thID < NUM_THREADS; thID++)
    {
        CThread* T =
            xr_new<LightThread>(thID, thID * stride, thID * stride + ((thID == (NUM_THREADS - 1)) ? last : stride));
        T->thMessages = FALSE;
        T->thMonitor = FALSE;
        Threads.start(T);
    }
    Threads.wait();
    Msg("%d seconds elapsed.", (start_time.GetElapsed_ms()) / 1000);
}

void xrCompileDO(bool net)
{
    Logger.Phase("Loading level...");
    gl_data.xrLoad();

    Logger.Phase("Lighting nodes...");
    if (net)
        lc_net::xrNetDOLight();
    else
        xrLight();

    gl_data.slots_data.Free();
}
