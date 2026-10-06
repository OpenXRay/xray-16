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
constexpr u32 MaxFrameGap = 4;
constexpr float MaxFrameDelta = .25f;
constexpr float MaxCameraJump = 1.5f;
constexpr float SettleTargetSq = 1e-10f;
constexpr float SettleDistanceSq = 1e-8f;
constexpr float SettleVelocitySq = 1e-6f;
constexpr float SafetyEpsilon = 1e-5f;
constexpr float MaxAnticipation = .1f;
constexpr float MinResponseSeconds = .01f;
constexpr float MaxContactSeconds = 1.f;
constexpr float MaxReleaseSeconds = 2.f;
constexpr u32 MaxAdvance = 24;
constexpr float VelocityEpsilon = 1e-6f;

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

void RemoveInward(Fvector& velocity, const Fvector& normal)
{
    const float along = velocity.dotproduct(normal);
    if (along < 0.f)
    {
        velocity.mad(velocity, normal, -along);
    }
}

void RemoveOutward(Fvector& velocity, const Fvector& direction)
{
    const float along = velocity.dotproduct(direction);
    if (along > 0.f)
    {
        velocity.mad(velocity, direction, -along);
    }
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
    output.easing = input.easing;
    output.radius = _valid(input.radius) ? std::clamp(input.radius, MinRadius, MaxRadius) : defaults.radius;
    output.forwardOffset =
        _valid(input.forwardOffset) ? std::clamp(input.forwardOffset, 0.f, MaxForwardOffset) : defaults.forwardOffset;
    output.maxPush = _valid(input.maxPush) ? std::clamp(input.maxPush, 0.f, MaxPushLimit) : defaults.maxPush;
    output.anticipation =
        _valid(input.anticipation) ? std::clamp(input.anticipation, 0.f, MaxAnticipation) : defaults.anticipation;
    output.contactSeconds = _valid(input.contactSeconds) ?
        std::clamp(input.contactSeconds, MinResponseSeconds, MaxContactSeconds) :
        defaults.contactSeconds;
    output.releaseSeconds = _valid(input.releaseSeconds) ?
        std::clamp(input.releaseSeconds, MinResponseSeconds, MaxReleaseSeconds) :
        defaults.releaseSeconds;
    return output;
}

void CHudWeaponCollision::SetSettings(const Settings& settings)
{
    const Settings sanitized = Sanitize(settings);
    if (sanitized.enabled == m_settings.enabled && sanitized.easing == m_settings.easing &&
        sanitized.radius == m_settings.radius && sanitized.forwardOffset == m_settings.forwardOffset &&
        sanitized.maxPush == m_settings.maxPush && sanitized.anticipation == m_settings.anticipation &&
        sanitized.contactSeconds == m_settings.contactSeconds && sanitized.releaseSeconds == m_settings.releaseSeconds)
    {
        return;
    }
    m_settings = sanitized;
}

const CHudWeaponCollision::State& CHudWeaponCollision::GetState() const
{
    return m_state;
}

void CHudWeaponCollision::BeginUpdate()
{
    m_state = State{};
    m_xrc.r_clear();
}

void CHudWeaponCollision::ResetSmoothing()
{
    m_smoothValid = false;
    m_restarted = false;
    m_smoothFrame = 0;
    m_frameDelta = 0.f;
    m_startPosition = Fvector();
    m_startVelocity = Fvector();
    m_position = Fvector();
    m_velocity = Fvector();
    m_lastCamera = Fvector();
}

