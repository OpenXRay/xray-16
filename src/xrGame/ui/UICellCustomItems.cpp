#include "StdAfx.h"
#include "UICellCustomItems.h"
#include "UIInventoryUtilities.h"
#include "Weapon.h"
#include "UIDragDropListEx.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"

#define INV_GRID_WIDTHF 50.0f
#define INV_GRID_HEIGHTF 50.0f

namespace detail
{
struct is_helper_pred
{
    bool operator()(CUICellItem* child) { return child->IsHelper(); }
}; // struct is_helper_pred

} // namespace detail

CUIInventoryCellItem::CUIInventoryCellItem(CInventoryItem* itm)
{
    m_pData = (void*)itm;

    inherited::SetShader(InventoryUtilities::GetEquipmentIconsShader());

    m_grid_size.set(itm->GetInvGridRect().rb);
    Frect rect;
    rect.lt.set(INV_GRID_WIDTHF * itm->GetInvGridRect().x1, INV_GRID_HEIGHTF * itm->GetInvGridRect().y1);

    rect.rb.set(rect.lt.x + INV_GRID_WIDTHF * m_grid_size.x, rect.lt.y + INV_GRID_HEIGHTF * m_grid_size.y);

    inherited::SetTextureRect(rect);
    inherited::SetStretchTexture(true);
}

bool CUIInventoryCellItem::EqualTo(CUICellItem* itm)
{
    CUIInventoryCellItem* ci = smart_cast<CUIInventoryCellItem*>(itm);
    if (!itm)
    {
        return false;
    }
    if (object()->object().cNameSect() != ci->object()->object().cNameSect())
    {
        return false;
    }
    if (!fsimilar(object()->GetCondition(), ci->object()->GetCondition(), 0.01f))
    {
        return false;
    }
    if (!object()->equal_upgrades(ci->object()->upgardes()))
    {
        return false;
    }
    return true;
}

bool CUIInventoryCellItem::IsHelperOrHasHelperChild()
{
    return std::count_if(m_childs.begin(), m_childs.end(), detail::is_helper_pred()) > 0 || IsHelper();
}

CUIDragItem* CUIInventoryCellItem::CreateDragItem()
{
    return IsHelperOrHasHelperChild() ? NULL : inherited::CreateDragItem();
}

bool CUIInventoryCellItem::IsHelper() { return object()->is_helper_item(); }
void CUIInventoryCellItem::SetIsHelper(bool is_helper) { object()->set_is_helper(is_helper); }
void CUIInventoryCellItem::Update()
{
    inherited::Update();
    UpdateConditionProgressBar(); //Alundaio
    UpdateItemText();

    u32 color = GetTextureColor();
    if (IsHelper() && !ChildsCount())
    {
        color = 0xbbbbbbbb;
    }
    else if (IsHelperOrHasHelperChild())
    {
        color = 0xffffffff;
    }

    SetTextureColor(color);
}

void CUIInventoryCellItem::UpdateItemText()
{
    const u32 helper_count =
        (u32)std::count_if(m_childs.begin(), m_childs.end(), detail::is_helper_pred()) + IsHelper() ? 1 : 0;

    const u32 count = ChildsCount() + 1 - helper_count;

    string32 tempStr;
    pcstr finalText = nullptr;
    if (count > 1 || helper_count)
    {
        xr_sprintf(tempStr, "x%d", count);
        finalText = tempStr;
    }

    if (m_text)
    {
        m_text->Show(nullptr != finalText);
        m_text->SetText(finalText);
    }
    else
    {
        this->SetText(finalText);
    }
}

CUIAmmoCellItem::CUIAmmoCellItem(CWeaponAmmo* itm) : inherited(itm) {}
bool CUIAmmoCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIAmmoCellItem* ci = smart_cast<CUIAmmoCellItem*>(itm);
    if (!ci)
        return false;

    return ((object()->cNameSect() == ci->object()->cNameSect()));
}

CUIDragItem* CUIAmmoCellItem::CreateDragItem() { return IsHelper() ? NULL : inherited::CreateDragItem(); }
u32 CUIAmmoCellItem::CalculateAmmoCount()
{
    xr_vector<CUICellItem*>::iterator it = m_childs.begin();
    xr_vector<CUICellItem*>::iterator it_e = m_childs.end();

    u32 total = IsHelper() ? 0 : object()->m_boxCurr;
    for (; it != it_e; ++it)
    {
        CUICellItem* child = *it;

        if (!child->IsHelper())
        {
            total += ((CUIAmmoCellItem*)(*it))->object()->m_boxCurr;
        }
    }

    return total;
}

void CUIAmmoCellItem::UpdateItemText()
{
    string32 tempStr;
    pcstr finalText = nullptr;
    if (!m_custom_draw)
    {
        xr_sprintf(tempStr, "%d", CalculateAmmoCount());
        finalText = tempStr;
    }

    if (m_text)
    {
        m_text->Show(nullptr != finalText);
        m_text->SetText(finalText);
    }
    else
    {
        this->SetText(finalText);
    }
}

