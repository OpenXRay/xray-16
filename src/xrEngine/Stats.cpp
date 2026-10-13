#include "stdafx.h"
#include "GameFont.h"
#pragma hdrstop

#include "xrCDB/ISpatial.h"
#include "IGame_Persistent.h"
#include "IGame_Level.h"
#include "Render.h"
#include "xr_object.h"

#include "Include/xrRender/DrawUtils.h" // for CStats::OnRender
#include "xr_input.h"
#include "xrCore/cdecl_cast.hpp"
#include "PerformanceAlert.hpp"
#include "xrCore/Threading/TaskManager.hpp"
#include <imgui.h>
#include <algorithm>
#include <cmath>

int g_ErrorLineCount = 15;
Flags32 g_stats_flags = {};

ENGINE_API CStatTimer gTestTimer0;
ENGINE_API CStatTimer gTestTimer1;
ENGINE_API CStatTimer gTestTimer2;
ENGINE_API CStatTimer gTestTimer3;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
ENGINE_API bool g_bDisableRedText = false;
int g_bShowRedText = 1;
CStats::CStats()
{
    statsFont = nullptr;
    Device.seqRender.Add(this, REG_PRIORITY_LOW - 1000);
}

CStats::~CStats()
{
    Device.seqRender.Remove(this);
    xr_delete(statsFont);
}

static void DumpTaskManagerStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    size_t allocated{}, pushed{}, finished{};
    TaskScheduler->GetStats(allocated, pushed, finished);

    static size_t allocatedPrev{};
    static size_t pushedPrev{};
    static size_t finishedPrev{};

    font.OutNext("Task scheduler:    ");
    font.OutNext("- threads:       %zu", TaskScheduler->GetWorkersCount());
    font.OutNext("- tasks:           ");
    font.OutNext("  - total:         ");
    font.OutNext("    - allocated: %zu", allocated);
    font.OutNext("    - pushed:    %zu", pushed);
    font.OutNext("    - finished:  %zu", finished);
    font.OutNext("  - this frame:    ");
    font.OutNext("    - allocated: %zu", allocated - allocatedPrev);
    font.OutNext("    - pushed     %zu", pushed - pushedPrev);
    font.OutNext("    - finished:  %zu", finished - finishedPrev);

    allocatedPrev = allocated;
    pushedPrev = pushed;
    finishedPrev = finished;
}

// XXX: move to IGame_Persistent
static void DumpSpatialStatistics(IGameFont& font, IPerformanceAlert* alert, ISpatial_DB& db, float engineTotal)
{
#ifdef DEBUG
    auto& stats = db.Stats;
    stats.FrameEnd();
#define PPP(a) (100.f * float(a) / engineTotal)
    font.OutNext("%s:", db.Name);
    font.OutNext("- query:      %.2fms, %u", stats.Query.result, stats.Query.count);
    font.OutNext("- nodes/obj:  %u/%u", stats.NodeCount, stats.ObjectCount);
    font.OutNext("- insert:     %.2fms, %2.1f%%", stats.Insert.result, PPP(stats.Insert.result));
    font.OutNext("- remove:     %.2fms, %2.1f%%", stats.Remove.result, PPP(stats.Remove.result));
#undef PPP
    stats.FrameStart();
#endif
}

