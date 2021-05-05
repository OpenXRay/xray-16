#include "pch_script.h"
#include "UIInventoryUtilities.h"
#include "WeaponAmmo.h"
#include "xrUICore/Static/UIStaticItem.h"
#include "xrUICore/Static/UIStatic.h"
#include "eatable_item.h"
#include "Level.h"
#include "date_time.h"
#include "string_table.h"
#include "Inventory.h"
#include "InventoryOwner.h"
#include "InfoPortion.h"
#include "game_base_space.h"
#include "Actor.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "Include/xrRender/UIShader.h"

#define BUY_MENU_TEXTURE "ui" DELIMITER "ui_mp_buy_menu"
#define EQUIPMENT_ICONS  "ui" DELIMITER "ui_icon_equipment"
#define CHAR_ICONS "ui" DELIMITER "ui_icons_npc"
#define MAP_ICONS "ui" DELIMITER "ui_icons_map"
#define MP_CHAR_ICONS "ui" DELIMITER "ui_models_multiplayer"

const LPCSTR relationsLtxSection = "game_relations";
const LPCSTR ratingField = "rating_names";
const LPCSTR reputationgField = "reputation_names";
const LPCSTR goodwillField = "goodwill_names";

const LPCSTR st_months[12] = // StringTable for GetDateAsString()
    {"month_january", "month_february", "month_march", "month_april", "month_may", "month_june", "month_july",
        "month_august", "month_september", "month_october", "month_november", "month_december"};

ui_shader* g_BuyMenuShader = NULL;
ui_shader* g_EquipmentIconsShader = NULL;
ui_shader* g_MPCharIconsShader = NULL;
ui_shader* g_OutfitUpgradeIconsShader = NULL;
ui_shader* g_WeaponUpgradeIconsShader = NULL;
static CUIStatic* GetUIStatic();

typedef std::pair<CHARACTER_RANK_VALUE, shared_str> CharInfoStringID;
using CharInfoStrings = xr_map<CHARACTER_RANK_VALUE, shared_str>;

CharInfoStrings* charInfoReputationStrings = NULL;
CharInfoStrings* charInfoRankStrings = NULL;
CharInfoStrings* charInfoGoodwillStrings = NULL;

static class InventoryUtilitiesReset : public CUIResetNotifier, public pureAppStart, public pureAppEnd
{
public:
    void OnAppStart() override
    {
        if (GEnv.isDedicatedServer)
            return;
        InventoryUtilities::CreateShaders();
    }

    void OnAppEnd() override
    {
        if (GEnv.isDedicatedServer)
            return;
        InventoryUtilities::DestroyShaders();
    }

    void OnUIReset() override
    {
        OnAppEnd();
        OnAppStart();
    }
} s_inventory_utilities_reset;

void InventoryUtilities::CreateShaders()
{
    // Nothing here. All needed shaders will be created on demand
}

void InventoryUtilities::DestroyShaders()
{
    xr_delete(g_BuyMenuShader);
    g_BuyMenuShader = 0;

    xr_delete(g_EquipmentIconsShader);
    g_EquipmentIconsShader = 0;

    xr_delete(g_MPCharIconsShader);
    g_MPCharIconsShader = 0;

    xr_delete(g_OutfitUpgradeIconsShader);
    g_OutfitUpgradeIconsShader = 0;

    xr_delete(g_WeaponUpgradeIconsShader);
    g_WeaponUpgradeIconsShader = 0;
}

bool InventoryUtilities::GreaterRoomInRuck(PIItem item1, PIItem item2)
{
    Ivector2 r1, r2;
    r1 = item1->GetInvGridRect().rb;
    r2 = item2->GetInvGridRect().rb;

    if (r1.x > r2.x)
        return true;

    if (r1.x == r2.x)
    {
        if (r1.y > r2.y)
            return true;

        if (r1.y == r2.y)
        {
            if (item1->object().cNameSect() == item2->object().cNameSect())
                return (item1->object().ID() > item2->object().ID());
            else
                return (item1->object().cNameSect() > item2->object().cNameSect());
        }

        return false;
    }
    return false;
}

