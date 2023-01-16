#pragma once
#ifndef igame_level_h_defined
#define igame_level_h_defined

#include "IInputReceiver.h"
#include "xr_object_list.h"
#include "xrCDB/xr_area.h"
#include "xrSound/Sound.h"
#include "xrCore/FixedVector.h"
#include "EngineAPI.h"
#include "EventAPI.h"
#include "pure.h"

// refs
class ENGINE_API CCameraManager;
class ENGINE_API CCursor;
class ENGINE_API CCustomHUD;
class ENGINE_API ISpatial;
namespace Feel
{
class ENGINE_API Sound;
}

class ENGINE_API CServerInfo
{
private:
    struct SItem_ServerInfo
    {
        string128 name;
        u32 color;
    };
    enum
    {
        max_item = 15
    };
    svector<SItem_ServerInfo, max_item> data;

public:
    u32 Size() { return data.size(); }
    void ResetData() { data.clear(); }
    void AddItem(pcstr name_, pcstr value_, u32 color_ = RGB(255, 255, 255));
    void AddItem(shared_str& name_, pcstr value_, u32 color_ = RGB(255, 255, 255));

    IC SItem_ServerInfo& operator[](u32 id)
    {
        VERIFY(id < max_item);
        return data[id];
    }

    CServerInfo(){};
    ~CServerInfo(){};
};

//-----------------------------------------------------------------------------------------------------------
class ENGINE_API IGame_Level : public FactoryObjectBase,
                               public IInputReceiver,
                               public pureRender,
                               public pureFrame,
                               public IEventReceiver
{
    bool m_world_rendered;

protected:
    // Network interface
    IGameObject* pCurrentEntity;
    IGameObject* pCurrentViewEntity;

    // Static sounds
    xr_vector<ref_sound> Sounds_Random;
    u32 Sounds_Random_dwNextTime;
    bool Sounds_Random_Enabled;
    CCameraManager* m_pCameras;

    // temporary
    xr_vector<ISpatial*> snd_ER;

public:
    CObjectList Objects;
    CObjectSpace ObjectSpace;
    CCameraManager& Cameras() { return *m_pCameras; };
    bool bReady;

    CInifile* pLevel;

public: // deferred sound events
    struct _esound_delegate
    {
        Feel::Sound* dest;
        ref_sound_data_ptr source;
        float power;
    };
    xr_vector<_esound_delegate> snd_Events;

public:
    // Main, global functions
    IGame_Level();
    virtual ~IGame_Level();

    virtual shared_str name() const = 0;
    virtual void GetLevelInfo(CServerInfo* si) = 0;

    virtual bool net_Start(const char* op_server, const char* op_client) = 0;
    virtual void net_Load(const char* name) = 0;
    virtual void net_Save(const char* name) = 0;
    virtual void net_Stop();
    virtual void net_Update() = 0;

    virtual bool Load(u32 dwNum);
    virtual bool Load_GameSpecific_Before() { return true; }; // before object loading
    virtual bool Load_GameSpecific_After() { return true; }; // after object loading
    virtual void Load_GameSpecific_CFORM(CDB::TRI* T, u32 count) = 0;

    virtual void OnFrame(void);
    virtual void OnRender(void);
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);

    [[nodiscard]] bool WorldRendered() const { return m_world_rendered; }
    void WorldRendered(bool rendered) { m_world_rendered = rendered; }

    virtual shared_str OpenDemoFile(const char* demo_file_name) = 0;
    virtual void net_StartPlayDemo() = 0;

    // Main interface
    IGameObject* CurrentEntity(void) const { return pCurrentEntity; }
    IGameObject* CurrentViewEntity(void) const { return pCurrentViewEntity; }
    void SetEntity(IGameObject* O); // { pCurrentEntity=pCurrentViewEntity=O; }
    void SetViewEntity(IGameObject* O); // { pCurrentViewEntity=O; }

    void SoundEvent_Register(ref_sound_data_ptr S, float range);
    void SoundEvent_Dispatch();
    void SoundEvent_OnDestDestroy(Feel::Sound*);

    // Loader interface
    // ref_shader LL_CreateShader (int S, int T, int M, int C);
    void LL_CheckTextures();
    virtual void SetEnvironmentGameTimeFactor(u64 const& GameTime, float const& fTimeFactor) = 0;
};

//-----------------------------------------------------------------------------------------------------------
extern ENGINE_API IGame_Level* g_pGameLevel;

#endif
