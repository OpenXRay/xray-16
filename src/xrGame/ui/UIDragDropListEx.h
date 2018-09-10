#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

class CUICellContainer;
class CUIScrollBar;
class CUIStatic;
class CUICellItem;
class CUIDragItem;

struct CUICell
{
    CUICell()
    {
        m_item = NULL;
        Clear();
    }

    CUICellItem* m_item;
    bool m_bMainItem;

    void SetItem(CUICellItem* itm, bool bMain)
    {
        m_item = itm;
        VERIFY(m_item);
        m_bMainItem = bMain;
    }
    bool Empty() { return m_item == NULL; }
    bool MainItem() { return m_bMainItem; }
    void Clear();
    bool operator==(const CUICell& C) const { return (m_item == C.m_item); }
};

typedef xr_vector<CUICell> UI_CELLS_VEC;
typedef UI_CELLS_VEC::iterator UI_CELLS_VEC_IT;

class CUIDragDropListEx : public CUIWindow, public CUIWndCallback
{
private:
    typedef CUIWindow inherited;

    enum
    {
        flGroupSimilar = (1 << 0),
        flAutoGrow = (1 << 1),
        flCustomPlacement = (1 << 2),
        flVerticalPlacement = (1 << 3),
        flAlwaysShowScroll = (1 << 4),
        flVirtualCells = (1 << 5),
    };
    Flags8 m_flags;
    Ivector2 m_orig_cell_capacity;
    Ivector2 m_virtual_cells_alignment;
    bool m_bConditionProgBarVisible;

protected:
    CUICellItem* m_selected_item;
    CUICellContainer* m_container;
    CUIScrollBar* m_vScrollBar;

