#pragma once

#include "xrCDB/xrXRC.h"

class CHudWeaponCollision
{
public:
    static constexpr u32 MaxContacts = 4;

    class Settings
    {
    public:
        bool enabled = true;
        bool easing = true;
        float radius = .025f;
        float forwardOffset = .03f;
        float maxPush = .75f;
        float anticipation = .06f;
        float contactSeconds = .05f;
        float releaseSeconds = .2f;
    };

    class Muzzle
    {
    public:
        bool valid = false;
        Fvector firePoint{};
        Fvector direction{};
        Fvector sphereCenter{};
    };

    class State
    {
    public:
        pcstr status = "inactive";
        u32 frame = 0;
        bool hit = false;
        bool clamped = false;
        bool anticipated = false;
        bool safety = false;
        bool restarted = false;
        s32 triangle = -1;
        Muzzle original{};
        Muzzle requested{};
        Muzzle resolved{};
        Fvector contact{};
        Fvector normal{};
        Fvector hardCorrection{};
        Fvector targetCorrection{};
        Fvector correction{};
        Fvector velocity{};
        float penetration = 0.f;
        float safetyPush = 0.f;
        float deltaTime = 0.f;
        u32 candidates = 0;
    };

    CHudWeaponCollision();
    ~CHudWeaponCollision();
    CHudWeaponCollision(const CHudWeaponCollision&) = delete;
    CHudWeaponCollision& operator=(const CHudWeaponCollision&) = delete;

    const Settings& GetSettings() const;
    void SetSettings(const Settings& settings);
    const State& GetState() const;
    void BeginUpdate();
    void ResetSmoothing();
    void Reset();
    void Solve(const Fvector& camera, const Muzzle& original, const Muzzle& requested, bool canApply);
    void SetResolved(const Muzzle& resolved);

private:
    class Contact
    {
    public:
        bool found = false;
        float time = 0.f;
        float push = 0.f;
        float penetration = 0.f;
        s32 triangle = -1;
        Fvector point{};
        Fvector normal{};
    };

    class Resolution
    {
    public:
        bool hit = false;
        bool clamped = false;
        bool unresolved = false;
        float penetration = 0.f;
        s32 triangle = -1;
        Fvector contact{};
        Fvector normal{};
        Fvector target{};
        Fvector normals[MaxContacts]{};
        u32 normalCount = 0;
    };

    class Triangle
    {
    public:
        bool Build(const CDB::RESULT& result);
        float SqrDistance(const Fvector& point, Fvector& closest) const;
        const Fvector& Normal() const;

    private:
        Fvector m_vertices[3]{};
        Fvector m_origin{};
        Fvector m_e0{};
        Fvector m_e1{};
        Fvector m_normal{};
        _vector3<double> m_baseD{};
        _vector3<double> m_edge0D{};
        _vector3<double> m_edge1D{};
        _vector3<double> m_normalD{};
        double m_normalSqD = 0.0;
        bool m_wide = false;
    };

    static Settings Sanitize(const Settings& input);
    bool BuildMuzzle(const Muzzle& input, Muzzle& output) const;
    bool FindContact(const CDB::MODEL& model, const Fvector& camera, const Fvector& target, float radius,
        Contact& contact, u32& candidates);
    bool Resolve(const CDB::MODEL& model, const Fvector& camera, const Fvector& desired, const Fvector& start,
        float radius, Resolution& resolution);
    void PrepareSmoothing(const Fvector& camera, const Fvector& target);
    void Integrate(const Fvector& target, Fvector& position, Fvector& velocity) const;

    Settings m_settings;
    State m_state;
    xrXRC m_xrc{"hud weapon collision"};
    bool m_smoothValid = false;
    bool m_restarted = false;
    u32 m_smoothFrame = 0;
    float m_frameDelta = 0.f;
    Fvector m_startPosition{};
    Fvector m_startVelocity{};
    Fvector m_position{};
    Fvector m_velocity{};
    Fvector m_lastCamera{};
};
