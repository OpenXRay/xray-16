#include "stdafx.h"
#include "GpuParticleTranslate.h"
#include "ParticleEffectDef.h"
#include "xrParticles/psystem.h"
#include "xrParticles/particle_actions_collection.h"
#include "xrParticles/particle_manager.h"
#include <cmath>
#include <cstring>
#include <limits>

namespace xray::render::fg
{
namespace
{
using namespace PAPI;

Fvector4 Vector(const Fvector& value, float w = 0.f)
{
    Fvector4 result;
    result.set(value.x, value.y, value.z, w);
    return result;
}

GpuPapiDomain Domain(const pDomain& source)
{
    GpuPapiDomain result{};
    result.type = source.type;
    result.p1 = Vector(source.p1);
    switch (source.type)
    {
    case PDPoint: break;
    case PDLine:
    case PDBox: result.p2 = Vector(source.p2); break;
    case PDPlane:
        result.p2 = Vector(source.p2);
        result.radii.x = source.radius1;
        break;
    case PDSphere:
        result.radii.set(source.radius1, source.radius2, source.radius1Sqr, source.radius2Sqr);
        break;
    case PDBlob:
        result.radii.set(source.radius1, source.radius2, 0.f, source.radius2Sqr);
        break;
    case PDTriangle:
    case PDRectangle:
        result.p2 = Vector(source.p2);
        result.u = Vector(source.u);
        result.v = Vector(source.v);
        result.radii.set(source.radius1, 0.f, source.radius1Sqr, source.radius2Sqr);
        break;
    case PDCylinder:
    case PDCone:
        result.p2 = Vector(source.p2);
        result.u = Vector(source.u);
        result.v = Vector(source.v);
        result.radii.set(source.radius1, source.radius2, source.radius1Sqr, source.radius2Sqr);
        break;
    case PDDisc:
        result.p2 = Vector(source.p2);
        result.u = Vector(source.u);
        result.v = Vector(source.v);
        result.radii.set(source.radius1, source.radius2, source.radius1Sqr, 0.f);
        break;
    }
    return result;
}

bool Fail(xr_string& error, const PS::CPEDef& definition, size_t index, const char* reason)
{
    char message[512];
    xr_sprintf(message, "GPU PAPI definition '%s', action %zu: %s", definition.Name(), index, reason);
    error = message;
    return false;
}

bool CheckStream(const PS::CPEDef& definition, xr_string& error)
{
    const auto* bytes = static_cast<const u8*>(definition.m_Actions.pointer());
    const size_t length = definition.m_Actions.size();
    if (!length)
        return true;
    if (length < sizeof(u32))
        return Fail(error, definition, 0, "truncated action count");
    u32 count;
    std::memcpy(&count, bytes, sizeof(count));
    size_t offset = sizeof(u32);
    for (u32 index = 0; index < count; ++index)
    {
        if (length - offset < 3 * sizeof(u32))
            return Fail(error, definition, index, "truncated action header");
        u32 type, storedType;
        std::memcpy(&type, bytes + offset, sizeof(type));
        std::memcpy(&storedType, bytes + offset + 2 * sizeof(u32), sizeof(storedType));
        const bool rotateAlias = (type == PATargetRotateID || type == PATargetRotateDID) &&
            (storedType == PATargetRotateID || storedType == PATargetRotateDID);
        const bool velocityAlias = (type == PATargetVelocityID || type == PATargetVelocityDID) &&
            (storedType == PATargetVelocityID || storedType == PATargetVelocityDID);
        if (type != storedType && !rotateAlias && !velocityAlias)
            return Fail(error, definition, index, "action allocation and payload type disagree");
        offset += 3 * sizeof(u32);
        size_t payload = 0;
        switch (type)
        {
        case PAAvoidID:
        case PABounceID: payload = sizeof(pDomain) + 12; break;
        case PACopyVertexBID:
        case PARestoreID: payload = 4; break;
        case PADampingID: payload = 20; break;
        case PAExplosionID: payload = 32; break;
        case PAFollowID:
        case PAGravitateID:
        case PAGravityID:
        case PAMatchVelocityID: payload = 12; break;
        case PAJetID: payload = sizeof(pDomain) + 24; break;
        case PAKillOldID:
        case PASpeedLimitID: payload = 8; break;
        case PAMoveID: break;
        case PAOrbitLineID:
        case PAVortexID: payload = 36; break;
        case PAOrbitPointID:
        case PAScatterID:
        case PATargetSizeID: payload = 24; break;
        case PARandomAccelID:
        case PARandomDisplaceID:
        case PARandomVelocityID: payload = sizeof(pDomain); break;
        case PASinkID:
        case PASinkVelocityID: payload = sizeof(pDomain) + 4; break;
        case PASourceID: payload = 5 * sizeof(pDomain) + 32; break;
        case PATargetColorID: payload = ShadowOfChernobylMode ? 20 : 28; break;
        case PATargetRotateID:
        case PATargetRotateDID:
        case PATargetVelocityID:
        case PATargetVelocityDID: payload = 16; break;
        case PATurbulenceID: payload = 28; break;
        case PACallActionListID_obsolette:
            return Fail(error, definition, index, "obsolete CallActionList is not executable by the CPU loader");
        default: return Fail(error, definition, index, "unknown action type");
        }
        if (payload > length - offset)
            return Fail(error, definition, index, "truncated action payload");
        offset += payload;
    }
    if (offset != length)
        return Fail(error, definition, count, "unexpected bytes after action stream");
    return true;
}

bool Finite(const Fvector4& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}

bool ValidDomain(const GpuPapiDomain& domain, bool generate)
{
    if (domain.type > PDRectangle || !Finite(domain.p1))
        return false;
    if (!generate)
    {
        if (domain.type == PDBlob)
            return std::isfinite(domain.radii.x) && !std::isnan(domain.radii.y) && !std::isnan(domain.radii.w);
        return Finite(domain.p2) && Finite(domain.u) && Finite(domain.v) && Finite(domain.radii);
    }
    switch (domain.type)
    {
    case PDPoint:
    case PDPlane: return true;
    case PDLine:
    case PDBox: return Finite(domain.p2);
    case PDTriangle:
    case PDRectangle: return Finite(domain.u) && Finite(domain.v);
    case PDSphere: return std::isfinite(domain.radii.x) && std::isfinite(domain.radii.y);
    case PDBlob: return std::isfinite(domain.radii.x);
    case PDCylinder:
    case PDCone:
        return Finite(domain.p2) && Finite(domain.u) && Finite(domain.v) &&
            std::isfinite(domain.radii.x) && std::isfinite(domain.radii.y);
    case PDDisc:
        return Finite(domain.u) && Finite(domain.v) && std::isfinite(domain.radii.x) && std::isfinite(domain.radii.y);
    default: return false;
    }
}

bool Encode(const ParticleAction& base, GpuPapiAction& result)
{
    result = {};
    result.type = base.type;
    result.flags = base.m_Flags.get();
    switch (base.type)
    {
    case PAAvoidID:
    {
        const auto& a = static_cast<const PAAvoid&>(base);
        result.domains[0] = Domain(a.positionL);
        result.p0.set(a.look_ahead, a.magnitude, a.epsilon, 0.f);
        break;
    }
    case PABounceID:
    {
        const auto& a = static_cast<const PABounce&>(base);
        result.domains[0] = Domain(a.positionL);
        result.p0.set(a.oneMinusFriction, a.resilience, a.cutoffSqr, 0.f);
        break;
    }
    case PACopyVertexBID: result.p0.x = static_cast<const PACopyVertexB&>(base).copy_pos; break;
    case PADampingID:
    {
        const auto& a = static_cast<const PADamping&>(base);
        result.p0 = Vector(a.damping, a.vlowSqr);
        result.p1.x = a.vhighSqr;
        break;
    }
    case PAExplosionID:
    {
        const auto& a = static_cast<const PAExplosion&>(base);
        result.p0 = Vector(a.centerL, a.velocity);
        result.p1.set(a.magnitude, a.stdev, a.epsilon, 0.f);
        result.p3.x = a.age;
        break;
    }
    case PAFollowID:
    {
        const auto& a = static_cast<const PAFollow&>(base);
        result.p0.set(a.magnitude, a.epsilon, a.max_radius, 0.f);
        break;
    }
    case PAGravitateID:
    {
        const auto& a = static_cast<const PAGravitate&>(base);
        result.p0.set(a.magnitude, a.epsilon, a.max_radius, 0.f);
        break;
    }
    case PAGravityID: result.p0 = Vector(static_cast<const PAGravity&>(base).direction); break;
    case PAJetID:
    {
        const auto& a = static_cast<const PAJet&>(base);
        result.p0 = Vector(a.centerL, a.magnitude);
        result.p1.set(a.epsilon, a.max_radius, 0.f, 0.f);
        result.domains[0] = Domain(a.accL);
        break;
    }
    case PAKillOldID:
    {
        const auto& a = static_cast<const PAKillOld&>(base);
        result.p0.set(a.age_limit, a.kill_less_than ? 1.f : 0.f, 0.f, 0.f);
        break;
    }
    case PAMatchVelocityID:
    {
        const auto& a = static_cast<const PAMatchVelocity&>(base);
        result.p0.set(a.magnitude, a.epsilon, a.max_radius, 0.f);
        break;
    }
    case PAMoveID: break;
    case PAOrbitLineID:
    {
        const auto& a = static_cast<const PAOrbitLine&>(base);
        result.p0 = Vector(a.pL, a.magnitude);
        result.p1 = Vector(a.axisL, a.epsilon);
        result.p2.x = a.max_radius;
        break;
    }
    case PAOrbitPointID:
    {
        const auto& a = static_cast<const PAOrbitPoint&>(base);
        result.p0 = Vector(a.centerL, a.magnitude);
        result.p1.set(a.epsilon, a.max_radius, 0.f, 0.f);
        break;
    }
    case PARandomAccelID: result.domains[0] = Domain(static_cast<const PARandomAccel&>(base).gen_accL); break;
    case PARandomDisplaceID: result.domains[0] = Domain(static_cast<const PARandomDisplace&>(base).gen_dispL); break;
    case PARandomVelocityID: result.domains[0] = Domain(static_cast<const PARandomVelocity&>(base).gen_velL); break;
    case PARestoreID: result.p3.x = static_cast<const PARestore&>(base).time_left; break;
    case PASinkID:
    {
        const auto& a = static_cast<const PASink&>(base);
        result.domains[0] = Domain(a.positionL);
        result.p0.x = a.kill_inside;
        break;
    }
    case PASinkVelocityID:
    {
        const auto& a = static_cast<const PASinkVelocity&>(base);
        result.domains[0] = Domain(a.velocityL);
        result.p0.x = a.kill_inside;
        break;
    }
    case PASourceID:
    {
        const auto& a = static_cast<const PASource&>(base);
        result.domains[0] = Domain(a.positionL);
        result.domains[1] = Domain(a.velocityL);
        result.domains[2] = Domain(a.rot);
        result.domains[3] = Domain(a.size);
        result.domains[4] = Domain(a.color);
        result.p0.set(a.alpha, a.particle_rate, a.age, a.age_sigma);
        result.p1 = Vector(a.parent_vel, a.parent_motion);
        result.flags &= ~PASource::flSilent;
        break;
    }
    case PASpeedLimitID:
    {
        const auto& a = static_cast<const PASpeedLimit&>(base);
        result.p0.set(a.min_speed, a.max_speed, 0.f, 0.f);
        break;
    }
    case PATargetColorID:
    {
        const auto& a = static_cast<const PATargetColor&>(base);
        result.p0 = Vector(a.color, a.alpha);
        result.p1.set(a.scale, a.timeFrom, a.timeTo, 0.f);
        break;
    }
    case PATargetSizeID:
    {
        const auto& a = static_cast<const PATargetSize&>(base);
        result.p0 = Vector(a.size);
        result.p1 = Vector(a.scale);
        break;
    }
    case PATargetRotateID:
    case PATargetRotateDID:
    {
        const auto& a = static_cast<const PATargetRotate&>(base);
        result.p0 = Vector(a.rot, a.scale);
        break;
    }
    case PATargetVelocityID:
    case PATargetVelocityDID:
    {
        const auto& a = static_cast<const PATargetVelocity&>(base);
        result.p0 = Vector(a.velocityL, a.scale);
        break;
    }
    case PAVortexID:
    {
        const auto& a = static_cast<const PAVortex&>(base);
        result.p0 = Vector(a.centerL, a.magnitude);
        result.p1 = Vector(a.axisL, a.epsilon);
        result.p2.x = a.max_radius;
        break;
    }
    case PATurbulenceID:
    {
        const auto& a = static_cast<const PATurbulence&>(base);
        result.p0.set(a.frequency, a.magnitude, a.epsilon, 0.f);
        result.p1 = Vector(a.offset);
        result.pad0 = static_cast<u32>(a.octaves);
#if defined(XR_ARCHITECTURE_ARM) || defined(XR_ARCHITECTURE_ARM64)
        result.p2.y = 1.f;
#endif
        if (a.octaves < 0 || a.frequency == 0.f)
            return false;
        break;
    }
    case PAScatterID:
    {
        const auto& a = static_cast<const PAScatter&>(base);
        result.p0 = Vector(a.centerL, a.magnitude);
        result.p1.set(a.epsilon, a.max_radius, 0.f, 0.f);
        break;
    }
    default: return false;
    }
    if (!Finite(result.p0) || !Finite(result.p1) || !Finite(result.p2) || !Finite(result.p3))
        return false;
    const bool generate = base.type == PASourceID || base.type == PAJetID || base.type == PARandomAccelID ||
        base.type == PARandomDisplaceID || base.type == PARandomVelocityID;
    for (const auto& domain : result.domains)
        if (!ValidDomain(domain, generate))
            return false;
    return true;
}
}

bool TranslateGpuParticleDefinition(const PS::CPEDef& definition, GpuPapiProgram& program,
    xr_vector<GpuPapiAction>& actions, xr_string& error)
{
    error.clear();
    actions.clear();
    if (definition.m_MaxParticles < 0)
        return Fail(error, definition, 0, "negative particle capacity");
    if (!CheckStream(definition, error))
        return false;
    auto* manager = static_cast<PAPI::CParticleManager*>(PAPI::ParticleManager());
    const int handle = manager->CreateActionList();
    struct ListGuard
    {
        PAPI::CParticleManager* manager;
        int handle;
        ~ListGuard() { manager->DestroyActionList(handle); }
    } guard{manager, handle};
    IReader reader(definition.m_Actions.pointer(), definition.m_Actions.size());
    manager->LoadActions(handle, reader);
    auto* list = manager->GetActionListPtr(handle);
    actions.reserve(list->size());
    u32 stateCount = 0;
    for (const PAPI::ParticleAction* action : *list)
    {
        GpuPapiAction encoded{};
        if (!Encode(*action, encoded))
        {
            const size_t index = actions.size();
            actions.clear();
            return Fail(error, definition, index, "invalid action parameters or domain data");
        }
        encoded.stateIndex = action->type == PAExplosionID || action->type == PARestoreID ||
            action->type == PATurbulenceID ? stateCount++ : GPU_PAPI_INVALID;
        actions.push_back(encoded);
    }
    program.flags = definition.m_Flags.get();
    program.frameSize = definition.m_Frame.m_fTexSize;
    program.frameSpeed = definition.m_Frame.m_fSpeed;
    program.frameCount = definition.m_Frame.m_iFrameCount;
    program.frameDimX = definition.m_Frame.m_iFrameDimX;
    program.alignRotation = definition.m_APDefaultRotation;
    program.velocityScale = definition.m_VelocityScale;
    program.maxParticles = static_cast<u32>(definition.m_MaxParticles);
    program.actionFirst = 0;
    program.actionCount = static_cast<u32>(actions.size());
    program.stateCount = stateCount;
    program.pad1 = 0;
    program.collisionFriction = definition.m_fCollideOneMinusFriction;
    program.collisionResilience = definition.m_fCollideResilience;
    program.collisionCutoff = definition.m_fCollideSqrCutoff;
    program.timeLimit = definition.m_fTimeLimit;
    if ((program.flags & PS::CPEDef::dfFramed) &&
        (definition.m_Frame.m_iFrameCount <= 0 || definition.m_Frame.m_iFrameDimX <= 0))
    {
        actions.clear();
        return Fail(error, definition, 0, "framed effect has an invalid atlas extent");
    }
    return true;
}
}