bool InventoryUtilities::FreeRoom_inBelt(TIItemContainer& item_list, PIItem _item, int width, int height)
{
    bool* ruck_room = (bool*)alloca(width * height);

    int i, j, k, m;
    int place_row = 0, place_col = 0;
    bool found_place;
    bool can_place;

    for (i = 0; i < height; ++i)
        for (j = 0; j < width; ++j)
            ruck_room[i * width + j] = false;

    item_list.push_back(_item);
    std::sort(item_list.begin(), item_list.end(), GreaterRoomInRuck);

    found_place = true;

    for (xr_vector<PIItem>::iterator it = item_list.begin(); (item_list.end() != it) && found_place; ++it)
    {
        PIItem pItem = *it;
        Ivector2 iWH = pItem->GetInvGridRect().rb;
        //проверить можно ли разместить элемент,
        //проверяем последовательно каждую клеточку
        found_place = false;

        for (i = 0; (i < height - iWH.y + 1) && !found_place; ++i)
        {
            for (j = 0; (j < width - iWH.x + 1) && !found_place; ++j)
            {
                can_place = true;

                for (k = 0; (k < iWH.y) && can_place; ++k)
                {
                    for (m = 0; (m < iWH.x) && can_place; ++m)
                    {
                        if (ruck_room[(i + k) * width + (j + m)])
                            can_place = false;
                    }
                }

                if (can_place)
                {
                    found_place = true;
                    place_row = i;
                    place_col = j;
                }
            }
        }

        //разместить элемент на найденном месте
        if (found_place)
        {
            for (k = 0; k < iWH.y; ++k)
            {
                for (m = 0; m < iWH.x; ++m)
                {
                    ruck_room[(place_row + k) * width + place_col + m] = true;
                }
            }
        }
    }

    // remove
    item_list.erase(std::remove(item_list.begin(), item_list.end(), _item), item_list.end());

    //для какого-то элемента места не нашлось
    if (!found_place)
        return false;

    return true;
}

const ui_shader& InventoryUtilities::GetBuyMenuShader()
{
    if (!g_BuyMenuShader)
    {
        g_BuyMenuShader = xr_new<ui_shader>();
        (*g_BuyMenuShader)->create("hud" DELIMITER "default", BUY_MENU_TEXTURE);
    }

    return *g_BuyMenuShader;
}

const ui_shader& InventoryUtilities::GetEquipmentIconsShader()
{
    if (!g_EquipmentIconsShader)
    {
        g_EquipmentIconsShader = xr_new<ui_shader>();
        (*g_EquipmentIconsShader)->create("hud" DELIMITER "default", EQUIPMENT_ICONS);
    }

    return *g_EquipmentIconsShader;
}

const ui_shader& InventoryUtilities::GetMPCharIconsShader()
{
    if (!g_MPCharIconsShader)
    {
        g_MPCharIconsShader = xr_new<ui_shader>();
        (*g_MPCharIconsShader)->create("hud" DELIMITER "default", MP_CHAR_ICONS);
    }

    return *g_MPCharIconsShader;
}

const ui_shader& InventoryUtilities::GetOutfitUpgradeIconsShader()
{
    if (!g_OutfitUpgradeIconsShader)
    {
        g_OutfitUpgradeIconsShader = xr_new<ui_shader>();
        (*g_OutfitUpgradeIconsShader)->create("hud" DELIMITER "default", "ui" DELIMITER "ui_actor_armor");
    }

    return *g_OutfitUpgradeIconsShader;
}

const ui_shader& InventoryUtilities::GetWeaponUpgradeIconsShader()
{
    if (!g_WeaponUpgradeIconsShader)
    {
        g_WeaponUpgradeIconsShader = xr_new<ui_shader>();
        (*g_WeaponUpgradeIconsShader)->create("hud" DELIMITER "default", "ui" DELIMITER "ui_actor_weapons");
    }

    return *g_WeaponUpgradeIconsShader;
}

//////////////////////////////////////////////////////////////////////////

const shared_str InventoryUtilities::GetGameDateAsString(EDatePrecision datePrec, char dateSeparator)
{
    return GetDateAsString(Level().GetGameTime(), datePrec, dateSeparator);
}

