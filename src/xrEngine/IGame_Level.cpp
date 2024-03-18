#include "stdafx.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "CustomHUD.h"
#include "Render.h"
#include "GameFont.h"
#include "Common/LevelStructure.hpp"
#include "CameraManager.h"
#include "xr_object.h"
#include "Feel_Sound.h"

ENGINE_API IGame_Level* g_pGameLevel = NULL;
extern bool g_bLoaded;

IGame_Level::IGame_Level()
    : ObjectSpace(&g_pGamePersistent->SpatialSpace)
{
    m_pCameras = xr_new<CCameraManager>(true);
    g_pGameLevel = this;
    pLevel = NULL;
    bReady = false;
    pCurrentEntity = NULL;
    pCurrentViewEntity = NULL;
    Sound = GEnv.Sound->create_scene();
    DefaultSoundScene = Sound;
#ifndef MASTER_GOLD
    GEnv.Render->ResourcesDumpMemoryUsage();
#endif
}

IGame_Level::~IGame_Level()
{
    if (strstr(Core.Params, "-nes_texture_storing"))
        GEnv.Render->ResourcesStoreNecessaryTextures();
    xr_delete(pLevel);

    // Render-level unload
    GEnv.Render->level_Unload();
    xr_delete(m_pCameras);
    // Unregister
    Device.seqRender.Remove(this);
    Device.seqFrame.Remove(this);
    CCameraManager::ResetPP();
    ///////////////////////////////////////////
    DefaultSoundScene = g_pGamePersistent->m_pSound;
    GEnv.Sound->destroy_scene(Sound);
#ifndef MASTER_GOLD
    GEnv.Render->ResourcesDumpMemoryUsage();
#endif

    u32 m_base = 0, c_base = 0, m_lmaps = 0, c_lmaps = 0;
    if (GEnv.Render)
        GEnv.Render->ResourcesGetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);

    Msg("* [ D3D ]: textures[%d K]", (m_base + m_lmaps) / 1024);
}

void IGame_Level::net_Stop()
{
    // XXX: why update 6 times?
    for (int i = 0; i < 6; i++)
        Objects.Update(false);
    // Destroy all objects
    Objects.Unload();
    IR_Release();

    bReady = false;
}

//-------------------------------------------------------------------------------------------
// extern CStatTimer tscreate;

static void build_callback(Fvector* V, int Vcnt, CDB::TRI* T, int Tcnt, void* params)
{
    g_pGameLevel->Load_GameSpecific_CFORM(T, Tcnt);
}

static void serialize_callback(IWriter& writer)
{
    g_pGameLevel->Load_GameSpecific_CFORM_Serialize(writer);
}

static bool deserialize_callback(IReader& reader)
{
    return g_pGameLevel->Load_GameSpecific_CFORM_Deserialize(reader);
}

bool IGame_Level::Load(u32 dwNum)
{
    // Initialize level data
    g_pGamePersistent->Level_Set(dwNum);
    string_path temp;
    if (!FS.exist(temp, "$level$", "level.ltx"))
        xrDebug::Fatal(DEBUG_INFO, "Can't find level configuration file '%s'.", temp);
    pLevel = xr_new<CInifile>(temp);

    // Open
    g_pGamePersistent->LoadTitle("st_opening_stream");
    IReader* LL_Stream = FS.r_open("$level$", "level");
    IReader& fs = *LL_Stream;

    // Header
    hdrLEVEL H;
    fs.r_chunk_safe(fsL_HEADER, &H, sizeof(H));
    R_ASSERT2(XRCL_PRODUCTION_VERSION == H.XRLC_version, "Incompatible level version.");

    // CForms
    g_pGamePersistent->LoadTitle("st_loading_cform");

    ObjectSpace.Load(build_callback, serialize_callback, deserialize_callback);
    g_pGamePersistent->SpatialSpace.initialize(ObjectSpace.GetBoundingVolume());
    g_pGamePersistent->SpatialSpacePhysic.initialize(ObjectSpace.GetBoundingVolume());

    Sound->set_geometry_occ(ObjectSpace.GetStaticModel(), ObjectSpace.GetBoundingVolume());
    Sound->set_handler([](const ref_sound& S, float range)
    {
        if (g_pGameLevel && S && S->feedback)
            g_pGameLevel->SoundEvent_Register(S, range);
    });

    // Render-level Load
    GEnv.Render->level_Load(LL_Stream);
    // tscreate.FrameEnd ();
    // Msg ("* S-CREATE: %f ms, %d times",tscreate.result,tscreate.count);

    // Objects
    g_pGamePersistent->Environment().mods_load();
    R_ASSERT(Load_GameSpecific_Before());
    Objects.Load();
    //. ANDY R_ASSERT (Load_GameSpecific_After ());

    // Done
    FS.r_close(LL_Stream);
    bReady = true;

    if (!GEnv.isDedicatedServer)
    {
        IR_Capture();
        Device.seqRender.Add(this);
    }

    Device.seqFrame.Add(this);
    return true;
}

int psNET_DedicatedSleep = 5;
void IGame_Level::OnRender()
{
    if (GEnv.isDedicatedServer)
    {
        Sleep(psNET_DedicatedSleep);
        return;
    }

    // if (_abs(Device.fTimeDelta)<EPS_S) return;

#ifdef _GPA_ENABLED
    TAL_ID rtID = TAL_MakeID(1, Core.dwFrame, 0);
    TAL_CreateID(rtID);
    TAL_BeginNamedVirtualTaskWithID("GameRenderFrame", rtID);
    TAL_Parami("Frame#", Device.dwFrame);
    TAL_EndVirtualTask();
#endif // _GPA_ENABLED

    // Level render, only when no client output required
    GEnv.Render->Calculate();
    GEnv.Render->Render();

#ifdef _GPA_ENABLED
    TAL_RetireID(rtID);
#endif // _GPA_ENABLED

    // Font
    // pApp->pFontSystem->SetSizeI(0.023f);
    // pApp->pFontSystem->OnRender();
}

