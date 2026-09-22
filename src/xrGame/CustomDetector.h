#pragma once

#include <functional>

#include "xrEngine/Feel_Touch.h"

#include "HudSound.h"
#include "map_manager.h"
#include "CustomZone.h"
#include "Artefact.h"

struct ITEM_TYPE
{
    Fvector2 freq; // min,max
    HUD_SOUND_ITEM detect_snds;

    shared_str zone_map_location;
    shared_str nightvision_particle;
    float radius{ type_max<float> };
};

//описание зоны, обнаруженной детектором
struct ITEM_INFO
{
    ITEM_TYPE* curr_ref{};
    float snd_time{};
    //текущая частота работы датчика
    float cur_period{};
    // particle for night-vision mode
    CParticlesObject* pParticle{};

    ~ITEM_INFO();
};

template <typename K>
class CDetectList : public Feel::Touch
{
protected:
    xr_unordered_map<shared_str, ITEM_TYPE> m_TypesMap;

public:
    xr_map<K*, ITEM_INFO> m_ItemInfos;

protected:
    void feel_touch_new(IGameObject* O) override
    {
        K* pK = smart_cast<K*>(O);
        R_ASSERT1_CURE(pK, { return; });
        const auto it = m_TypesMap.find(O->cNameSect());
        R_ASSERT1_CURE(it != m_TypesMap.end(), { return; });
        m_ItemInfos[pK].snd_time = 0.0f;
        m_ItemInfos[pK].curr_ref = &(it->second);
        add_remove_map_spot(pK, true);
    }

    void feel_touch_delete(IGameObject* O) override
    {
        K* pK = smart_cast<K*>(O);
        R_ASSERT1_CURE(pK, { return; });
        m_ItemInfos.erase(pK);
        add_remove_map_spot(pK, false);
    }

public:
    void destroy()
    {
        for (auto& [_, item_type] : m_TypesMap)
            HUD_SOUND_ITEM::DestroySound(item_type.detect_snds);
    }

    void clear()
    {
        m_ItemInfos.clear();
        Feel::Touch::feel_touch.clear();
    }

    void load(cpcstr sect, cpcstr prefix, const std::function<ITEM_TYPE(const shared_str&)>& custom_defaults_handler = {})
    {
        u32 i = 1;
        while (true)
        {
            string256 temp;
            xr_sprintf(temp, "%s_class_%d", prefix, i);
            if (!pSettings->line_exist(sect, temp))
                break;

            pcstr item_sect = pSettings->r_string(sect, temp);
            if (!item_sect || !pSettings->section_exist(item_sect))
            {
#ifndef MASTER_GOLD
                Msg("! [CDetectList] Base section[%s], line[%s]: item section[%s] cannot be loaded", sect, temp, item_sect);
#endif
                continue;
            }

            m_TypesMap.insert(std::make_pair(item_sect, ITEM_TYPE()));
            ITEM_TYPE& item_type = m_TypesMap[item_sect];

            if (custom_defaults_handler)
            {
                item_type = custom_defaults_handler(item_sect);
            }

            // min/max freq is a SOC format, in Hz
            xr_sprintf(temp, "%s_min_freq_%d", prefix, i);
            if (pSettings->line_exist(sect, temp))
                item_type.freq.y = 1.0f / pSettings->r_float(sect, temp);

            xr_sprintf(temp, "%s_max_freq_%d", prefix, i);
            if (pSettings->line_exist(sect, temp))
                item_type.freq.x = 1.0f / pSettings->r_float(sect, temp);

            // Loading directly as Fvector2 is a CS/COP format in periods
            xr_sprintf(temp, "%s_freq_%d", prefix, i);
            item_type.freq = pSettings->read_if_exists(sect, temp, item_type.freq);

            xr_sprintf(temp, "%s_sound_%d_", prefix, i);
            HUD_SOUND_ITEM::LoadSound(sect, temp, item_type.detect_snds, SOUND_TYPE_ITEM);

            xr_sprintf(temp, "%s_map_location_%d", prefix, i);
            if (pSettings->line_exist(sect, temp))
                item_type.zone_map_location = pSettings->r_string(sect, temp);

            // https://github.com/OGSR/OGSR-Engine/commit/328216a653dec58ff024a44b12d44c76f859cf8a
            xr_sprintf(temp, "%s_radius_%d", prefix, i);
            item_type.radius = pSettings->read_if_exists(sect, temp, item_type.radius);

            xr_sprintf(temp, "%s_night_vision_particle_%d", prefix, i);
            if (pSettings->line_exist(sect, temp))
                item_type.nightvision_particle = pSettings->r_string(sect, temp);

            ++i;
        } // while (true)

        // Add all items that have same classes to simulate SOC/CS behaviour
        if (ShadowOfChernobylMode || ClearSkyMode)
        {
            for (CInifile::Sect* section : pSettings->sections())
            {
                const auto& name = section->Name;
                if (name.empty())
                    continue;

                if (!section->line_exist("class"))
                    continue;

                const auto clsid = pSettings->r_clsid(name, "class");

                for (auto& [item_sect, item_type] : m_TypesMap)
                {
                    CLASS_ID item_cls = TEXT2CLSID(pSettings->r_string(item_sect, "class"));
                    if (clsid == item_cls)
                        m_TypesMap.emplace(name, item_type);
                }
            }
        }
    }

