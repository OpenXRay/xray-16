#pragma once
#include "game_base_space.h"
#include "alife_space.h"
#include "gametype_chooser.h"
#include "xrCommon/xr_deque.h"
#include "xrCommon/xr_vector.h"
#include "xrEngine/EngineAPI.h"

class IGameState
{
public:
    virtual ~IGameState() = 0;
    virtual EGameIDs Type() const = 0;
    virtual u16 Phase() const = 0;
    virtual u32 StartTime() const = 0;
    virtual void Create(shared_str& options) = 0;
    virtual const char* type_name() const = 0;
    virtual ALife::_TIME_ID GetStartGameTime() = 0;
    virtual ALife::_TIME_ID GetGameTime() = 0;
    virtual float GetGameTimeFactor() = 0;
    virtual void SetGameTimeFactor(const float value) = 0;
    virtual ALife::_TIME_ID GetEnvironmentGameTime() = 0;
    virtual float GetEnvironmentGameTimeFactor() = 0;
    virtual void SetEnvironmentGameTimeFactor(const float value) = 0;
    virtual void SetGameTimeFactor(ALife::_TIME_ID gameTime, const float timeFactor) = 0;
    virtual void SetEnvironmentGameTimeFactor(ALife::_TIME_ID gameTime, const float timeFactor) = 0;
};
IC IGameState::~IGameState() {}

class game_GameState : public FactoryObjectBase, public virtual IGameState
{
protected:
    EGameIDs m_type;
    u16 m_phase;
    u32 m_start_time;

protected:
    virtual void switch_Phase(u32 new_phase);
    virtual void OnSwitchPhase(u32 old_phase, u32 new_phase){};

public:
    game_GameState();
    virtual ~game_GameState() {}
    // IGameState
    virtual EGameIDs Type() const override { return m_type; }
    virtual u16 Phase() const override { return m_phase; }
    virtual u32 StartTime() const override { return m_start_time; }
    virtual void Create(shared_str& options) override {}
    virtual const char* type_name() const override { return "base game"; }
    virtual ALife::_TIME_ID GetStartGameTime() override;
    virtual ALife::_TIME_ID GetGameTime() override;
    virtual float GetGameTimeFactor() override;
    virtual void SetGameTimeFactor(const float value) override;
    virtual ALife::_TIME_ID GetEnvironmentGameTime() override;
    virtual float GetEnvironmentGameTimeFactor() override;
    virtual void SetEnvironmentGameTimeFactor(const float value) override;
    virtual void SetGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor) override;
    virtual void SetEnvironmentGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor) override;

    static CLASS_ID getCLASS_ID(LPCSTR game_type_name, bool bServer);

private:
    // scripts
    u64 m_qwStartProcessorTime;
    u64 m_qwStartGameTime;
    float m_fTimeFactor;
    //-------------------------------------------------------
    u64 m_qwEStartProcessorTime;
    u64 m_qwEStartGameTime;
    float m_fETimeFactor;
};