CUIWeaponCellItem::CUIWeaponCellItem(CWeapon* itm) : inherited(itm)
{
    m_addons[eSilencer] = NULL;
    m_addons[eScope] = NULL;
    m_addons[eLauncher] = NULL;

    if (itm->SilencerAttachable())
        m_addon_offset[eSilencer].set(object()->GetSilencerX(), object()->GetSilencerY());

    if (itm->ScopeAttachable())
        m_addon_offset[eScope].set(object()->GetScopeX(), object()->GetScopeY());

    if (itm->GrenadeLauncherAttachable())
        m_addon_offset[eLauncher].set(object()->GetGrenadeLauncherX(), object()->GetGrenadeLauncherY());
}

#include "Common/object_broker.h"
CUIWeaponCellItem::~CUIWeaponCellItem() {}
bool CUIWeaponCellItem::is_scope() { return object()->ScopeAttachable() && object()->IsScopeAttached(); }
bool CUIWeaponCellItem::is_silencer() { return object()->SilencerAttachable() && object()->IsSilencerAttached(); }
bool CUIWeaponCellItem::is_launcher()
{
    return object()->GrenadeLauncherAttachable() && object()->IsGrenadeLauncherAttached();
}

void CUIWeaponCellItem::CreateIcon(eAddonType t)
{
    if (m_addons[t])
        return;
    m_addons[t] = xr_new<CUIStatic>();
    m_addons[t]->SetAutoDelete(true);
    AttachChild(m_addons[t]);
    m_addons[t]->SetShader(InventoryUtilities::GetEquipmentIconsShader());

    u32 color = GetTextureColor();
    m_addons[t]->SetTextureColor(color);
}

void CUIWeaponCellItem::DestroyIcon(eAddonType t)
{
    DetachChild(m_addons[t]);
    m_addons[t] = NULL;
}

CUIStatic* CUIWeaponCellItem::GetIcon(eAddonType t) { return m_addons[t]; }
void CUIWeaponCellItem::RefreshOffset()
{
    if (object()->SilencerAttachable())
        m_addon_offset[eSilencer].set(object()->GetSilencerX(), object()->GetSilencerY());

    if (object()->ScopeAttachable())
        m_addon_offset[eScope].set(object()->GetScopeX(), object()->GetScopeY());

    if (object()->GrenadeLauncherAttachable())
        m_addon_offset[eLauncher].set(object()->GetGrenadeLauncherX(), object()->GetGrenadeLauncherY());
}

void CUIWeaponCellItem::Draw()
{
    inherited::Draw();

    if (m_upgrade && m_upgrade->IsShown())
        m_upgrade->Draw();
};

void CUIWeaponCellItem::Update()
{
    bool b = Heading();
    inherited::Update();

    bool bForceReInitAddons = (b != Heading());

    if (object()->SilencerAttachable())
    {
        if (object()->IsSilencerAttached())
        {
            if (!GetIcon(eSilencer) || bForceReInitAddons)
            {
                CreateIcon(eSilencer);
                RefreshOffset();
                InitAddon(GetIcon(eSilencer), *object()->GetSilencerName(), m_addon_offset[eSilencer], Heading());
            }
        }
        else
        {
            if (m_addons[eSilencer])
                DestroyIcon(eSilencer);
        }
    }

    if (object()->ScopeAttachable())
    {
        if (object()->IsScopeAttached())
        {
            if (!GetIcon(eScope) || bForceReInitAddons)
            {
                CreateIcon(eScope);
                RefreshOffset();
                InitAddon(GetIcon(eScope), *object()->GetScopeName(), m_addon_offset[eScope], Heading());
            }
        }
        else
        {
            if (m_addons[eScope])
                DestroyIcon(eScope);
        }
    }

    if (object()->GrenadeLauncherAttachable())
    {
        if (object()->IsGrenadeLauncherAttached())
        {
            if (!GetIcon(eLauncher) || bForceReInitAddons)
            {
                CreateIcon(eLauncher);
                RefreshOffset();
                InitAddon(
                    GetIcon(eLauncher), *object()->GetGrenadeLauncherName(), m_addon_offset[eLauncher], Heading());
            }
        }
        else
        {
            if (m_addons[eLauncher])
                DestroyIcon(eLauncher);
        }
    }
}

void CUIWeaponCellItem::SetTextureColor(u32 color)
{
    inherited::SetTextureColor(color);
    if (m_addons[eSilencer])
    {
        m_addons[eSilencer]->SetTextureColor(color);
    }
    if (m_addons[eScope])
    {
        m_addons[eScope]->SetTextureColor(color);
    }
    if (m_addons[eLauncher])
    {
        m_addons[eLauncher]->SetTextureColor(color);
    }
}