void CStats::Show()
{
    gTestTimer0.FrameEnd();
    gTestTimer1.FrameEnd();
    gTestTimer2.FrameEnd();
    gTestTimer3.FrameEnd();

    if (GEnv.isDedicatedServer)
        return;
    auto& font = *statsFont;
    auto engineTotal = Device.GetStats().EngineTotal.result;
    PerformanceAlert alertInstance(font.GetHeight(), {300, 300});
    auto alertPtr = g_bDisableRedText ? nullptr : &alertInstance;

    // Show them
    if (psDeviceFlags.test(rsStatistic))
    {
        font.SetColor(0xFFFFFFFF);
        font.OutSet(0, 0);
#if defined(FS_DEBUG)
        font.OutNext("Mapped:       %d", g_file_mapped_memory);
#endif
        Device.DumpStatistics(font, alertPtr);
        if (g_pGameLevel)
            g_pGameLevel->DumpStatistics(font, alertPtr);
        Engine.Sheduler.DumpStatistics(font, alertPtr);
        DumpTaskManagerStatistics(font, alertPtr);
        if (g_pGamePersistent)
        {
            g_pGamePersistent->DumpStatistics(font, alertPtr);
            DumpSpatialStatistics(font, alertPtr, g_pGamePersistent->SpatialSpace, engineTotal);
            DumpSpatialStatistics(font, alertPtr, g_pGamePersistent->SpatialSpacePhysic, engineTotal);
        }
        font.OutSet(200, 0);
        GEnv.Render->DumpStatistics(font, alertPtr);
        font.OutSkip();
        GEnv.Sound->DumpStatistics(font, alertPtr);
        font.OutSkip();
        pInput->DumpStatistics(font, alertPtr);
        font.OutSkip();
        font.OutNext("TEST 0:      %2.2fms, %d", gTestTimer0.result, gTestTimer0.count);
        font.OutNext("TEST 1:      %2.2fms, %d", gTestTimer1.result, gTestTimer1.count);
        font.OutNext("TEST 2:      %2.2fms, %d", gTestTimer2.result, gTestTimer2.count);
        font.OutNext("TEST 3:      %2.2fms, %d", gTestTimer3.result, gTestTimer3.count);
        font.OutSkip();
        font.OutNext("QPC: %u", CPU::qpc_counter);
        CPU::qpc_counter = 0;
    }
    if (psDeviceFlags.test(rsCameraPos))
    {
        float refHeight = font.GetHeight();
        font.SetHeightI(0.02f);
        font.SetColor(0xffffffff);
        font.Out(10, 600, "CAMERA POSITION:  [%3.2f,%3.2f,%3.2f]", VPUSH(Device.vCameraPosition));
        font.SetHeight(refHeight);
    }
#ifdef DEBUG
    if (!g_bDisableRedText && errors.size() && g_bShowRedText)
    {
        font.SetColor(color_rgba(255, 16, 16, 191));
        font.OutSet(400, 0);

        for (u32 it = (u32)_max(int(0), (int)errors.size() - g_ErrorLineCount); it < errors.size(); it++)
            font.OutNext("%s", errors[it].c_str());
    }
#endif
    font.OnRender();

    gTestTimer0.FrameStart();
    gTestTimer1.FrameStart();
    gTestTimer2.FrameStart();
    gTestTimer3.FrameStart();
}

void CStats::ResetFPSOverlay()
{
    fpsHistoryWrite = 0;
    fpsHistoryCount = 0;
    fpsFrameCount = 0;
    fpsElapsed = 0.0;
    fpsSampleTime = 0.0;
    fpsSampleFrames = 0;
    fpsAverage = 0.f;
    fpsMinimum = 0.f;
    fpsMaximum = 0.f;
}

