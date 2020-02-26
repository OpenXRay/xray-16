#include "StdAfx.h"
#include "UIDragDropReferenceList.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "xrUICore/Static/UIStatic.h"
#include "Inventory.h"
#include "InventoryOwner.h"
#include "Actor.h"
#include "actor_defs.h"
#include "UIInventoryUtilities.h"
#include "xrEngine/xr_input.h"
#include "xrUICore/Cursor/UICursor.h"
#include "UICellItemFactory.h"
#include "UIHelper.h"

CUIDragDropReferenceList::CUIDragDropReferenceList()
{
    AddCallbackStr("cell_item_reference", WINDOW_LBUTTON_DB_CLICK,
        CUIWndCallback::void_function(this, &CUIDragDropReferenceList::OnItemDBClick));
}
CUIDragDropReferenceList::~CUIDragDropReferenceList() {}
void CUIDragDropReferenceList::Initialize(pcstr labelSection /*= nullptr*/, pcstr translationId /*= nullptr*/, CUIXml* uiXml /*= nullptr*/)
{
    m_translation_id = translationId;
    R_ASSERT((labelSection && uiXml && translationId) || (!labelSection && !uiXml && !translationId)); // all or nothing

    Fvector2 listAbsPos;
    GetAbsolutePos(listAbsPos);
    const Ivector2& cellSize = m_container->CellSize();
    const Ivector2& cellSpacing = m_container->CellsSpacing();
    const Ivector2& cellsCapacity = m_container->CellsCapacity();

    m_references.reserve(cellsCapacity.x * cellsCapacity.y);
    for (int i = 0; i < cellsCapacity.x; i++)
    {
        for (int j = 0; j < cellsCapacity.y; j++)
        {
            CUIStatic* reference = m_references.emplace_back(new CUIStatic());

            const Fvector2 pos = Fvector2().set((cellSize.x + cellSpacing.x) * i, (cellSize.y + cellSpacing.y) * j);
            const Fvector2 size = Fvector2().set(cellSize.x, cellSize.y);
            reference->SetWndPos(pos);
            reference->SetWndSize(size);

            reference->SetWindowName("cell_item_reference");
            reference->SetAutoDelete(true);
            AttachChild(reference);
            Register(reference);

            if (labelSection)
            {
                string32 temp;
                xr_sprintf(temp, labelSection, i + j + 1);
                CUITextWnd* label = UIHelper::CreateTextWnd(*uiXml, temp, this, false);
                if (label)
                {
                    if (!label->WndPosIsProbablyRelative()) // Without this, UI Frustum will cull our label
                    {
                        const Fvector2& lblPos = label->GetWndPos();
                        label->SetWndPos({ lblPos.x - listAbsPos.x, lblPos.y - listAbsPos.y });
                    }
                    m_labels.emplace_back(label);
                }
            }
        }
    }
}
void CUIDragDropReferenceList::SetItem(CUICellItem* itm) { inherited::SetItem(itm); }
void CUIDragDropReferenceList::SetItem(CUICellItem* itm, Fvector2 abs_pos)
{
    const Ivector2 dest_cell_pos = m_container->PickCell(abs_pos);
    if (m_container->ValidCell(dest_cell_pos) && m_container->IsRoomFree(dest_cell_pos, itm->GetGridSize()))
        SetItem(itm, dest_cell_pos);
    else
    {
        if (dest_cell_pos.x != -1 && dest_cell_pos.y != -1)
        {
            CUICellItem* old_itm = GetCellAt(dest_cell_pos).m_item;
            RemoveItem(old_itm, false);
            SetItem(itm, dest_cell_pos);
        }
    }
}
void CUIDragDropReferenceList::SetItem(CUICellItem* itm, Ivector2 cell_pos)
{
    CUIStatic* ref = m_references[m_container->CellsCapacity().x * cell_pos.y + cell_pos.x];
    ref->SetShader(itm->GetShader());
    ref->SetTextureRect(itm->GetTextureRect());
    ref->TextureOn();
    ref->SetTextureColor(color_rgba(255, 255, 255, 255));
    ref->SetStretchTexture(true);

    CUICell& C = m_container->GetCellAt(cell_pos);
    if (C.m_item != itm)
    {
        m_container->PlaceItemAtPos(itm, cell_pos);
        itm->SetWindowName("cell_item");
        Register(itm);
        itm->SetOwnerList(this);
    }
}
CUICellItem* CUIDragDropReferenceList::RemoveItem(CUICellItem* itm, bool force_root)
{
    const Ivector2 vec2 = m_container->GetItemPos(itm);
    if (vec2.x != -1 && vec2.y != -1)
    {
        u8 index = u8(m_container->CellsCapacity().x * vec2.y + vec2.x);
        xr_strcpy(ACTOR_DEFS::g_quick_use_slots[index], "");
        m_references[index]->SetTextureColor(color_rgba(255, 255, 255, 0));
    }
    inherited::RemoveItem(itm, force_root);
    return NULL;
}

