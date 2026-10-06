#include "StdAfx.h"
#include "HudWeaponCollision.h"

#include "xrCDB/Intersect.hpp"
#include "xrMaterialSystem/GameMtlLib.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float MinRadius = .005f;
constexpr float MaxRadius = .15f;
constexpr float MaxForwardOffset = .3f;
constexpr float MaxPushLimit = 1.f;
constexpr float MaxQueryDistance = 3.f;
constexpr float Skin = .003f;
constexpr float BroadphaseMargin = .01f;
constexpr float HitTolerance = 1e-5f;
constexpr float PushEpsilon = 1e-5f;
constexpr float ContactEpsilon = 1e-5f;
constexpr float MotionEpsilon = 1e-6f;
constexpr float ApproachEpsilon = 1e-6f;
constexpr float TimeTieEpsilon = 1e-6f;
constexpr float DirectionEpsilon = 1e-8f;
constexpr float AbsoluteAreaEpsilon = 1e-12f;
constexpr float MinConditioning = .01f;
constexpr u32 MaxContacts = 4;
constexpr u32 MaxAdvance = 24;

bool IsPassable(u32 index)
{
    if (index >= GMLib.CountMaterial())
    {
        return false;
    }
    SGameMtl* material = GMLib.GetMaterialByIdx(u16(index));
    return material && material->Flags.test(SGameMtl::flPassable);
}

void ClosestOnSegment(const Fvector& point, const Fvector& a, const Fvector& b, Fvector& closest)
{
    Fvector edge;
    edge.sub(b, a);
    Fvector offset;
    offset.sub(point, a);
    const float lengthSq = edge.square_magnitude();
    const float t = lengthSq > 0.f ? std::clamp(offset.dotproduct(edge) / lengthSq, 0.f, 1.f) : 0.f;
    closest.mad(a, edge, t);
}

_vector3<double> Cross(const _vector3<double>& a, const _vector3<double>& b)
{
    _vector3<double> result;
    result.set(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    return result;
}
}

bool CHudWeaponCollision::Triangle::Build(const CDB::RESULT& result)
{
    for (u32 i = 0; i < 3; ++i)
    {
        if (!_valid(result.verts[i]))
        {
            return false;
        }
        m_vertices[i] = result.verts[i];
    }

    m_baseD.set(double(m_vertices[0].x), double(m_vertices[0].y), double(m_vertices[0].z));
    m_edge0D.set(double(m_vertices[1].x) - m_baseD.x, double(m_vertices[1].y) - m_baseD.y,
        double(m_vertices[1].z) - m_baseD.z);
    m_edge1D.set(double(m_vertices[2].x) - m_baseD.x, double(m_vertices[2].y) - m_baseD.y,
        double(m_vertices[2].z) - m_baseD.z);
    m_normalD = Cross(m_edge0D, m_edge1D);
    m_normalSqD = m_normalD.dotproduct(m_normalD);
    if (!(m_normalSqD > AbsoluteAreaEpsilon) || !std::isfinite(m_normalSqD))
    {
        return false;
    }
    const double inverse = 1.0 / std::sqrt(m_normalSqD);
    _vector3<double> unit;
    unit.set(m_normalD.x * inverse, m_normalD.y * inverse, m_normalD.z * inverse);
    m_normal.set(unit);

    bool found = false;
    float bestRatio = 0.f;
    for (u32 i = 0; i < 3; ++i)
    {
        Fvector e0;
        e0.sub(m_vertices[(i + 1) % 3], m_vertices[i]);
        Fvector e1;
        e1.sub(m_vertices[(i + 2) % 3], m_vertices[i]);
        const float a00 = e0.square_magnitude();
        const float a11 = e1.square_magnitude();
        const float a01 = e0.dotproduct(e1);
        const float product = a00 * a11;
        if (!_valid(product) || !(product > 0.f))
        {
            continue;
        }
        const float ratio = (product - a01 * a01) / product;
        if (_valid(ratio) && (!found || ratio > bestRatio))
        {
            found = true;
            bestRatio = ratio;
            m_origin = m_vertices[i];
            m_e0 = e0;
            m_e1 = e1;
        }
    }
    m_wide = found && bestRatio >= MinConditioning;
    return true;
}