void CStats::RenderFPSOverlay()
{
    if (GEnv.isDedicatedServer || !psDeviceFlags.test(rsShowFPS | rsShowFPSGraph))
    {
        if (fpsFrameCount != 0)
            ResetFPSOverlay();
        return;
    }

    const float frameTime = Device.fTimeDeltaReal;
    const bool validTiming = frameTime > 0.f && std::isfinite(frameTime);
    if (validTiming)
    {
        fpsElapsed += frameTime;
        ++fpsFrameCount;
        fpsSampleTime += frameTime;
        ++fpsSampleFrames;
        if (fpsSampleTime >= 0.25)
        {
            const float fps = static_cast<float>(fpsSampleFrames / fpsSampleTime);
            fpsMinimum = fpsHistoryCount == 0 ? fps : std::min(fpsMinimum, fps);
            fpsMaximum = std::max(fpsMaximum, fps);
            fpsAverage = static_cast<float>(fpsFrameCount / fpsElapsed);
            fpsHistory[fpsHistoryWrite] = {fpsElapsed, fps};
            fpsHistoryWrite = (fpsHistoryWrite + 1) % fpsHistorySize;
            fpsHistoryCount = std::min(fpsHistoryCount + 1, fpsHistorySize);
            fpsSampleTime = 0.0;
            fpsSampleFrames = 0;
        }
    }
    else
    {
        fpsSampleTime = 0.0;
        fpsSampleFrames = 0;
    }

    auto* viewport = ImGui::GetMainViewport();
    const float fontSize = ImGui::GetFontSize();
    const float padding = fontSize * 0.75f;
    const float lineHeight = fontSize * 1.5f;
    const float width = std::min(fontSize * 29.f, viewport->Size.x - padding * 2.f);
    const float height = padding * 2.f + lineHeight * 4.f + fontSize * 7.f;
    if (width <= padding * 2.f + fontSize * 3.f || viewport->Size.y <= padding * 2.f)
        return;

    const ImVec2 topLeft(viewport->Pos.x + viewport->Size.x - padding - width, viewport->Pos.y + padding);
    const ImVec2 bottomRight(topLeft.x + width, topLeft.y + height);
    auto* drawList = ImGui::GetForegroundDrawList(viewport);
    drawList->PushClipRect(topLeft,
        ImVec2(bottomRight.x, std::min(bottomRight.y, viewport->Pos.y + viewport->Size.y - padding)), true);
    drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(15, 18, 24, 225), padding * 0.4f);

    const ImU32 textColor = IM_COL32(230, 234, 240, 255);
    const ImU32 mutedColor = IM_COL32(145, 155, 172, 255);
    const ImU32 graphColor = IM_COL32(90, 210, 225, 255);
    const ImU32 averageColor = IM_COL32(225, 180, 90, 210);
    const float textX = topLeft.x + padding;
    const float rightX = bottomRight.x - padding;
    char text[128];
    if (fpsHistoryCount != 0 && validTiming)
    {
        const float fps = fpsHistory[(fpsHistoryWrite + fpsHistorySize - 1) % fpsHistorySize].fps;
        xr_sprintf(text, sizeof(text), "%.1f FPS", fps);
        drawList->AddText(ImVec2(textX, topLeft.y + padding), graphColor, text);
        xr_sprintf(text, sizeof(text), "%.2f ms", 1000.f / fps);
        drawList->AddText(ImVec2(rightX - ImGui::CalcTextSize(text).x, topLeft.y + padding), textColor, text);
    }
    else
        drawList->AddText(ImVec2(textX, topLeft.y + padding), mutedColor, "FPS  measuring...");

    if (fpsHistoryCount != 0)
        xr_sprintf(text, sizeof(text), "AVG %.1f   MIN %.1f   MAX %.1f", fpsAverage, fpsMinimum, fpsMaximum);
    else
        xr_sprintf(text, sizeof(text), "AVG --   MIN --   MAX --");
    drawList->AddText(ImVec2(textX, topLeft.y + padding + lineHeight), textColor, text);

    const ImVec2 plotMin(textX + fontSize * 3.f, topLeft.y + padding + lineHeight * 2.f);
    const ImVec2 plotMax(rightX, plotMin.y + fontSize * 7.f);
    const float plotWidth = plotMax.x - plotMin.x;
    const float plotHeight = plotMax.y - plotMin.y;
    const float graphMax = std::max(120.f, std::ceil(fpsMaximum / 60.f) * 60.f);
    drawList->AddRectFilled(plotMin, plotMax, IM_COL32(5, 8, 12, 170));
    for (int i = 0; i <= 4; ++i)
    {
        const float y = plotMax.y - i * plotHeight / 4.f;
        drawList->AddLine(ImVec2(plotMin.x, y), ImVec2(plotMax.x, y), IM_COL32(115, 130, 150, 45));
        xr_sprintf(text, sizeof(text), "%.0f", graphMax * i / 4.f);
        drawList->AddText(ImVec2(plotMin.x - padding * 0.5f - ImGui::CalcTextSize(text).x,
            y - fontSize * 0.5f), mutedColor, text);
    }
    for (int i = 0; i <= 4; ++i)
    {
        const float x = plotMin.x + i * plotWidth / 4.f;
        drawList->AddLine(ImVec2(x, plotMin.y), ImVec2(x, plotMax.y), IM_COL32(115, 130, 150, 35));
    }
    drawList->AddText(ImVec2(plotMin.x, plotMax.y + fontSize * 0.3f), mutedColor, "-10 s");
    drawList->AddText(ImVec2(plotMin.x + plotWidth * 0.5f - ImGui::CalcTextSize("-5 s").x * 0.5f,
        plotMax.y + fontSize * 0.3f), mutedColor, "-5 s");
    drawList->AddText(ImVec2(plotMax.x - ImGui::CalcTextSize("now").x, plotMax.y + fontSize * 0.3f), mutedColor, "now");

    if (fpsHistoryCount != 0)
    {
        std::array<ImVec2, fpsHistorySize + 1> points;
        int pointCount = 0;
        const double startTime = fpsElapsed - 10.0;
        const u32 oldest = (fpsHistoryWrite + fpsHistorySize - fpsHistoryCount) % fpsHistorySize;
        for (u32 i = 0; i < fpsHistoryCount; ++i)
        {
            if (i + 1 < fpsHistoryCount && fpsHistory[(oldest + i + 1) % fpsHistorySize].time < startTime)
                continue;
            const auto& sample = fpsHistory[(oldest + i) % fpsHistorySize];
            points[pointCount++] = ImVec2(plotMin.x + static_cast<float>((sample.time - startTime) / 10.0) * plotWidth,
                plotMax.y - sample.fps / graphMax * plotHeight);
        }
        points[pointCount] = ImVec2(plotMax.x, points[pointCount - 1].y);
        ++pointCount;
        drawList->PushClipRect(plotMin, plotMax, true);
        for (int i = 1; i < pointCount; ++i)
            drawList->AddQuadFilled(points[i - 1], points[i],
                ImVec2(points[i].x, plotMax.y), ImVec2(points[i - 1].x, plotMax.y), IM_COL32(90, 210, 225, 28));
        const float averageY = plotMax.y - fpsAverage / graphMax * plotHeight;
        drawList->AddLine(ImVec2(plotMin.x, averageY), ImVec2(plotMax.x, averageY), averageColor);
        drawList->AddPolyline(points.data(), pointCount, graphColor, ImDrawFlags_None, 1.5f);
        drawList->AddCircleFilled(points[pointCount - 1], 2.f, graphColor);
        drawList->PopClipRect();
    }
    xr_sprintf(text, sizeof(text), "Session %.1fs   /   250ms samples", fpsElapsed);
    drawList->AddText(ImVec2(textX, plotMax.y + lineHeight), mutedColor, text);
    drawList->PopClipRect();
}

