#include "stdafx.h"
#include "UIOutfitSlot.h"
#include "UICellItem.h"
#include "CustomOutfit.h"
#include "Actor.h"
#include "Level.h"
//#include "UIInventoryUtilities.h"

CUIOutfitDragDropList::CUIOutfitDragDropList()
{
    m_background = new CUIStatic();
    m_background->SetAutoDelete(true);
    CUIWindow::AttachChild(m_background);
    m_default_outfit = "npc_icon_without_outfit";
}

void CUIOutfitDragDropList::SetOutfit(CUICellItem* itm)
{
    /*
    static Fvector2 fNoOutfit			= pSettings->r_fvector2(m_default_outfit, "full_scale_icon");
    Frect								r;
    r.x1								= fNoOutfit.x*ICON_GRID_WIDTH;
    r.y1								= fNoOutfit.y*ICON_GRID_HEIGHT;
    r.x2								= r.x1+CHAR_ICON_FULL_WIDTH*ICON_GRID_WIDTH;
    r.y2								= r.y1+CHAR_ICON_FULL_HEIGHT*ICON_GRID_HEIGHT;
    */
    m_background->SetWndPos({ 0, 0 });
    m_background->SetWndSize({ GetWidth(), GetHeight() });

    m_background->SetStretchTexture(true);


    if (IsGameTypeSingle() && !itm)
    {
        IGameObject* pActor = smart_cast<CActor*>(Level().CurrentEntity());

        xr_string iconName;
        if (pActor)
            iconName = *pActor->cNameVisual();
        else
            iconName = *m_default_outfit;

        xr_string::iterator it = std::find(iconName.rbegin(), iconName.rend(), '\\').base();

        // Cut leading full path
        if (it != iconName.begin())
            iconName.erase(iconName.begin(), it);
        // Cut trailing ".ogf"
        R_ASSERT(xr_strlen(iconName.c_str()) > 4);
        if ('.' == iconName[iconName.size() - 4])
            iconName.erase(iconName.size() - 4);

        m_background->InitTexture(iconName.c_str());
    }
    else
    {
        if (itm)
        {
            PIItem _iitem = (PIItem)itm->m_pData;
            CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>(_iitem);
            VERIFY(pOutfit);
            /*
            r.lt			= pOutfit->GetIconPos();
            r.x1			*= ICON_GRID_WIDTH;
            r.y1			*= ICON_GRID_HEIGHT;
            */
            m_background->InitTexture(pOutfit->GetFullIconName().c_str());
        }
        else
        {
            m_background->InitTexture("npc_icon_without_outfit");
        }
        /*
        r.x2			= r.x1+CHAR_ICON_FULL_WIDTH*ICON_GRID_WIDTH;
        r.y2			= r.y1+CHAR_ICON_FULL_HEIGHT*ICON_GRID_HEIGHT;

        m_background->SetShader				(InventoryUtilities::GetCharIconsShader());
        m_background->SetOriginalRect		(r);
        */
    }

    m_background->TextureOn();
    //	m_background->RescaleRelative2Rect	(r);
}

void CUIOutfitDragDropList::SetDefaultOutfit(pcstr default_outfit)
{
    m_default_outfit = default_outfit;
}

void CUIOutfitDragDropList::SetItem(CUICellItem* itm)
{
    if (itm)
        inherited::SetItem(itm);
    SetOutfit(itm);
}

void CUIOutfitDragDropList::SetItem(CUICellItem* itm, Fvector2 abs_pos)
{
    if (itm)
        inherited::SetItem(itm, abs_pos);
    SetOutfit(itm);
}

void CUIOutfitDragDropList::SetItem(CUICellItem* itm, Ivector2 cell_pos)
{
    if (itm)
        inherited::SetItem(itm, cell_pos);
    SetOutfit(itm);
}

CUICellItem* CUIOutfitDragDropList::RemoveItem(CUICellItem* itm, bool force_root)
{
    VERIFY(!force_root);
    CUICellItem* ci = inherited::RemoveItem(itm, force_root);
    SetOutfit(nullptr);
    return ci;
}

void CUIOutfitDragDropList::Draw()
{
    m_background->Draw();
    // inherited::Draw();
}
