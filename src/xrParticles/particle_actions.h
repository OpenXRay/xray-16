#pragma once

#include <atomic>

namespace PAPI
{
// refs
struct ParticleEffect;

struct PARTICLES_API ParticleAction
{
    enum
    {
        ALLOW_ROTATE = 1 << 1
    };

    Flags32 m_Flags;
    PActionEnum type; // Type field
    ParticleAction() { m_Flags.zero(); }
    virtual void Execute(ParticleEffect* pe, const float dt, float& m_max) = 0;
    virtual void Transform(const Fmatrix& m) = 0;

    virtual void Load(IReader& F) = 0;
    virtual void Save(IWriter& F) = 0;
};

using PAVec = xr_vector<ParticleAction*>;
using PAVecIt = PAVec::iterator;

class ParticleActions
{
    PAVec actions;

public:
    ParticleActions() { actions.reserve(4); }

    ~ParticleActions() { clear(); }

    void clear()
    {
        for (auto& it : actions)
            xr_delete(it);
        actions.clear();
    }

    void append(ParticleAction* pa) { actions.push_back(pa); }

    bool empty() const { return actions.empty(); }
    PAVecIt begin() { return actions.begin(); }
    PAVecIt end() { return actions.end(); }
    int size() const { return actions.size(); }

    void resize(int cnt) { actions.resize(cnt); }

    void copy(ParticleActions* src);

    void lock() { }

    void unlock() { }
};
} // namespace PAPI