void CUIWeaponCellItem::OnAfterChild(CUIDragDropListEx* parent_list)
{
    if (is_silencer() && GetIcon(eSilencer))
        InitAddon(GetIcon(eSilencer), *object()->GetSilencerName(), m_addon_offset[eSilencer],
            parent_list->GetVerticalPlacement());

    if (is_scope() && GetIcon(eScope))
        InitAddon(
            GetIcon(eScope), *object()->GetScopeName(), m_addon_offset[eScope], parent_list->GetVerticalPlacement());

    if (is_launcher() && GetIcon(eLauncher))
        InitAddon(GetIcon(eLauncher), *object()->GetGrenadeLauncherName(), m_addon_offset[eLauncher],
            parent_list->GetVerticalPlacement());
}

void CUIWeaponCellItem::InitAddon(CUIStatic* s, LPCSTR section, Fvector2 addon_offset, bool b_rotate)
{
    Frect tex_rect;
    Fvector2 base_scale;

    if (Heading())
    {
        base_scale.x = GetHeight() / (INV_GRID_WIDTHF * m_grid_size.x);
        base_scale.y = GetWidth() / (INV_GRID_HEIGHTF * m_grid_size.y);
    }
    else
    {
        base_scale.x = GetWidth() / (INV_GRID_WIDTHF * m_grid_size.x);
        base_scale.y = GetHeight() / (INV_GRID_HEIGHTF * m_grid_size.y);
    }
    Fvector2 cell_size;
    cell_size.x = pSettings->r_u32(section, "inv_grid_width") * INV_GRID_WIDTHF;
    cell_size.y = pSettings->r_u32(section, "inv_grid_height") * INV_GRID_HEIGHTF;

    tex_rect.x1 = pSettings->r_u32(section, "inv_grid_x") * INV_GRID_WIDTHF;
    tex_rect.y1 = pSettings->r_u32(section, "inv_grid_y") * INV_GRID_HEIGHTF;

    tex_rect.rb.add(tex_rect.lt, cell_size);

    cell_size.mul(base_scale);

    if (b_rotate)
    {
        s->SetWndSize(Fvector2().set(cell_size.y, cell_size.x));
        Fvector2 new_offset;
        new_offset.x = addon_offset.y * base_scale.x;
        new_offset.y = GetHeight() - addon_offset.x * base_scale.x - cell_size.x;
        addon_offset = new_offset;
        addon_offset.x *= UI().get_current_kx();
    }
    else
    {
        s->SetWndSize(cell_size);
        addon_offset.mul(base_scale);
    }

    s->SetWndPos(addon_offset);
    s->SetTextureRect(tex_rect);
    s->SetStretchTexture(true);

    s->EnableHeading(b_rotate);

    if (b_rotate)
    {
        s->SetHeading(GetHeading());
        Fvector2 offs;
        offs.set(0.0f, s->GetWndSize().y);
        s->SetHeadingPivot(Fvector2().set(0.0f, 0.0f), /*Fvector2().set(0.0f,0.0f)*/ offs, true);
    }
}

CUIDragItem* CUIWeaponCellItem::CreateDragItem()
{
    CUIDragItem* i = inherited::CreateDragItem();
    CUIStatic* s = NULL;

    if (GetIcon(eSilencer))
    {
        s = xr_new<CUIStatic>();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader());
        InitAddon(s, *object()->GetSilencerName(), m_addon_offset[eSilencer], false);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eScope))
    {
        s = xr_new<CUIStatic>();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader());
        InitAddon(s, *object()->GetScopeName(), m_addon_offset[eScope], false);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }

    if (GetIcon(eLauncher))
    {
        s = xr_new<CUIStatic>();
        s->SetAutoDelete(true);
        s->SetShader(InventoryUtilities::GetEquipmentIconsShader());
        InitAddon(s, *object()->GetGrenadeLauncherName(), m_addon_offset[eLauncher], false);
        s->SetTextureColor(i->wnd()->GetTextureColor());
        i->wnd()->AttachChild(s);
    }
    return i;
}

bool CUIWeaponCellItem::EqualTo(CUICellItem* itm)
{
    if (!inherited::EqualTo(itm))
        return false;

    CUIWeaponCellItem* ci = smart_cast<CUIWeaponCellItem*>(itm);
    if (!ci)
        return false;

    //	bool b_addons					= ( (object()->GetAddonsState() == ci->object()->GetAddonsState()) );
    if (object()->GetAddonsState() != ci->object()->GetAddonsState())
    {
        return false;
    }
    if (this->is_scope() && ci->is_scope())
    {
        if (object()->GetScopeName() != ci->object()->GetScopeName())
        {
            return false;
        }
    }
    //	bool b_place					= ( (object()->m_eItemCurrPlace == ci->object()->m_eItemCurrPlace) );

    return true;
}

CBuyItemCustomDrawCell::CBuyItemCustomDrawCell(LPCSTR str, CGameFont* pFont)
{
    m_pFont = pFont;
    VERIFY(xr_strlen(str) < 16);
    xr_strcpy(m_string, str);
}

void CBuyItemCustomDrawCell::OnDraw(CUICellItem* cell)
{
    Fvector2 pos;
    cell->GetAbsolutePos(pos);
    UI().ClientToScreenScaled(pos, pos.x, pos.y);
    m_pFont->Out(pos.x, pos.y, m_string);
    m_pFont->OnRender();
}
