#include "StdAfx.h"
#include "IKDebugDraw.h"

#include "Include/xrRender/DrawUtils.h"
#include "xrCore/Threading/ScopeLock.hpp"
#include "xrCore/math_constants.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/IGame_Persistent.h"

#include <cmath>

namespace
{
constexpr u32 ikChainAnimatedUpper = color_rgba(0, 210, 255, 255);
constexpr u32 ikChainAnimatedLower = color_rgba(40, 90, 255, 255);
constexpr u32 ikChainResolvedSolved = color_rgba(0, 255, 80, 255);
constexpr u32 ikChainResolvedRejected = color_rgba(255, 40, 40, 255);
constexpr u32 ikRequestedGoal = color_rgba(255, 255, 0, 255);
constexpr u32 ikClampedTarget = color_rgba(255, 140, 0, 255);
constexpr u32 ikReachRing = color_rgba(0, 140, 255, 96);
constexpr u32 ikPoleDirection = color_rgba(255, 0, 255, 255);
constexpr u32 ikHipToTarget = color_rgba(200, 200, 200, 255);
constexpr u32 ikAxisX = color_rgba(255, 0, 0, 255);
constexpr u32 ikAxisY = color_rgba(0, 255, 0, 255);
constexpr u32 ikAxisZ = color_rgba(0, 0, 255, 255);
constexpr u32 ikRingSegments = 24;
constexpr float ikAxisScale = 8.f;
constexpr float ikGoalAxisScale = 6.f;
constexpr float ikKneeMarkerScale = 1.5f;
constexpr float ikPoleLengthScale = 10.f;
constexpr u32 ikFreshFrameTolerance = 1u;
}

Flags32 ps_ik_debug{};
float ps_ik_debug_distance = 30.f;
float ps_ik_debug_size = .05f;

CIKDebugData::CIKDebugData()
{
    object.identity();
    start.identity();
    goal.identity();
    for (u32 i = 0; i < 3; ++i)
    {
        animated[i].set(0.f, 0.f, 0.f);
        resolved[i].set(0.f, 0.f, 0.f);
    }
    knee.set(0.f, 0.f, 0.f);
    length = 0.f;
    solved = false;
    frame = 0;
}

CIKDebugDraw::CIKDebugDraw()
#ifdef CONFIG_PROFILE_LOCKS
    : m_lock(MUTEX_PROFILE_ID(CIKDebugDraw))
#endif
{
}

bool CIKDebugDraw::Enabled() const
{
    return m_enabled.load(std::memory_order_relaxed);
}

void CIKDebugDraw::Capture(const CIKDebugData& data)
{
    if (!m_enabled.load(std::memory_order_relaxed))
        return;

    ScopeLock lock(&m_lock);
    if (!m_enabled.load(std::memory_order_relaxed))
        return;

    m_snapshot = data;
    m_valid = true;
}

void CIKDebugDraw::Clear()
{
    m_enabled.store(false, std::memory_order_relaxed);

    ScopeLock lock(&m_lock);
    m_valid = false;
}

void CIKDebugDraw::Render()
{
    const u32 flags = ps_ik_debug.get();
    const bool enabled = flags != 0 && GEnv.DU != nullptr && Device.b_is_Ready && Device.b_is_Active
        && !GEnv.isDedicatedServer && g_pGameLevel != nullptr && g_pGameLevel->bReady
        && (g_pGamePersistent == nullptr || !g_pGamePersistent->IsMainMenuActive());

    const bool wasEnabled = m_enabled.exchange(enabled, std::memory_order_relaxed);

    if (!enabled)
    {
        if (wasEnabled)
        {
            ScopeLock lock(&m_lock);
            m_valid = false;
        }
        return;
    }

    CIKDebugData data;
    {
        ScopeLock lock(&m_lock);
        if (!m_valid)
            return;

        if (!Device.Paused() && (Device.dwFrame - m_snapshot.frame) > ikFreshFrameTolerance)
        {
            m_valid = false;
            return;
        }

        data = m_snapshot;
    }

    const float size = ps_ik_debug_size;
    const float maxDistance = ps_ik_debug_distance;
    if (!std::isfinite(size) || size <= 0.f)
        return;
    if (!std::isfinite(maxDistance) || maxDistance <= 0.f)
        return;
    if (!Valid(data))
        return;

    const Fvector hip = ToWorld(data, data.start.c);
    const float distanceSquared = hip.distance_to_sqr(Device.vCameraPosition);
    if (!_valid(hip) || !_valid(distanceSquared) || distanceSquared > maxDistance * maxDistance)
        return;

    if ((flags & ikDebugSolver) != 0 && ValidSolver(data))
        DrawSolver(data, size);
    if ((flags & ikDebugTargets) != 0)
        DrawTargets(data, size);
    if ((flags & ikDebugChains) != 0)
        DrawChains(data, size);
}

bool CIKDebugDraw::Valid(const CIKDebugData& data) const
{
    if (!_valid(data.object) || !_valid(data.start) || !_valid(data.goal) || !_valid(data.knee))
        return false;

    for (u32 i = 0; i < 3; ++i)
    {
        if (!_valid(data.animated[i]) || !_valid(data.resolved[i]))
            return false;
    }

    return true;
}

bool CIKDebugDraw::ValidSolver(const CIKDebugData& data) const
{
    if (data.solver.hasTarget && !_valid(data.solver.target))
        return false;
    if (data.solver.hasPole && !_valid(data.solver.pole))
        return false;

    return true;
}

