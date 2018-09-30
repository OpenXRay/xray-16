////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInventoryUpgradeWnd.h
//	Created 	: 06.10.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade UI window class
////////////////////////////////////////////////////////////////////////////

#ifndef UI_INVENTORY_UPGRADE_WND_H_INCLUDED
#define UI_INVENTORY_UPGRADE_WND_H_INCLUDED

#include "xrUICore/Static/UIStatic.h"
#include "UIInvUpgrade.h"

extern const LPCSTR g_inventory_upgrade_xml;
#define MAX_UI_UPGRADE_CELLS 25

namespace inventory
{
namespace upgrade
{
class Manager;
class Upgrade;
class Property;
}
} // namespace upgrade, inventory

class UIUpgrade;
class CInventoryItem;
class CUIItemInfo;
class CUIFrameLineWnd;
class CUI3tButton;

class CUIInventoryUpgradeWnd : public CUIWindow
{
private:
    typedef CUIWindow inherited;

    typedef inventory::upgrade::Manager Manager_type;
    typedef inventory::upgrade::Upgrade Upgrade_type;
    typedef inventory::upgrade::Property Property_type;
    typedef xr_vector<UIUpgrade*> UI_Upgrades_type;

private: // sub-classes
    struct Scheme
    {
        shared_str name;
        UI_Upgrades_type cells;

        Scheme();
        virtual ~Scheme();
    };
    typedef xr_vector<Scheme*> SCHEMES;

public:
    CUIInventoryUpgradeWnd();
    virtual ~CUIInventoryUpgradeWnd();

    virtual void Init();
    void InitInventory(CInventoryItem* item, bool can_upgrade);

    IC CInventoryItem const* get_inventory() const { return m_inv_item; }
    IC LPCSTR get_cell_texture(UIUpgrade::ViewState state) const { return m_cell_textures[state].c_str(); }
    IC LPCSTR get_point_texture(UIUpgrade::ViewState state) const { return m_point_textures[state].c_str(); }
    Fvector2 get_scheme_position() const { return m_scheme_wnd->GetWndPos(); }
    Fvector2 get_item_position() const { return m_item->GetWndPos(); }
    virtual void Show(bool status);
    virtual void Update();
    virtual void Reset();
    void UpdateAllUpgrades();

    bool DBClickOnUIUpgrade(Upgrade_type const* upgr);
    void AskUsing(LPCSTR text, LPCSTR upgrade_name);
    void OnMesBoxYes();

    void HighlightHierarchy(shared_str const& upgrade_id);
    void ResetHighlight();
    void set_info_cur_upgrade(Upgrade_type* upgrade);
    UIUpgrade* FindUIUpgrade(Upgrade_type const* upgr);

private:
    void LoadCellsBacks(CUIXml& uiXml);
    void LoadCellStates(LPCSTR state_str, LPCSTR texture_name, LPCSTR texture_name2, u32 color);
    UIUpgrade::ViewState SelectCellState(LPCSTR state_str);
    void SetCellState(UIUpgrade::ViewState state, LPCSTR texture_name, LPCSTR texture_name2, u32 color);
    bool VerirfyCells();

    void LoadSchemes(CUIXml& uiXml);
    void SetCurScheme(const shared_str& id);
    bool install_item(CInventoryItem& inv_item, bool can_upgrade);
    Manager_type& get_manager();

public:
    CUI3tButton* m_btn_repair;

protected:
    CUIStatic* m_background;
    CUIStatic* m_item;
    CUIWindow* m_back;
    CInventoryItem* m_inv_item;

    shared_str m_cell_textures[UIUpgrade::STATE_COUNT];
    shared_str m_point_textures[UIUpgrade::STATE_COUNT];

    SCHEMES m_schemes;
    Scheme* m_current_scheme;
    LPCSTR m_cur_upgrade_id;
    CUIWindow* m_scheme_wnd;

public:
    ui_shader* m_WeaponIconsShader;
    ui_shader* m_OutfitIconsShader;

}; // class CUIInventoryUpgradeWnd

#endif // UI_INVENTORY_UPGRADE_WND_H_INCLUDED
