#pragma once
#include "UIDragDropListEx.h"

class CUIOutfitDragDropList : public CUIDragDropListEx
{
    using inherited = CUIDragDropListEx;
    CUIStatic* m_background;
    shared_str m_default_outfit;
    void SetOutfit(CUICellItem* itm);

public:
    CUIOutfitDragDropList();
    ~CUIOutfitDragDropList() override = default;

    void Draw() override;

    void SetItem(CUICellItem* itm) override; //auto
    void SetItem(CUICellItem* itm, Fvector2 abs_pos) override;  // start at cursor pos
    void SetItem(CUICellItem* itm, Ivector2 cell_pos) override; // start at cell

    CUICellItem* RemoveItem(CUICellItem* itm, bool force_root) override;

    void SetDefaultOutfit(pcstr default_outfit);
};
