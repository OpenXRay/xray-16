// xrXRC.cpp: implementation of the xrXRC class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrXRC.h"
#include "xrEngine/IGameFont.hpp"
#include "xrEngine/IPerformanceAlert.hpp"

XRCDB_API xrXRC XRC("global");

void xrXRC::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    Stats.FrameEnd();
    font.OutNext("XRC (%s):", name);
    font.OutNext("- ray:        %2.2fms, %d, %2.0fK", Stats.RayQuery.result, Stats.RayQuery.count, Stats.RayPs);
    font.OutNext("- box:        %2.2fms, %d, %2.0fK", Stats.BoxQuery.result, Stats.BoxQuery.count, Stats.BoxPs);
    font.OutNext("- frustum:    %2.2fms, %d", Stats.FrustumQuery.result, Stats.FrustumQuery.count);
    Stats.FrameStart();
}
