#pragma once

#include "xrCDB/xrXRC.h"

class CHudWeaponCollision
{
public:
    class Settings
    {
    public:
        bool enabled = true;
        float radius = .025f;
        float forwardOffset = .03f;
        float maxPush = .75f;
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
        s32 triangle = -1;
        Muzzle original{};
        Muzzle requested{};
        Muzzle resolved{};
        Fvector contact{};
        Fvector normal{};
        Fvector correction{};
        float penetration = 0.f;
        u32 candidates = 0;
    };

    CHudWeaponCollision();
    ~CHudWeaponCollision();
    CHudWeaponCollision(const CHudWeaponCollision&) = delete;
    CHudWeaponCollision& operator=(const CHudWeaponCollision&) = delete;

    const Settings& GetSettings() const;
    void SetSettings(const Settings& settings);
    const State& GetState() const;
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
    bool FindContact(const CDB::MODEL& model, const Fvector& camera, const Fvector& target, Contact& contact,
        u32& candidates);

    Settings m_settings;
    State m_state;
    xrXRC m_xrc{"hud weapon collision"};
    bool m_solved = false;
    u32 m_solvedFrame = 0;
};