    virtual void __stdcall OnScrollV(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemStartDragging(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemDrop(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemSelected(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemLButtonClick(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemRButtonClick(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemDBClick(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemFocusReceived(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemFocusLost(CUIWindow* w, void* pData);
    virtual void __stdcall OnItemFocusedUpdate(CUIWindow* w, void* pData);

public:
    static CUIDragItem* m_drag_item;
    CUIDragDropListEx();
    virtual ~CUIDragDropListEx();
    void InitDragDropList(Fvector2 pos, Fvector2 size);

    typedef fastdelegate::FastDelegate1<CUICellItem*, bool> DRAG_CELL_EVENT;
    typedef fastdelegate::FastDelegate2<CUIDragItem*, bool, void> DRAG_ITEM_EVENT;

    DRAG_CELL_EVENT m_f_item_drop;
    DRAG_CELL_EVENT m_f_item_start_drag;
    DRAG_CELL_EVENT m_f_item_db_click;
    DRAG_CELL_EVENT m_f_item_selected;
    DRAG_CELL_EVENT m_f_item_lbutton_click;
    DRAG_CELL_EVENT m_f_item_rbutton_click;
    DRAG_CELL_EVENT m_f_item_focus_received;
    DRAG_CELL_EVENT m_f_item_focus_lost;
    DRAG_CELL_EVENT m_f_item_focused_update;
    DRAG_ITEM_EVENT m_f_drag_event;

    u32 back_color;

    const Ivector2& CellsCapacity();
    void SetCellsCapacity(const Ivector2 c);
    void SetStartCellsCapacity(const Ivector2 c)
    {
        m_orig_cell_capacity = c;
        SetCellsCapacity(c);
    };
    void ResetCellsCapacity()
    {
        VERIFY(ItemsCount() == 0);
        SetCellsCapacity(m_orig_cell_capacity);
    };
    const Ivector2& CellSize();
    void SetCellSize(const Ivector2 new_sz);
    const Ivector2& CellsSpacing();
    void SetCellsSpacing(const Ivector2& new_sz);
    void SetCellsVertAlignment(xr_string alignment);
    void SetCellsHorizAlignment(xr_string alignment);

    const Ivector2 GetVirtualCellsAlignment() { return m_virtual_cells_alignment; };
    int ScrollPos();
    void ReinitScroll();
    void GetClientArea(Frect& r);
    Fvector2 GetDragItemPosition();

    void SetAutoGrow(bool b);
    bool IsAutoGrow();
    void SetGrouping(bool b);
    bool IsGrouping();
    void SetCustomPlacement(bool b);
    bool GetCustomPlacement();
    void SetVerticalPlacement(bool b);
    bool GetVerticalPlacement();
    void SetAlwaysShowScroll(bool b);
    bool GetVirtualCells();
    void SetVirtualCells(bool b);

    bool GetConditionProgBarVisibility() { return m_bConditionProgBarVisible; };
    void SetConditionProgBarVisibility(bool b) { m_bConditionProgBarVisible = b; };
public:
    // items management
    virtual void SetItem(CUICellItem* itm); // auto
    virtual void SetItem(CUICellItem* itm, Fvector2 abs_pos); // start at cursor pos
    virtual void SetItem(CUICellItem* itm, Ivector2 cell_pos); // start at cell
    bool CanSetItem(CUICellItem* itm);

    u32 ItemsCount();
    CUICellItem* GetItemIdx(u32 idx);
    virtual CUICellItem* RemoveItem(CUICellItem* itm, bool force_root);
    void CreateDragItem(CUICellItem* itm);

    void DestroyDragItem();
    void ClearAll(bool bDestroy);
    void Compact();
    bool IsOwner(CUICellItem* itm);
    void clear_select_armament();
    Ivector2 PickCell(const Fvector2& abs_pos);
    CUICell& GetCellAt(const Ivector2& pos);
    CUICellContainer* GetContainer() { return m_container; }; //Alundaio

public:
    // UIWindow overriding
    virtual void Draw();
    virtual void Update();
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    void OnDragEvent(CUIDragItem* drag_item, bool b_receive);
};

class CUICellContainer : public CUIWindow
{
    friend class CUIDragDropListEx;
    friend class CUIDragDropReferenceList;

private:
    typedef CUIWindow inherited;
    ui_shader hShader;
    UI_CELLS_VEC m_cells_to_draw;

protected:
    CUIDragDropListEx* m_pParentDragDropList;

    Ivector2 m_cellsCapacity; // count		(col,	row)
    Ivector2 m_cellSize; // pixels	(width, height)
    Ivector2 m_cellSpacing; // pixels	(width, height)

    UI_CELLS_VEC m_cells;

    void GetTexUVLT(Fvector2& uv, u32 col, u32 row, u8 select_mode);
    void ReinitSize();
    u32 GetCellsInRange(const Irect& rect, UI_CELLS_VEC& res);

public:
    CUICellContainer(CUIDragDropListEx* parent);
    virtual ~CUICellContainer();

protected:
    virtual void Draw();

    IC const Ivector2& CellsCapacity() { return m_cellsCapacity; };
    void SetCellsCapacity(const Ivector2& c);
    IC const Ivector2& CellSize() { return m_cellSize; };
    void SetCellSize(const Ivector2& new_sz);
    IC const Ivector2& CellsSpacing() { return m_cellSpacing; };
    void SetCellsSpacing(const Ivector2& new_sz);
    Ivector2 TopVisibleCell();
    CUICell& GetCellAt(const Ivector2& pos);
    Ivector2 PickCell(const Fvector2& abs_pos);
    Ivector2 GetItemPos(CUICellItem* itm);
    Ivector2 FindFreeCell(const Ivector2& size);
    bool HasFreeSpace(const Ivector2& size);
    bool IsRoomFree(const Ivector2& pos, const Ivector2& size);

    bool AddSimilar(CUICellItem* itm);
    CUICellItem* FindSimilar(CUICellItem* itm);

    void PlaceItemAtPos(CUICellItem* itm, Ivector2& cell_pos);
    CUICellItem* RemoveItem(CUICellItem* itm, bool force_root);
    bool ValidCell(const Ivector2& pos) const;

    void Grow();
    void Shrink();
    void ClearAll(bool bDestroy);
    void clear_select_armament();
};