void CHudWeaponCollision::Reset()
{
    m_state = State{};
    ResetSmoothing();
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
    float radius, Contact& contact, u32& candidates)
{
    contact = Contact{};

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

bool CHudWeaponCollision::Resolve(const CDB::MODEL& model, const Fvector& camera, const Fvector& desired,
    const Fvector& start, float radius, Resolution& resolution)
{
    resolution = Resolution{};
    resolution.target = start;
    for (u32 pass = 0; pass <= MaxContacts; ++pass)
    {
        Contact contact;
        if (!FindContact(model, camera, resolution.target, radius, contact, m_state.candidates))
        {
            break;
        }
        if (pass == MaxContacts)
        {
            resolution.unresolved = true;
            break;
        }
        if (!resolution.hit)
        {
            resolution.hit = true;
            resolution.triangle = contact.triangle;
            resolution.contact = contact.point;
            resolution.normal = contact.normal;
        }
        resolution.penetration = std::max(resolution.penetration, contact.penetration);
        resolution.normals[resolution.normalCount++] = contact.normal;
        resolution.target.mad(resolution.target, contact.normal, contact.push);

        Fvector correction;
        correction.sub(resolution.target, desired);
        const float length = correction.magnitude();
        if (!_valid(length))
        {
            return false;
        }
        if (length > m_settings.maxPush)
        {
            correction.mul(m_settings.maxPush / length);
            resolution.target.add(desired, correction);
            resolution.clamped = true;
            break;
        }
    }
    return _valid(resolution.target);
}

void CHudWeaponCollision::PrepareSmoothing(const Fvector& camera, const Fvector& target)
{
    const u32 frame = Device.dwFrame;
    Fvector jump;
    jump.sub(camera, m_lastCamera);
    const bool jumped = m_smoothValid && (!_valid(jump) || jump.square_magnitude() > MaxCameraJump * MaxCameraJump);
    if (m_smoothValid && frame == m_smoothFrame && !jumped)
    {
        m_lastCamera = camera;
        return;
    }

    bool restart = !m_smoothValid || jumped || frame < m_smoothFrame || frame - m_smoothFrame > MaxFrameGap;
    float delta = 0.f;
    if (!restart)
    {
        delta = Device.fTimeDelta;
        if (!_valid(delta) || delta < 0.f)
        {
            delta = 0.f;
        }
        if (delta > MaxFrameDelta || !_valid(m_position) || !_valid(m_velocity))
        {
            restart = true;
        }
    }

    if (restart)
    {
        m_startPosition = target;
        m_startVelocity = Fvector();
        m_frameDelta = 0.f;
    }
    else
    {
        m_startPosition = m_position;
        m_startVelocity = m_velocity;
        m_frameDelta = delta;
    }
    m_restarted = restart;
    m_smoothValid = true;
    m_smoothFrame = frame;
    m_lastCamera = camera;
}

void CHudWeaponCollision::Integrate(const Fvector& target, Fvector& position, Fvector& velocity) const
{
    position = m_startPosition;
    velocity = m_startVelocity;
    const float delta = m_frameDelta;
    if (!(delta > 0.f))
    {
        return;
    }

    const bool targetZero = target.square_magnitude() <= SettleTargetSq;
    Fvector toTarget;
    toTarget.sub(target, m_startPosition);
    const bool engaging = !targetZero && target.dotproduct(toTarget) >= 0.f;
    const float seconds = engaging ? m_settings.contactSeconds : m_settings.releaseSeconds;
    const float omega = 2.f / seconds;

    Fvector offset;
    offset.sub(m_startPosition, target);
    Fvector carry;
    carry.mad(m_startVelocity, offset, omega);
    const float decay = std::exp(-omega * delta);

    Fvector next;
    next.mad(offset, carry, delta);
    next.mul(decay);
    Fvector rate;
    rate.mad(m_startVelocity, carry, -omega * delta);
    rate.mul(decay);

    const float startSq = offset.square_magnitude();
    const float nextSq = next.square_magnitude();
    if (next.dotproduct(offset) < 0.f)
    {
        next = Fvector();
        rate = Fvector();
    }
    else if (!engaging && nextSq > startSq)
    {
        Fvector direction;
        direction.mul(next, 1.f / std::sqrt(nextSq));
        next.mul(std::sqrt(startSq / nextSq));
        RemoveOutward(rate, direction);
    }

    position.add(target, next);
    velocity = rate;
    if (!engaging && targetZero && position.square_magnitude() <= SettleDistanceSq &&
        velocity.square_magnitude() <= SettleVelocitySq)
    {
        position = Fvector();
        velocity = Fvector();
    }
}

void CHudWeaponCollision::Solve(const Fvector& camera, const Muzzle& original, const Muzzle& requested, bool canApply)
{
    m_state = State{};
    m_state.frame = Device.dwFrame;

    const bool originalValid = BuildMuzzle(original, m_state.original);
    const bool requestedValid = BuildMuzzle(requested, m_state.requested);

    auto fail = [this]()
    {
        m_state = State{};
        m_state.frame = Device.dwFrame;
        m_state.status = "invalid";
        ResetSmoothing();
    };

    if (!m_settings.enabled)
    {
        m_state.status = "disabled";
        ResetSmoothing();
        return;
    }

    const CDB::MODEL* model = g_pGameLevel ? g_pGameLevel->ObjectSpace.GetStaticModel() : nullptr;
    if (!originalValid || !requestedValid || !_valid(camera) || !model || model->get_tris_count() == 0 ||
        model->get_verts_count() == 0)
    {
        m_state.status = "invalid";
        ResetSmoothing();
        return;
    }

    if (!canApply)
    {
        m_state.status = "no_ik";
        ResetSmoothing();
        return;
    }

    const Fvector desired = m_state.requested.sphereCenter;
    Fvector toDesired;
    toDesired.sub(desired, camera);
    const float range = toDesired.magnitude();
    if (!_valid(range) || range > MaxQueryDistance)
    {
        m_state.status = "out_of_range";
        ResetSmoothing();
        return;
    }

    const float radius = m_settings.radius;
    const bool easing = m_settings.easing;
    const float anticipation = easing ? m_settings.anticipation : 0.f;

    Resolution hard;
    Resolution ahead;
    bool resolved = Resolve(*model, camera, desired, desired, radius, hard);
    if (resolved)
    {
        if (anticipation > 0.f)
        {
            resolved = Resolve(*model, camera, desired, desired, radius + anticipation, ahead);
        }
        else
        {
            ahead = hard;
        }
    }
    if (!resolved)
    {
        fail();
        return;
    }

    Fvector hardCorrection;
    hardCorrection.sub(hard.target, desired);
    Fvector target;
    target.sub(ahead.target, desired);

    Fvector position = target;
    Fvector velocity{};
    bool springClamped = false;
    if (easing)
    {
        PrepareSmoothing(camera, target);
        Integrate(target, position, velocity);
        if (!_valid(position) || !_valid(velocity))
        {
            m_smoothValid = false;
            PrepareSmoothing(camera, target);
            position = m_startPosition;
            velocity = m_startVelocity;
        }
        const float length = position.magnitude();
        if (length > m_settings.maxPush)
        {
            Fvector direction;
            direction.mul(position, 1.f / length);
            position.mul(m_settings.maxPush / length);
            RemoveOutward(velocity, direction);
            springClamped = true;
        }
    }
    else
    {
        ResetSmoothing();
    }

    Fvector candidate;
    candidate.add(desired, position);
    Resolution safe;
    if (!Resolve(*model, camera, desired, candidate, radius, safe))
    {
        fail();
        return;
    }

    Fvector applied;
    applied.sub(safe.target, desired);
    Fvector push;
    push.sub(applied, position);
    const float pushLength = push.magnitude();
    if (!_valid(applied) || !_valid(pushLength))
    {
        fail();
        return;
    }
    const bool safetyApplied = pushLength > SafetyEpsilon;
    if (safetyApplied)
    {
        Fvector limit{};
        const float length = applied.magnitude();
        const bool limited = safe.clamped && length > 0.f;
        if (limited)
        {
            limit.mul(applied, 1.f / length);
        }
        bool violated = true;
        for (u32 pass = 0; pass < MaxContacts && violated; ++pass)
        {
            for (u32 index = 0; index < safe.normalCount; ++index)
            {
                RemoveInward(velocity, safe.normals[index]);
            }
            if (limited)
            {
                RemoveOutward(velocity, limit);
            }
            violated = limited && velocity.dotproduct(limit) > VelocityEpsilon;
            for (u32 index = 0; index < safe.normalCount && !violated; ++index)
            {
                violated = velocity.dotproduct(safe.normals[index]) < -VelocityEpsilon;
            }
        }
        if (violated)
        {
            velocity = Fvector();
        }
    }

    if (easing)
    {
        m_position = applied;
        m_velocity = velocity;
    }

    const bool unresolved = hard.unresolved || safe.unresolved;
    const bool clamped = hard.clamped || safe.clamped || springClamped || unresolved || ahead.clamped || ahead.unresolved;
    const Resolution& source = hard.hit ? hard : (safe.hit ? safe : ahead);

    m_state.hit = hard.hit || safe.hit;
    if (source.hit)
    {
        m_state.triangle = source.triangle;
        m_state.contact = source.contact;
        m_state.normal = source.normal;
    }
    m_state.penetration = hard.penetration;
    m_state.anticipated = anticipation > 0.f && ahead.hit;
    m_state.hardCorrection = hardCorrection;
    m_state.targetCorrection = easing ? target : hardCorrection;
    m_state.correction = applied;
    m_state.velocity = velocity;
    m_state.safety = safetyApplied;
    m_state.safetyPush = safetyApplied ? pushLength : 0.f;
    m_state.deltaTime = easing ? m_frameDelta : 0.f;
    m_state.restarted = easing && m_restarted;
    m_state.clamped = clamped;

    if (unresolved)
    {
        m_state.status = "unresolved";
    }
    else if (clamped)
    {
        m_state.status = "clamped";
    }
    else if (m_state.hit)
    {
        m_state.status = "contact";
    }
    else if (m_state.anticipated)
    {
        m_state.status = "anticipating";
    }
    else if (applied.square_magnitude() > SettleTargetSq)
    {
        m_state.status = "releasing";
    }
    else
    {
        m_state.status = "clear";
    }
}