Fvector CIKDebugDraw::ToWorld(const CIKDebugData& data, const Fvector& modelPoint) const
{
    Fvector world;
    data.object.transform_tiny(world, modelPoint);
    return world;
}

Fvector CIKDebugDraw::ToWorldDirection(const CIKDebugData& data, const Fvector& modelDirection) const
{
    Fvector world;
    data.object.transform_dir(world, modelDirection);
    world.normalize_safe();
    return world;
}

void CIKDebugDraw::DrawSegment(const Fvector& from, const Fvector& to, u32 color) const
{
    if (_valid(from) && _valid(to))
    {
        GEnv.DU->DrawLine(from, to, color);
    }
}

void CIKDebugDraw::DrawMarker(const Fvector& worldPoint, float size, u32 color) const
{
    if (_valid(worldPoint) && _valid(size) && size > 0.f)
    {
        GEnv.DU->DrawCross(worldPoint, size, color);
    }
}

void CIKDebugDraw::DrawAxes(const CIKDebugData& data, const Fmatrix& model, float size) const
{
    Fmatrix world;
    world.mul_43(data.object, model);

    Fvector end;
    end.mad(world.c, world.i, size);
    DrawSegment(world.c, end, ikAxisX);
    end.mad(world.c, world.j, size);
    DrawSegment(world.c, end, ikAxisY);
    end.mad(world.c, world.k, size);
    DrawSegment(world.c, end, ikAxisZ);
}

void CIKDebugDraw::DrawChain(const CIKDebugData& data, const Fvector (&modelPoints)[3], u32 upperColor,
    u32 lowerColor, float size) const
{
    const Fvector first = ToWorld(data, modelPoints[0]);
    const Fvector second = ToWorld(data, modelPoints[1]);
    const Fvector third = ToWorld(data, modelPoints[2]);

    DrawSegment(first, second, upperColor);
    DrawSegment(second, third, lowerColor);
    DrawMarker(first, size, upperColor);
    DrawMarker(second, size, lowerColor);
    DrawMarker(third, size, upperColor);
}

void CIKDebugDraw::DrawRing(const Fmatrix& object, const Fvector& modelCenter, float radius, u32 plane, u32 color) const
{
    Fvector previous;
    previous.set(0.f, 0.f, 0.f);

    for (u32 i = 0; i <= ikRingSegments; ++i)
    {
        const float angle = PI_MUL_2 * float(i) / float(ikRingSegments);
        const float c = _cos(angle) * radius;
        const float s = _sin(angle) * radius;

        Fvector model;
        switch (plane)
        {
        case 0:
            model.set(modelCenter.x + c, modelCenter.y + s, modelCenter.z);
            break;
        case 1:
            model.set(modelCenter.x + c, modelCenter.y, modelCenter.z + s);
            break;
        default:
            model.set(modelCenter.x, modelCenter.y + c, modelCenter.z + s);
            break;
        }

        Fvector world;
        object.transform_tiny(world, model);

        if (i != 0)
            DrawSegment(previous, world, color);

        previous = world;
    }
}

void CIKDebugDraw::DrawChains(const CIKDebugData& data, float size) const
{
    DrawChain(data, data.animated, ikChainAnimatedUpper, ikChainAnimatedLower, size);

    const u32 resolvedColor = data.solved ? ikChainResolvedSolved : ikChainResolvedRejected;
    DrawChain(data, data.resolved, resolvedColor, resolvedColor, size);
}

void CIKDebugDraw::DrawTargets(const CIKDebugData& data, float size) const
{
    const Fvector goal = ToWorld(data, data.goal.c);
    DrawMarker(goal, size, ikRequestedGoal);
    DrawAxes(data, data.goal, size * ikGoalAxisScale);

    if (!data.solver.hasTarget || !_valid(data.solver.target))
        return;

    const Fvector target = ToWorld(data, data.solver.target);

    Fvector offset;
    offset.sub(target, goal);
    if (offset.square_magnitude() > EPS_L * EPS_L)
    {
        DrawMarker(target, size, ikClampedTarget);
        DrawSegment(goal, target, ikClampedTarget);
    }

    if (_valid(data.resolved[2]))
        DrawSegment(ToWorld(data, data.resolved[2]), target, ikClampedTarget);
}

void CIKDebugDraw::DrawSolver(const CIKDebugData& data, float size) const
{
    const Fvector hip = ToWorld(data, data.start.c);
    DrawAxes(data, data.start, size * ikAxisScale);

    if (std::isfinite(data.length) && data.length > 0.f)
    {
        DrawRing(data.object, data.start.c, data.length, 0, ikReachRing);
        DrawRing(data.object, data.start.c, data.length, 1, ikReachRing);
        DrawRing(data.object, data.start.c, data.length, 2, ikReachRing);
    }

    if (_valid(data.knee))
        DrawMarker(ToWorld(data, data.knee), size * ikKneeMarkerScale, ikPoleDirection);

    if (data.solver.hasPole)
    {
        const Fvector pole = ToWorldDirection(data, data.solver.pole);
        Fvector poleEnd;
        poleEnd.mad(hip, pole, size * ikPoleLengthScale);
        DrawSegment(hip, poleEnd, ikPoleDirection);
    }

    if (data.solver.hasTarget && _valid(data.solver.target))
        DrawSegment(hip, ToWorld(data, data.solver.target), ikHipToTarget);
}