float CHudWeaponCollision::Triangle::SqrDistance(const Fvector& point, Fvector& closest) const
{
    if (m_wide)
    {
        CDB::MgcSqrDistance(point, m_origin, m_e0, m_e1, &closest);
    }
    else
    {
        _vector3<double> offset;
        offset.set(double(point.x) - m_baseD.x, double(point.y) - m_baseD.y, double(point.z) - m_baseD.z);
        const _vector3<double> crossE1 = Cross(offset, m_edge1D);
        const _vector3<double> crossE0 = Cross(m_edge0D, offset);
        const double s = crossE1.dotproduct(m_normalD) / m_normalSqD;
        const double t = crossE0.dotproduct(m_normalD) / m_normalSqD;
        if (s >= 0.0 && t >= 0.0 && s + t <= 1.0)
        {
            _vector3<double> inside;
            inside.set(m_baseD.x + s * m_edge0D.x + t * m_edge1D.x, m_baseD.y + s * m_edge0D.y + t * m_edge1D.y,
                m_baseD.z + s * m_edge0D.z + t * m_edge1D.z);
            closest.set(inside);
        }
        else
        {
            Fvector candidate;
            float best = -1.f;
            for (u32 i = 0; i < 3; ++i)
            {
                ClosestOnSegment(point, m_vertices[i], m_vertices[(i + 1) % 3], candidate);
                Fvector diff;
                diff.sub(point, candidate);
                const float distanceSq = diff.square_magnitude();
                if (best < 0.f || distanceSq < best)
                {
                    best = distanceSq;
                    closest = candidate;
                }
            }
        }
    }
    if (!_valid(closest))
    {
        return -1.f;
    }
    Fvector diff;
    diff.sub(point, closest);
    return diff.square_magnitude();
}

const Fvector& CHudWeaponCollision::Triangle::Normal() const
{
    return m_normal;
}

CHudWeaponCollision::CHudWeaponCollision() = default;
CHudWeaponCollision::~CHudWeaponCollision() = default;

const CHudWeaponCollision::Settings& CHudWeaponCollision::GetSettings() const
{
    return m_settings;
}

CHudWeaponCollision::Settings CHudWeaponCollision::Sanitize(const Settings& input)
{
    const Settings defaults;
    Settings output;
    output.enabled = input.enabled;
    output.radius = _valid(input.radius) ? std::clamp(input.radius, MinRadius, MaxRadius) : defaults.radius;
    output.forwardOffset =
        _valid(input.forwardOffset) ? std::clamp(input.forwardOffset, 0.f, MaxForwardOffset) : defaults.forwardOffset;
    output.maxPush = _valid(input.maxPush) ? std::clamp(input.maxPush, 0.f, MaxPushLimit) : defaults.maxPush;
    return output;
}

void CHudWeaponCollision::SetSettings(const Settings& settings)
{
    const Settings sanitized = Sanitize(settings);
    if (sanitized.enabled == m_settings.enabled && sanitized.radius == m_settings.radius &&
        sanitized.forwardOffset == m_settings.forwardOffset && sanitized.maxPush == m_settings.maxPush)
    {
        return;
    }
    m_settings = sanitized;
    m_solved = false;
}

const CHudWeaponCollision::State& CHudWeaponCollision::GetState() const
{
    return m_state;
}

void CHudWeaponCollision::Reset()
{
    m_state = State{};
    m_solved = false;
    m_solvedFrame = 0;
    m_xrc.r_clear();
}

bool CHudWeaponCollision::BuildMuzzle(const Muzzle& input, Muzzle& output) const
{
    output = Muzzle{};
    if (!input.valid || !_valid(input.firePoint) || !_valid(input.direction))
    {
        return false;
    }
    const float lengthSq = input.direction.square_magnitude();
    if (!_valid(lengthSq) || lengthSq <= DirectionEpsilon)
    {
        return false;
    }
    output.firePoint = input.firePoint;
    output.direction.mul(input.direction, 1.f / std::sqrt(lengthSq));
    output.sphereCenter.mad(output.firePoint, output.direction, m_settings.forwardOffset);
    output.valid = _valid(output.sphereCenter);
    return output.valid;
}

void CHudWeaponCollision::SetResolved(const Muzzle& resolved)
{
    BuildMuzzle(resolved, m_state.resolved);
}