    void update_sound(ITEM_INFO& item_info, const IGameObject* parent, const float relative_signal_power, const bool set_frequency)
    {
        ITEM_TYPE& item_type = *item_info.curr_ref;

        if (item_type.detect_snds.sounds.empty())
            return;

        //определить текущую частоту срабатывания сигнала
        item_info.cur_period = item_type.freq.x + (item_type.freq.y - item_type.freq.x) * (relative_signal_power * relative_signal_power);

        if (item_info.snd_time > item_info.cur_period)
        {
            item_info.snd_time = 0.0f;
            HUD_SOUND_ITEM::PlaySound(item_type.detect_snds, {}, parent, true, false);

            if (item_type.detect_snds.m_activeSnd && set_frequency)
            {
                constexpr float min_snd_freq = 0.9f;
                constexpr float max_snd_freq = 1.4f;

                const float snd_freq = min_snd_freq + (max_snd_freq - min_snd_freq) * (1.0f - relative_signal_power);

                item_type.detect_snds.m_activeSnd->snd.set_frequency(snd_freq);
            }
        }
        else
        {
            item_info.snd_time += Device.fTimeDelta;
        }
    }

    void add_remove_map_spot(K* zone, const bool add)
    {
        const auto it = m_TypesMap.find(zone->cNameSect());
        if (it == m_TypesMap.end())
            return;

        const auto& item_type = it->second;

        if (item_type.zone_map_location.empty())
            return;

        auto& map_manager = Level().MapManager();
        if (add)
        {
            if (auto* location = map_manager.GetMapLocation(item_type.zone_map_location.c_str(), zone->ID()))
                location->UpdateTTL();
            else
                map_manager.AddMapLocation(item_type.zone_map_location.c_str(), zone->ID());
        }
        else
        {
            map_manager.RemoveMapLocation(item_type.zone_map_location.c_str(), zone->ID());
        }
    }

    void update_night_vision(const bool enable_particle)
    {
        for (auto& [object, item_info] : m_ItemInfos)
        {
            ITEM_TYPE& item_type = *item_info.curr_ref;

            const auto& nightvision_particle = item_type.nightvision_particle;

            if (enable_particle && !nightvision_particle.empty())
            {
                if (!item_info.pParticle)
                    item_info.pParticle = CParticlesObject::Create(nightvision_particle.c_str(), false);

                item_info.pParticle->UpdateParent(object->XFORM(), {});
                if (!item_info.pParticle->IsPlaying())
                    item_info.pParticle->Play(false);
            }
            else
            {
                if (item_info.pParticle)
                {
                    item_info.pParticle->Stop();
                    CParticlesObject::Destroy(item_info.pParticle);
                }
            }
        }
    }
};

class CZoneList final : public CDetectList<CCustomZone>
{
protected:
    bool feel_touch_contact(IGameObject* O) override;

public:
    void scan(Fvector detector_position, const IGameObject* parent);

    float zone_cur_power[ALife::infl_max_count]{};
    bool anomalies_always_visible{};
};

class CAfList final : public CDetectList<CArtefact>
{
protected:
    bool feel_touch_contact(IGameObject* O) override;

public:
    // returns nearest artefact and relative detector signal power
    std::pair<CArtefact*, float> scan(Fvector detector_position, const IGameObject* parent, float visibility_radius);

    int m_af_rank{};
};

class CUIArtefactDetectorBase;

class CCustomDetector : public CHudItemObject
{
    typedef CHudItemObject inherited;

protected:
    CUIArtefactDetectorBase* m_ui{};
    bool m_bFastAnimMode{};
    bool m_bNeedActivation{};

public:
    virtual ~CCustomDetector();

    virtual bool net_Spawn(CSE_Abstract* DC);
    virtual void Load(LPCSTR section);

    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);

    virtual void shedule_Update(u32 dt);
    virtual void UpdateCL();

    bool IsWorking();

    void OnMoveToSlot(const SInvItemPlace& prev) override;
    void OnMoveToRuck(const SInvItemPlace& prev) override;
    void OnMoveToBelt(const SInvItemPlace& prev) override;

    virtual void OnActiveItem();
    virtual void OnHiddenItem();
    virtual void OnStateSwitch(u32 S, u32 oldState);
    virtual void OnAnimationEnd(u32 state);
    virtual void UpdateXForm();

    void ToggleDetector(bool bFastMode);
    void HideDetector(bool bFastMode);
    void ShowDetector(bool bFastMode);

    virtual bool CheckCompatibility(CHudItem* itm);

    virtual u32 ef_detector_type() const { return 1; }

    auto GetAfDetectRadius() const { return m_fAfVisRadius; }

protected:
    bool CheckCompatibilityInt(CHudItem* itm, u16* slot_to_activate);
    void TurnDetectorInternal(bool on);
    void UpdateNightVisionMode(bool on);
    void UpdateVisibility();

    virtual void Scan();
    virtual void CreateUI() {}

    bool m_bWorking{};
    float m_fRadius{ 10.0f };
    float m_fAfDetectRadius{ 30.0f };
    float m_fAfVisRadius{ 3.0f };
    float m_fDecayRate{}; //Alundaio
    CAfList m_artefacts;
    CZoneList m_zones;
};