void IGame_Level::OnFrame()
{
    SoundEvent_Dispatch();

    // Log ("- level:on-frame: ",u32(Device.dwFrame));
    // if (_abs(Device.fTimeDelta)<EPS_S) return;

    // Update all objects
    VERIFY(bReady);
    Objects.Update(false);
    pHUD->OnFrame();

    // Ambience
    if (Sounds_Random.size() && (Device.dwTimeGlobal > Sounds_Random_dwNextTime))
    {
        Sounds_Random_dwNextTime = Device.dwTimeGlobal + ::Random.randI(10000, 20000);
        Fvector pos;
        pos.random_dir().normalize().mul(::Random.randF(30, 100)).add(Device.vCameraPosition);
        int id = ::Random.randI(Sounds_Random.size());
        if (Sounds_Random_Enabled)
        {
            Sounds_Random[id].play_at_pos(0, pos, 0);
            Sounds_Random[id].set_volume(1.f);
            Sounds_Random[id].set_range(10, 200);
        }
    }
}

void IGame_Level::DumpStatistics(IGameFont& font, IPerformanceAlert* alert) { Objects.DumpStatistics(font, alert); }
// ==================================================================================================

void CServerInfo::AddItem(pcstr name_, pcstr value_, u32 color_)
{
    shared_str s_name(name_);
    AddItem(s_name, value_, color_);
}

void CServerInfo::AddItem(shared_str& name_, pcstr value_, u32 color_)
{
    SItem_ServerInfo it;
    // shared_str s_name = CStringTable().translate( name_ );

    // xr_strcpy( it.name, s_name.c_str() );
    xr_strcpy(it.name, name_.c_str());
    xr_strcat(it.name, " = ");
    xr_strcat(it.name, value_);
    it.color = color_;

    if (data.size() < max_item)
    {
        data.push_back(it);
    }
}

void IGame_Level::SetEntity(IGameObject* O)
{
    if (pCurrentEntity)
        pCurrentEntity->On_LostEntity();

    if (O)
        O->On_SetEntity();

    pCurrentEntity = pCurrentViewEntity = O;
}

void IGame_Level::SetViewEntity(IGameObject* O)
{
    if (pCurrentViewEntity)
        pCurrentViewEntity->On_LostEntity();

    if (O)
        O->On_SetEntity();

    pCurrentViewEntity = O;
}

void IGame_Level::SoundEvent_Register(const ref_sound& S, float range)
{
    if (!g_bLoaded)
        return;
    if (!S)
        return;
    if (S->g_object && S->g_object->getDestroy())
    {
        S->g_object = 0;
        return;
    }
    if (0 == S->feedback)
        return;

    clamp(range, 0.1f, 500.f);

    const CSound_params* p = S->feedback->get_params();
    Fvector snd_position = p->position;
    if (S->feedback->is_2D())
    {
        snd_position.add(GEnv.Sound->listener_position());
    }

    VERIFY(p && _valid(range));
    range = std::min(range, p->max_ai_distance);
    VERIFY(_valid(snd_position));
    VERIFY(_valid(p->max_ai_distance));
    VERIFY(_valid(p->volume));

    // Query objects
    Fvector bb_size = {range, range, range};
    g_pGamePersistent->SpatialSpace.q_box(snd_ER, 0, STYPE_REACTTOSOUND, snd_position, bb_size);

    // Iterate
    for (auto& it : snd_ER)
    {
        Feel::Sound* L = it->dcast_FeelSound();
        if (0 == L)
            continue;
        IGameObject* CO = it->dcast_GameObject();
        VERIFY(CO);
        if (CO->getDestroy())
            continue;

        // Energy and signal
        VERIFY(_valid(it->GetSpatialData().sphere.P));
        const float dist = snd_position.distance_to(it->GetSpatialData().sphere.P);
        if (dist > p->max_ai_distance)
            continue;
        VERIFY(_valid(dist));
        VERIFY2(!fis_zero(p->max_ai_distance), S->handle->file_name());
        float Power = (1.f - dist / p->max_ai_distance) * p->volume;
        VERIFY(_valid(Power));
        if (Power > EPS_S)
        {
            const float occ = Sound->get_occlusion_to(it->GetSpatialData().sphere.P, snd_position);
            VERIFY(_valid(occ));
            Power *= occ;
            if (Power > EPS_S)
            {
                _esound_delegate D = {L, S, Power};
                snd_Events.push_back(D);
            }
        }
    }
    snd_ER.clear();
}

void IGame_Level::SoundEvent_Dispatch()
{
    while (!snd_Events.empty())
    {
        _esound_delegate& D = snd_Events.back();
        VERIFY(D.dest && D.source);
        if (D.source->feedback)
        {
            D.dest->feel_sound_new(D.source->g_object, D.source->g_type, D.source->g_userdata,

                D.source->feedback->is_2D() ? Device.vCameraPosition : D.source->feedback->get_params()->position,
                D.power);
        }
        snd_Events.pop_back();
    }
}

// Lain: added
void IGame_Level::SoundEvent_OnDestDestroy(Feel::Sound* obj)
{
    struct rem_pred
    {
        rem_pred(Feel::Sound* obj) : m_obj(obj) {}
        bool operator()(const _esound_delegate& d) { return d.dest == m_obj; }
    private:
        Feel::Sound* m_obj;
    };

    snd_Events.erase(std::remove_if(snd_Events.begin(), snd_Events.end(), rem_pred(obj)), snd_Events.end());
}
