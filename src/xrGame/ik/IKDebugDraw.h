#pragma once

#include "xrAnimation/OzzLimbSolver.h"
#include "xrCore/_flags.h"
#include "xrCore/Threading/Lock.hpp"

#include <atomic>

enum EIKDebugDraw : u32
{
    ikDebugSolver = 1u << 0,
    ikDebugTargets = 1u << 1,
    ikDebugChains = 1u << 2,
};

extern Flags32 ps_ik_debug;
extern float ps_ik_debug_distance;
extern float ps_ik_debug_size;

class CIKDebugData
{
public:
    CIKDebugData();

    XRay::Animation::OzzLimbSolver::DebugData solver;
    Fmatrix object;
    Fmatrix start;
    Fmatrix goal;
    Fvector animated[3];
    Fvector resolved[3];
    Fvector knee;
    float length;
    bool solved;
    u32 frame;
};

class CIKDebugDraw
{
public:
    CIKDebugDraw();

    bool Enabled() const;
    void Capture(const CIKDebugData& data);
    void Render();
    void Clear();

private:
    bool Valid(const CIKDebugData& data) const;
    bool ValidSolver(const CIKDebugData& data) const;
    Fvector ToWorld(const CIKDebugData& data, const Fvector& modelPoint) const;
    Fvector ToWorldDirection(const CIKDebugData& data, const Fvector& modelDirection) const;
    void DrawSegment(const Fvector& from, const Fvector& to, u32 color) const;
    void DrawMarker(const Fvector& worldPoint, float size, u32 color) const;
    void DrawAxes(const CIKDebugData& data, const Fmatrix& model, float size) const;
    void DrawChain(const CIKDebugData& data, const Fvector (&modelPoints)[3], u32 upperColor, u32 lowerColor,
        float size) const;
    void DrawRing(const Fmatrix& object, const Fvector& modelCenter, float radius, u32 plane, u32 color) const;
    void DrawChains(const CIKDebugData& data, float size) const;
    void DrawTargets(const CIKDebugData& data, float size) const;
    void DrawSolver(const CIKDebugData& data, float size) const;

    Lock m_lock;
    CIKDebugData m_snapshot;
    bool m_valid = false;
    std::atomic<bool> m_enabled{false};
};
