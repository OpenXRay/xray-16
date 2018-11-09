#pragma once

#include "Common/object_interfaces.h"
#include "inventory_space.h"
#include "gametype_chooser.h"
#include "UIDialogHolder.h"
#include "xrEngine/CustomHUD.h"
#include "script_game_object.h"
#include "xrCommon/xr_string.h"

// fwd. decl.
class CUI;
class CTeamBaseZone;
class game_cl_GameState;
class CUIDialogWnd;
class CUICaption;
class CUIStatic;
class CUIWindow;
class CUIXml;
class CUIActorMenu;
class CUIPdaWnd;
struct KillMessageStruct;
class CUIMainIngameWnd;
class CUIMessagesWindow;

struct StaticDrawableWrapper : public IPureDestroyableObject
{
    CUIStatic* m_static;
    float m_endTime;
    shared_str m_name;

    StaticDrawableWrapper();
    virtual void destroy();
    void Draw();
    void Update();
    CUIStatic* wnd() { return m_static; }
    bool IsActual() const;
    void SetText(const char* text);
};

struct MPLevelDesc
{
    shared_str map_name;
    shared_str map_ver;
    bool operator==(const MPLevelDesc& rhs) { return map_name == rhs.map_name && map_ver == rhs.map_ver; }
};

struct SGameTypeMaps
{
    shared_str m_game_type_name;
    EGameIDs m_game_type_id;
    xr_vector<MPLevelDesc> m_map_names;
};

struct MPWeatherDesc
{
    shared_str Name;
    shared_str StartTime;
};

class CMapListHelper
{
private:
    xr_vector<SGameTypeMaps> m_storage;
    xr_vector<MPWeatherDesc> m_weathers;

public:
    const SGameTypeMaps& GetMapListFor(const shared_str& gameType);
    const SGameTypeMaps& GetMapListFor(const EGameIDs gameId);
    const xr_vector<MPWeatherDesc>& GetGameWeathers();

private:
    void Load();
    void LoadMapInfo(const char* cfgName, const xr_string& levelName, const char* levelVer = "1.0");
    SGameTypeMaps* GetMapListInt(const shared_str& gameType);
};

extern CMapListHelper gMapListHelper;

class CUIGameCustom : public FactoryObjectBase, public CDialogHolder, public CUIResetNotifier
{
protected:
    CUIWindow* Window;
    CUIXml* MsgConfig;
    xr_vector<StaticDrawableWrapper*> CustomStatics;
    CUIActorMenu* ActorMenu;
    CUIPdaWnd* PdaMenu;
    bool showGameIndicators;

public:
    // XXX nitrocaster: make not public
    CUIMainIngameWnd* UIMainIngameWnd;
    CUIMessagesWindow* m_pMessagesWnd;

    CUIGameCustom();
    virtual ~CUIGameCustom();
    virtual void SetClGame(game_cl_GameState* gameState);
    virtual void OnInventoryAction(PIItem item, u16 actionType);
    virtual void Init(int stage) {}
    virtual void Render();
    virtual void OnFrame() override;
    IC CUIActorMenu& GetActorMenu() const { return *ActorMenu; }
    IC CUIPdaWnd& GetPdaMenu() const { return *PdaMenu; }
    bool ShowActorMenu();
    void HideActorMenu();
    void UpdateActorMenu(); //Alundaio
    CScriptGameObject* CurrentItemAtCell(); //Alundaio
    bool ShowPdaMenu();
    void HidePdaMenu();
    void ShowMessagesWindow();
    void HideMessagesWindow();
    void ShowGameIndicators(bool show) { showGameIndicators = show; }
    bool GameIndicatorsShown() { return showGameIndicators; }
    void ShowCrosshair(bool show) { psHUD_Flags.set(HUD_CROSSHAIR_RT, show); }
    bool CrosshairShown() { return !!psHUD_Flags.test(HUD_CROSSHAIR_RT); }
    virtual void HideShownDialogs() {}
    virtual void ReinitDialogs() {}
    StaticDrawableWrapper* AddCustomStatic(const char* id, bool singleInstance);
    StaticDrawableWrapper* GetCustomStatic(const char* id);
    void RemoveCustomStatic(const char* id);
    void CommonMessageOut(const char* text);
    virtual void ChangeTotalMoneyIndicator(const char* newMoneyString) {}
    virtual void DisplayMoneyChange(const char* deltaMoney) {}
    virtual void DisplayMoneyBonus(KillMessageStruct* bonus) {}
    virtual void UnLoad();
    void Load();
    void OnConnected();
    void UpdatePda();
    void update_fake_indicators(u8 type, float power);
    void enable_fake_indicators(bool enable);
};

extern CUIGameCustom* CurrentGameUI();