bool CHudWeaponCollision::FindContact(const CDB::MODEL& model, const Fvector& camera, const Fvector& target,
    Contact& contact, u32& candidates)
{
    contact = Contact{};

    const float radius = m_settings.radius;
    Fvector motion;
    motion.sub(target, camera);
    const float motionLength = motion.magnitude();
    const float margin = radius + Skin + BroadphaseMargin;
    Fvector center;
    center.add(camera, target);
    center.mul(.5f);
    Fvector extents;
    extents.set(_abs(motion.x) * .5f + margin, _abs(motion.y) * .5f + margin, _abs(motion.z) * .5f + margin);

    m_xrc.box_query(CDB::OPT_FULL_TEST, &model, center, extents);
    xr_vector<CDB::RESULT>* results = m_xrc.r_get();
    const size_t count = results->size();
    candidates += u32(count);

    for (size_t index = 0; index < count; ++index)
    {
        const CDB::RESULT& result = (*results)[index];
        if (IsPassable(result.material))
        {
            continue;
        }
        Triangle triangle;
        if (!triangle.Build(result))
        {
            continue;
        }

        float time = 0.f;
        float distance = 0.f;
        bool hit = false;
        Fvector probe{};
        Fvector closest{};
        Fvector diff{};
        for (u32 iteration = 0; iteration < MaxAdvance; ++iteration)
        {
            probe.mad(camera, motion, time);
            const float distanceSq = triangle.SqrDistance(probe, closest);
            if (!_valid(distanceSq) || distanceSq < 0.f)
            {
                break;
            }
            diff.sub(probe, closest);
            distance = std::sqrt(distanceSq);
            if (distance <= radius + HitTolerance)
            {
                hit = true;
                break;
            }
            if (motionLength <= MotionEpsilon)
            {
                break;
            }
            const float approach = motion.dotproduct(diff) / distance;
            if (!(approach < -ApproachEpsilon))
            {
                break;
            }
            time += (distance - radius) / -approach;
            if (!(time <= 1.f))
            {
                break;
            }
            if (iteration + 1 == MaxAdvance)
            {
                probe.mad(camera, motion, time);
                const float finalSq = triangle.SqrDistance(probe, closest);
                if (_valid(finalSq) && finalSq >= 0.f)
                {
                    diff.sub(probe, closest);
                    distance = std::sqrt(finalSq);
                    hit = true;
                }
            }
        }
        if (!hit)
        {
            continue;
        }

        Fvector normal;
        if (distance > ContactEpsilon)
        {
            normal.mul(diff, 1.f / distance);
        }
        else
        {
            normal = triangle.Normal();
            Fvector safeSide;
            safeSide.sub(camera, target);
            if (normal.dotproduct(safeSide) < 0.f)
            {
                normal.invert();
            }
        }

        Fvector offset;
        offset.sub(target, closest);
        const float height = offset.dotproduct(normal);
        const float push = radius + Skin - height;
        if (!_valid(push) || push <= PushEpsilon)
        {
            continue;
        }

        const bool earlier = time < contact.time - TimeTieEpsilon;
        const bool tied = _abs(time - contact.time) <= TimeTieEpsilon && push > contact.push;
        if (contact.found && !earlier && !tied)
        {
            continue;
        }
        contact.found = true;
        contact.time = time;
        contact.push = push;
        contact.penetration = std::max(0.f, radius - height);
        contact.triangle = result.id;
        contact.point = closest;
        contact.normal = normal;
    }
    return contact.found;
}

void CHudWeaponCollision::Solve(const Fvector& camera, const Muzzle& original, const Muzzle& requested, bool canApply)
{
    if (m_solved && m_solvedFrame == Device.dwFrame)
    {
        return;
    }
    m_solved = true;
    m_solvedFrame = Device.dwFrame;

    m_state = State{};
    m_state.frame = Device.dwFrame;

    const bool originalValid = BuildMuzzle(original, m_state.original);
    const bool requestedValid = BuildMuzzle(requested, m_state.requested);

    if (!m_settings.enabled)
    {
        m_state.status = "disabled";
        return;
    }

    const CDB::MODEL* model = g_pGameLevel ? g_pGameLevel->ObjectSpace.GetStaticModel() : nullptr;
    if (!originalValid || !requestedValid || !_valid(camera) || !model || model->get_tris_count() == 0 ||
        model->get_verts_count() == 0)
    {
        m_state.status = "invalid";
        return;
    }

    if (!canApply)
    {
        m_state.status = "no_ik";
        return;
    }

    const Fvector desired = m_state.requested.sphereCenter;
    Fvector toDesired;
    toDesired.sub(desired, camera);
    const float range = toDesired.magnitude();
    if (!_valid(range) || range > MaxQueryDistance)
    {
        m_state.status = "out_of_range";
        return;
    }

    Fvector target = desired;
    bool clamped = false;
    bool unresolved = false;
    for (u32 pass = 0; pass <= MaxContacts; ++pass)
    {
        Contact contact;
        if (!FindContact(*model, camera, target, contact, m_state.candidates))
        {
            break;
        }
        if (pass == MaxContacts)
        {
            unresolved = true;
            break;
        }
        if (!m_state.hit)
        {
            m_state.hit = true;
            m_state.triangle = contact.triangle;
            m_state.contact = contact.point;
            m_state.normal = contact.normal;
        }
        m_state.penetration = std::max(m_state.penetration, contact.penetration);
        target.mad(target, contact.normal, contact.push);

        Fvector correction;
        correction.sub(target, desired);
        const float length = correction.magnitude();
        if (!_valid(length))
        {
            m_state = State{};
            m_state.frame = Device.dwFrame;
            m_state.status = "invalid";
            return;
        }
        if (length > m_settings.maxPush)
        {
            correction.mul(m_settings.maxPush / length);
            target.add(desired, correction);
            clamped = true;
            break;
        }
    }

    m_state.correction.sub(target, desired);
    m_state.clamped = clamped || unresolved;
    if (!m_state.hit)
    {
        m_state.status = "clear";
    }
    else if (unresolved)
    {
        m_state.status = "unresolved";
    }
    else if (clamped)
    {
        m_state.status = "clamped";
    }
    else
    {
        m_state.status = "contact";
    }
}
