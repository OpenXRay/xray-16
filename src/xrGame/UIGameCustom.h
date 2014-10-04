#pragma once

#include "script_export_space.h"
#include "object_interfaces.h"
#include "inventory_space.h"
#include "gametype_chooser.h"
#include "UIDialogHolder.h"
#include "xrEngine/CustomHUD.h"
// refs
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

struct SDrawStaticStruct : public IPureDestroyableObject
{
    SDrawStaticStruct();
    virtual	void destroy();
    CUIStatic* m_static;
    float m_endTime;
    shared_str m_name;
    void Draw();
    void Update();
    CUIStatic* wnd() { return m_static; }
    bool IsActual()	const;
    void SetText(const char* text);
};

struct SGameTypeMaps
{
    struct SMapItm
    {
        shared_str map_name;
        shared_str map_ver;
        bool operator == (const SMapItm& rhs) { return map_name == rhs.map_name && map_ver == rhs.map_ver; }
    };

    shared_str m_game_type_name;
    EGameIDs m_game_type_id;
    xr_vector<SMapItm> m_map_names;
};

struct SGameWeathers
{
    shared_str Name;
    shared_str StartTime;
};

class CMapListHelper
{
private:
    xr_vector<SGameTypeMaps> m_storage;
    xr_vector<SGameWeathers> m_weathers;

public:
    const SGameTypeMaps& GetMapListFor(const shared_str& gameType);
    const SGameTypeMaps& GetMapListFor(const EGameIDs gameId);
    const xr_vector<SGameWeathers>& GetGameWeathers();

private:
    void Load();
    void LoadMapInfo(const char* cfgName, const xr_string& levelName, const char* levelVer = "1.0");
    SGameTypeMaps* GetMapListInt(const shared_str& gameType);
};

extern CMapListHelper gMapListHelper;

class CUIGameCustom :
    public DLL_Pure,
    public CDialogHolder
{
protected:
    CUIWindow* Window;
    CUIXml* MsgConfig;
    xr_vector<SDrawStaticStruct*> CustomStatics;
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
    virtual	void Init(int stage) {}
    virtual void Render();
    virtual void _BCL OnFrame() override;
    IC CUIActorMenu& GetActorMenu() const { return *ActorMenu; }
    IC CUIPdaWnd& GetPdaMenu() const { return *PdaMenu; }
    bool ShowActorMenu();
    void HideActorMenu();
    bool ShowPdaMenu();
    void HidePdaMenu();
    void ShowMessagesWindow();
    void HideMessagesWindow();
    void ShowGameIndicators(bool show) { showGameIndicators = show; }
    bool GameIndicatorsShown() { return showGameIndicators; }
    void ShowCrosshair(bool show) { psHUD_Flags.set(HUD_CROSSHAIR_RT, show); }
    bool CrosshairShown() { return !!psHUD_Flags.test(HUD_CROSSHAIR_RT); }    
    virtual void HideShownDialogs() {}
    SDrawStaticStruct* AddCustomStatic(const char* id, bool singleInstance);
    SDrawStaticStruct* GetCustomStatic(const char* id);
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
    DECLARE_SCRIPT_REGISTER_FUNCTION
};

extern CUIGameCustom* CurrentGameUI();