//////////////////////////////////////////////////////////////////////////

const shared_str InventoryUtilities::GetGameTimeAsString(ETimePrecision timePrec, char timeSeparator)
{
    return GetTimeAsString(Level().GetGameTime(), timePrec, timeSeparator);
}

const shared_str InventoryUtilities::GetTimeAndDateAsString(ALife::_TIME_ID time)
{
    string256 buf;
    LPCSTR time_str = GetTimeAsString(time, etpTimeToMinutes).c_str();
    LPCSTR date_str = GetDateAsString(time, edpDateToDay).c_str();
    strconcat(sizeof(buf), buf, time_str, ", ", date_str);
    return buf;
}

const shared_str InventoryUtilities::Get_GameTimeAndDate_AsString()
{
    return GetTimeAndDateAsString(Level().GetGameTime());
}

//////////////////////////////////////////////////////////////////////////

const shared_str InventoryUtilities::GetTimeAsString(
    ALife::_TIME_ID time, ETimePrecision timePrec, char timeSeparator, bool full_mode)
{
    string32 bufTime;

    ZeroMemory(bufTime, sizeof(bufTime));

    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

    split_time(time, year, month, day, hours, mins, secs, milisecs);

    // Time
    switch (timePrec)
    {
    case etpTimeToHours: xr_sprintf(bufTime, "%02i", hours); break;
    case etpTimeToMinutes:
        if (full_mode || hours > 0)
        {
            xr_sprintf(bufTime, "%02i%c%02i", hours, timeSeparator, mins);
            break;
        }
        xr_sprintf(bufTime, "0%c%02i", timeSeparator, mins);
        break;
    case etpTimeToSeconds:
        if (full_mode || hours > 0)
        {
            xr_sprintf(bufTime, "%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs);
            break;
        }
        if (mins > 0)
        {
            xr_sprintf(bufTime, "%02i%c%02i", mins, timeSeparator, secs);
            break;
        }
        xr_sprintf(bufTime, "0%c%02i", timeSeparator, secs);
        break;
    case etpTimeToMilisecs:
        xr_sprintf(bufTime, "%02i%c%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs, timeSeparator,
            milisecs);
        break;
    case etpTimeToSecondsAndDay:
    {
        int total_day = (int)(time / (1000 * 60 * 60 * 24));
        xr_sprintf(bufTime, sizeof(bufTime), "%dd %02i%c%02i%c%02i", total_day, hours, timeSeparator, mins,
            timeSeparator, secs);
        break;
    }
    default: R_ASSERT(!"Unknown type of date precision");
    }

    return bufTime;
}

const shared_str InventoryUtilities::GetDateAsString(ALife::_TIME_ID date, EDatePrecision datePrec, char dateSeparator)
{
    string64 bufDate;

    ZeroMemory(bufDate, sizeof(bufDate));

    u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

    split_time(date, year, month, day, hours, mins, secs, milisecs);
    VERIFY(1 <= month && month <= 12);
    LPCSTR month_str = StringTable().translate(st_months[month - 1]).c_str();

    // Date
    switch (datePrec)
    {
    case edpDateToYear: xr_sprintf(bufDate, "%04i", year); break;
    case edpDateToMonth: xr_sprintf(bufDate, "%s%c% 04i", month_str, dateSeparator, year); break;
    case edpDateToDay: xr_sprintf(bufDate, "%s %d%c %04i", month_str, day, dateSeparator, year); break;
    default: R_ASSERT(!"Unknown type of date precision");
    }

    return bufDate;
}

LPCSTR InventoryUtilities::GetTimePeriodAsString(pstr _buff, u32 buff_sz, ALife::_TIME_ID _from, ALife::_TIME_ID _to)
{
    u32 year1, month1, day1, hours1, mins1, secs1, milisecs1;
    u32 year2, month2, day2, hours2, mins2, secs2, milisecs2;

    split_time(_from, year1, month1, day1, hours1, mins1, secs1, milisecs1);
    split_time(_to, year2, month2, day2, hours2, mins2, secs2, milisecs2);

    int cnt = 0;
    _buff[0] = 0;

    if (month1 != month2)
        cnt = xr_sprintf(
            _buff + cnt, buff_sz - cnt, "%d %s ", month2 - month1, *StringTable().translate("ui_st_months"));

    if (!cnt && day1 != day2)
        cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", day2 - day1, *StringTable().translate("ui_st_days"));

    if (!cnt && hours1 != hours2)
        cnt =
            xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", hours2 - hours1, *StringTable().translate("ui_st_hours"));

    if (!cnt && mins1 != mins2)
        cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", mins2 - mins1, *StringTable().translate("ui_st_mins"));

    if (!cnt && secs1 != secs2)
        cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", secs2 - secs1, *StringTable().translate("ui_st_secs"));

    return _buff;
}

void InventoryUtilities::UpdateWeight(CUIStatic& wnd, CInventoryOwner* pInvOwner, bool withPrefix /*= false*/)
{
    R_ASSERT(pInvOwner);
    string128 buf;

    float total = pInvOwner->inventory().CalcTotalWeight();
    float max = pInvOwner->MaxCarryWeight();

    string16 cl;

    if (total > max)
        xr_strcpy(cl, "%c[red]");
    else
        xr_strcpy(cl, "%c[UI_orange]");

    string32 prefix;

    if (withPrefix)
        xr_sprintf(prefix, "%%c[default]%s ", StringTable().translate("ui_inv_weight").c_str());
    else
        xr_strcpy(prefix, "");

    xr_sprintf(buf, "%s%s%3.1f %s/%5.1f", prefix, cl, total, "%c[UI_orange]", max);
    wnd.SetText(buf);
}

//////////////////////////////////////////////////////////////////////////

void InventoryUtilities::UpdateWeightStr(CUITextWnd& wnd, CUITextWnd& wnd_max, CInventoryOwner* pInvOwner)
{
    R_ASSERT(pInvOwner);
    string128 buf;

    float total = pInvOwner->inventory().CalcTotalWeight();
    float max = pInvOwner->MaxCarryWeight();

    LPCSTR kg_str = StringTable().translate("st_kg").c_str();
    xr_sprintf(buf, "%.1f %s", total, kg_str);
    wnd.SetText(buf);

    xr_sprintf(buf, "(max %.1f %s)", max, kg_str);
    wnd_max.SetText(buf);
}

//////////////////////////////////////////////////////////////////////////

void LoadStrings(CharInfoStrings* container, LPCSTR section, LPCSTR field)
{
    R_ASSERT(container);

    LPCSTR cfgRecord = pSettings->r_string(section, field);
    u32 count = _GetItemCount(cfgRecord);
    R_ASSERT3(count % 2, "there're must be an odd number of elements", field);
    string64 singleThreshold;
    ZeroMemory(singleThreshold, sizeof(singleThreshold));
    int upBoundThreshold = 0;
    CharInfoStringID id;

    for (u32 k = 0; k < count; k += 2)
    {
        _GetItem(cfgRecord, k, singleThreshold);
        id.second = singleThreshold;

        _GetItem(cfgRecord, k + 1, singleThreshold);
        if (k + 1 != count)
            sscanf(singleThreshold, "%i", &upBoundThreshold);
        else
            upBoundThreshold += 1;

        id.first = upBoundThreshold;

        container->insert(id);
    }
}

//////////////////////////////////////////////////////////////////////////

void InitCharacterInfoStrings()
{
    if (charInfoReputationStrings && charInfoRankStrings)
        return;

    if (!charInfoReputationStrings)
    {
        // Create string->Id DB
        charInfoReputationStrings = xr_new<CharInfoStrings>();
        // Reputation
        LoadStrings(charInfoReputationStrings, relationsLtxSection, reputationgField);
    }

    if (!charInfoRankStrings)
    {
        // Create string->Id DB
        charInfoRankStrings = xr_new<CharInfoStrings>();
        // Ranks
        LoadStrings(charInfoRankStrings, relationsLtxSection, ratingField);
    }

    if (!charInfoGoodwillStrings)
    {
        // Create string->Id DB
        charInfoGoodwillStrings = xr_new<CharInfoStrings>();
        // Goodwills
        LoadStrings(charInfoGoodwillStrings, relationsLtxSection, goodwillField);
    }
}

//////////////////////////////////////////////////////////////////////////

void InventoryUtilities::ClearCharacterInfoStrings()
{
    xr_delete(charInfoReputationStrings);
    xr_delete(charInfoRankStrings);
    xr_delete(charInfoGoodwillStrings);
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetRankAsText(CHARACTER_RANK_VALUE rankID)
{
    InitCharacterInfoStrings();
    CharInfoStrings::const_iterator cit = charInfoRankStrings->upper_bound(rankID);
    if (charInfoRankStrings->end() == cit)
        return charInfoRankStrings->rbegin()->second.c_str();
    return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetReputationAsText(CHARACTER_REPUTATION_VALUE rankID)
{
    InitCharacterInfoStrings();

    CharInfoStrings::const_iterator cit = charInfoReputationStrings->upper_bound(rankID);
    if (charInfoReputationStrings->end() == cit)
        return charInfoReputationStrings->rbegin()->second.c_str();

    return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////

LPCSTR InventoryUtilities::GetGoodwillAsText(CHARACTER_GOODWILL goodwill)
{
    InitCharacterInfoStrings();

    CharInfoStrings::const_iterator cit = charInfoGoodwillStrings->upper_bound(goodwill);
    if (charInfoGoodwillStrings->end() == cit)
        return charInfoGoodwillStrings->rbegin()->second.c_str();

    return cit->second.c_str();
}

//////////////////////////////////////////////////////////////////////////
// специальная функция для передачи info_portions при нажатии кнопок UI
// (для tutorial)
void InventoryUtilities::SendInfoToActor(LPCSTR info_id)
{
    if (GameID() != eGameIDSingle)
        return;

    CActor* actor = smart_cast<CActor*>(Level().CurrentEntity());
    if (actor)
    {
        actor->TransferInfo(info_id, true);
    }
}

void InventoryUtilities::SendInfoToLuaScripts(shared_str info)
{
    if (GameID() != eGameIDSingle)
        return;
    if (info == shared_str("ui_talk_show"))
    {
        int mode = 10; // now Menu is Talk Dialog (show)
        luabind::functor<void> funct;
        if (GEnv.ScriptEngine->functor("pda.actor_menu_mode", funct))
            funct(mode);
    }
    if (info == shared_str("ui_talk_hide"))
    {
        int mode = 11; // Talk Dialog hide
        luabind::functor<void> funct;
        if (GEnv.ScriptEngine->functor("pda.actor_menu_mode", funct))
            funct(mode);
    }
}
// XXX: interpolate color (enemy..neutral..friend)<->(red..gray..lime)
u32 InventoryUtilities::GetGoodwillColor(CHARACTER_GOODWILL gw)
{
    u32 res = 0xffc0c0c0;
    if (gw == NEUTRAL_GOODWILL)
    {
        res = 0xffc0c0c0;
    }
    else if (gw > 1000)
    {
        res = 0xff00ff00;
    }
    else if (gw < -1000)
    {
        res = 0xffff0000;
    }
    return res;
}

u32 InventoryUtilities::GetReputationColor(CHARACTER_REPUTATION_VALUE rv)
{
    u32 res = 0xffc0c0c0;
    if (rv == NEUTAL_REPUTATION)
    {
        res = 0xffc0c0c0;
    }
    else if (rv > 50)
    {
        res = 0xff00ff00;
    }
    else if (rv < -50)
    {
        res = 0xffff0000;
    }
    return res;
}

u32 InventoryUtilities::GetRelationColor(ALife::ERelationType relation)
{
    switch (relation)
    {
    case ALife::eRelationTypeFriend: return 0xff00ff00; break;
    case ALife::eRelationTypeNeutral: return 0xffc0c0c0; break;
    case ALife::eRelationTypeEnemy: return 0xffff0000; break;
    default: NODEFAULT;
    }
#ifdef DEBUG
    return 0xffffffff;
#endif
}