void CUIDragDropReferenceList::LoadItemTexture(LPCSTR section, Ivector2 cell_pos)
{
    CUIStatic* ref = m_references[m_container->CellsCapacity().x * cell_pos.y + cell_pos.x];
    ref->SetShader(InventoryUtilities::GetEquipmentIconsShader());
    Frect texture_rect;
    texture_rect.x1 = pSettings->r_float(section, "inv_grid_x") * INV_GRID_WIDTH;
    texture_rect.y1 = pSettings->r_float(section, "inv_grid_y") * INV_GRID_HEIGHT;
    texture_rect.x2 = pSettings->r_float(section, "inv_grid_width") * INV_GRID_WIDTH;
    texture_rect.y2 = pSettings->r_float(section, "inv_grid_height") * INV_GRID_HEIGHT;
    texture_rect.rb.add(texture_rect.lt);
    ref->SetTextureRect(texture_rect);
    ref->TextureOn();
    ref->SetTextureColor(color_rgba(255, 255, 255, 255));
    ref->SetStretchTexture(true);
}

void CUIDragDropReferenceList::ReloadReferences(CInventoryOwner* pActor)
{
    if (!pActor)
        return;

    if (m_drag_item)
        DestroyDragItem();

    m_container->ClearAll(true);
    m_selected_item = NULL;

    const Ivector2& cellsCapacity = m_container->CellsCapacity();
    for (int i = 0; i < cellsCapacity.x; i++)
    {
        for (int j = 0; j < cellsCapacity.y; j++)
        {
            CUIStatic* ref = m_references[cellsCapacity.x * j + i];
            LPCSTR item_name = ACTOR_DEFS::g_quick_use_slots[cellsCapacity.x * j + i];
            if (item_name && xr_strlen(item_name))
            {
                PIItem itm = pActor->inventory().GetAny(item_name);
                if (itm)
                {
                    SetItem(create_cell_item(itm), Ivector2().set(i, j));
                }
                else
                {
                    LoadItemTexture(item_name, Ivector2().set(i, j));
                    ref->SetTextureColor(color_rgba(255, 255, 255, 100));
                }
            }
            else
            {
                ref->SetTextureColor(color_rgba(255, 255, 255, 0));
            }
        }
    }
}

void CUIDragDropReferenceList::UpdateLabels()
{
    for (size_t i = 0; i < m_labels.size(); i++)
    {
        string32 tmp;
        xr_sprintf(tmp, m_translation_id, i + 1);
        pcstr str = StringTable().translate(tmp).c_str();
        strncpy_s(tmp, sizeof(tmp), str, 3);
        if (tmp[2] == ',')
            tmp[1] = '\0';
        m_labels[i]->SetTextST(tmp);
    }
}

void CUIDragDropReferenceList::OnItemDBClick(CUIWindow* w, void* pData)
{
    CUIStatic* ref = smart_cast<CUIStatic*>(w);
    ITEMS_REFERENCES_VEC_IT it = std::find(m_references.begin(), m_references.end(), ref);
    if (it != m_references.end())
    {
        u8 index = u8(it - m_references.begin());
        CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
        if (actor)
        {
            PIItem itm = actor->inventory().GetAny(ACTOR_DEFS::g_quick_use_slots[index]);
            if (itm)
                inherited::RemoveItem(GetCellIdx(index).m_item, false);
        }
        xr_strcpy(ACTOR_DEFS::g_quick_use_slots[index], "");
        (*it)->SetTextureColor(color_rgba(255, 255, 255, 0));
    }
}

void CUIDragDropReferenceList::OnItemDrop(CUIWindow* w, void* pData)
{
    OnItemSelected(w, pData);
    CUICellItem* itm = smart_cast<CUICellItem*>(w);
    VERIFY(itm->OwnerList() == itm->OwnerList());

    if (m_f_item_drop && m_f_item_drop(itm))
    {
        DestroyDragItem();
        return;
    }

    CUIDragDropListEx* old_owner = itm->OwnerList();
    CUIDragDropListEx* new_owner = m_drag_item->BackList();
    if (old_owner && new_owner && old_owner != new_owner)
    {
        inherited::OnItemDrop(w, pData);
        return;
    }

    CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
    if (actor)
    {
        const Ivector2 vec = PickCell(GetUICursor().GetCursorPosition());
        if (vec.x != -1 && vec.y != -1)
        {
            const Ivector2 vec2 = m_container->GetItemPos(itm);
            if (vec2.x != -1 && vec2.y != -1)
            {
                const int capX = m_container->CellsCapacity().x;

                const u8 index = u8(capX * vec.y + vec.x);
                const u8 index2 = u8(capX * vec2.y + vec2.x);
                shared_str tmp = ACTOR_DEFS::g_quick_use_slots[index];
                xr_strcpy(ACTOR_DEFS::g_quick_use_slots[index], ACTOR_DEFS::g_quick_use_slots[index2]);
                xr_strcpy(ACTOR_DEFS::g_quick_use_slots[index2], tmp.c_str());
                ReloadReferences(actor);
                return;
            }
        }
    }
    DestroyDragItem();
}