void CStats::OnDeviceCreate()
{
    g_bDisableRedText = !!strstr(Core.Params, "-xclsx");

    if (!GEnv.isDedicatedServer)
    {
        statsFont = xr_new<CGameFont>("stat_font", CGameFont::fsDeviceIndependent);
    }

#ifdef DEBUG
    if (!g_bDisableRedText)
    {
        auto cb = [](void* ctx, const char* s) {
            auto& stats = *static_cast<CStats*>(ctx);
            stats.FilteredLog(s);
        };
        SetLogCB(LogCallback(cdecl_cast(cb), this));
    }
#endif
}

void CStats::OnDeviceDestroy()
{
    SetLogCB(nullptr);
    xr_delete(statsFont);
    ResetFPSOverlay();
}

void CStats::FilteredLog(const char* s)
{
    if (s && s[0] == '!' && s[1] == ' ')
        errors.push_back(shared_str(s));
}

void CStats::OnRender()
{
// XXX: move to xrSound
#ifdef DEBUG
    if (g_stats_flags.is(st_sound))
    {
        CSound_stats_ext snd_stat_ext;
        GEnv.Sound->statistic(0, &snd_stat_ext);
        auto _I = snd_stat_ext.items.begin();
        auto _E = snd_stat_ext.items.end();
        for (; _I != _E; ++_I)
        {
            const CSound_stats_ext::SItem& item = *_I;
            if (item._3D)
            {
                GEnv.DU->DrawCross(item.params.position, 0.5f, 0xFF0000FF, true);
                if (g_stats_flags.is(st_sound_min_dist))
                    GEnv.DU->DrawSphere(
                        Fidentity, item.params.position, item.params.min_distance, 0x400000FF, 0xFF0000FF, true, true);
                if (g_stats_flags.is(st_sound_max_dist))
                    GEnv.DU->DrawSphere(
                        Fidentity, item.params.position, item.params.max_distance, 0x4000FF00, 0xFF008000, true, true);

                xr_string out_txt = (out_txt.size() && g_stats_flags.is(st_sound_info_name)) ? item.name.c_str() : "";

                if (item.game_object)
                {
                    if (g_stats_flags.is(st_sound_ai_dist))
                        GEnv.DU->DrawSphere(Fidentity, item.params.position, item.params.max_ai_distance,
                            0x80FF0000, 0xFF800000, true, true);
                    if (g_stats_flags.is(st_sound_info_object))
                    {
                        out_txt += " (";
                        out_txt += item.game_object->cNameSect().c_str();
                        out_txt += ")";
                    }
                }
                if (g_stats_flags.is_any(st_sound_info_name | st_sound_info_object) && item.name.size())
                    GEnv.DU->OutText(item.params.position, out_txt.c_str(), 0xFFFFFFFF, 0xFF000000);
            }
        }
    }
#endif
}
