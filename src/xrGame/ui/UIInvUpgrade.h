////////////////////////////////////////////////////////////////////////////
//	Module 		: UIInvUpgrade.h
//	Created 	: 08.11.2007
//  Modified 	: 13.03.2009
//	Author		: Evgeniy Sokolov, Prishchepa Sergey
//	Description : inventory upgrade UI class
////////////////////////////////////////////////////////////////////////////

#ifndef UI_INVENTORY_UPGRADE_H_INCLUDED
#define UI_INVENTORY_UPGRADE_H_INCLUDED

#include "xrUICore/Static/UIStatic.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"

namespace inventory
{
namespace upgrade
{
class Upgrade;
class Property;
}
} // namespace upgrade, inventory

class CUIStatic;
class CUIInventoryUpgradeWnd;
class CInventoryItem;
class CUIUpgradePoint;

class UIUpgrade : public CUIWindow
{
private:
    typedef inventory::upgrade::Upgrade Upgrade_type;
    typedef inventory::upgrade::Property Property_type;
    typedef CUIWindow inherited;

public:
    enum ButtonState
    {
        BUTTON_FREE = 0,
        BUTTON_PRESSED,
        BUTTON_DPRESSED,
        BUTTON_FOCUSED
    };

public:
    enum ViewState
    {
        STATE_ENABLED = 0,
        STATE_FOCUSED,
        STATE_TOUCHED,
        STATE_SELECTED,
        STATE_UNKNOWN,

        STATE_DISABLED_PARENT,
        STATE_DISABLED_GROUP,
        STATE_DISABLED_PREC_MONEY,
        STATE_DISABLED_PREC_QUEST,
        STATE_DISABLED_FOCUSED,

        STATE_COUNT
    };

    enum Layer
    {
        LAYER_ITEM = 0,
        LAYER_COLOR,
        LAYER_POINT,
        LAYER_COUNT
    };

public:
    Fvector2 offset;

private:
    CUIInventoryUpgradeWnd* m_parent_wnd;

    CUIStatic* m_item;
    CUIStatic* m_color;
    shared_str m_upgrade_id;

protected:
    Ivector2 m_scheme_index;

    ButtonState m_button_state;

    ViewState m_state;
    ViewState m_prev_state;

    bool m_state_lock;

public:
    UIUpgrade(CUIInventoryUpgradeWnd* parent_wnd);
    virtual ~UIUpgrade();

    void init_upgrade(LPCSTR upgrade_id, CInventoryItem& item);

    void load_from_xml(CUIXml& ui_xml, int i_column, int i_cell, Frect const& t_cell_item);
    void set_texture(Layer layer, LPCSTR texture);

    virtual void Draw();
    virtual void Update();
    virtual void Reset();

    void update_upgrade_state();
    void update_mask();
    void update_item(CInventoryItem* inv_item);

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void OnFocusReceive();
    virtual void OnFocusLost();
    virtual void OnClick();
    virtual bool OnDbClick();
    void OnRClick();

    void on_over_window();

    void highlight_relation(bool enable);

    IC ButtonState get_button_state() const { return m_button_state; }
    void set_button_state(ButtonState state) { m_button_state = state; }
    IC Ivector2 const& get_scheme_index() const { return m_scheme_index; }
    Upgrade_type* get_upgrade();
    CUIInventoryUpgradeWnd* get_upgrade_window() { return m_parent_wnd; }
    void attach_point(CUIUpgradePoint* point);

public:
    CUIUpgradePoint* m_point;
};

class CUIUpgradePoint : public CUIStatic
{
private:
    typedef CUIStatic inherited;
    UIUpgrade* m_parent_upgrade;

public:
    CUIUpgradePoint(UIUpgrade* upgr);
    virtual ~CUIUpgradePoint();
    void load_from_xml(CUIXml& ui_xml, int i_cell);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void OnFocusReceive();
    virtual void OnFocusLost();
};

#endif // UI_INVENTORY_UPGRADE_H_INCLUDED
