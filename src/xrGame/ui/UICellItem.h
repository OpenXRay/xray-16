#pragma once

#include "xrUICore/Static/UIStatic.h"
#include "UIDialogWnd.h"

class CUIDragItem;
class CUIDragDropListEx;
class CUICellItem;
class CUIProgressBar;

class ICustomDrawCellItem
{
public:
    virtual ~ICustomDrawCellItem(){};
    virtual void OnDraw(CUICellItem* cell) = 0;
};

class ICustomDrawDragItem
{
public:
    virtual ~ICustomDrawDragItem(){};
    virtual void OnDraw(CUIDragItem* drag_item) = 0;
};

class CUICellItem : public CUIStatic
{
private:
    typedef CUIStatic inherited;

protected:
    xr_vector<CUICellItem*> m_childs;

    CUIDragDropListEx* m_pParentList;
    CUIProgressBar* m_pConditionState;
    Ivector2 m_grid_size;
    ICustomDrawCellItem* m_custom_draw;
    int m_accelerator;
    CUIStatic* m_text;
    CUIStatic* m_upgrade;
    Fvector2 m_upgrade_pos;

    virtual void UpdateItemText();
    void init();

public:
    CUICellItem();
    virtual ~CUICellItem();

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void Draw();
    virtual void Update();
    virtual void OnAfterChild(CUIDragDropListEx* parent_list){};

    u32 ChildsCount();
    void PushChild(CUICellItem*);
    CUICellItem* PopChild(CUICellItem*);
    CUICellItem* Child(u32 idx) { return m_childs[idx]; };
    bool HasChild(CUICellItem* item);
    virtual bool EqualTo(CUICellItem* itm);
    IC const Ivector2& GetGridSize() { return m_grid_size; }; // size in grid
    IC void SetAccelerator(int dik) { m_accelerator = dik; };
    IC int GetAccelerator() const { return m_accelerator; };
    virtual CUIDragItem* CreateDragItem();

    CUIDragDropListEx* OwnerList() { return m_pParentList; }
    void SetOwnerList(CUIDragDropListEx* p);
    void UpdateConditionProgressBar();
    void SetCustomDraw(ICustomDrawCellItem* c);
    void Mark(bool status);
    CUIStatic& get_ui_text() const { return *m_text; }
    virtual bool IsHelper() { return false; }
    virtual void SetIsHelper(bool is_helper) { ; }
public:
    static CUICellItem* m_mouse_selected_item;
    void* m_pData;
    int m_index;
    u32 m_drawn_frame;
    bool m_b_destroy_childs;
    bool m_selected;
    bool m_select_armament;
    bool m_cur_mark;
    bool m_has_upgrade;
};

class CUIDragItem : public CUIWindow, public pureRender, public pureFrame
{
    typedef CUIWindow inherited;
    CUIStatic m_static;
    CUICellItem* m_pParent;
    Fvector2 m_pos_offset;
    CUIDragDropListEx* m_back_list;
    ICustomDrawDragItem* m_custom_draw;

public:
    CUIDragItem(CUICellItem* parent);
    virtual void Init(const ui_shader& sh, const Frect& rect, const Frect& text_rect);
    virtual ~CUIDragItem();
    void SetCustomDraw(ICustomDrawDragItem* c);

    CUIStatic* wnd() { return &m_static; }
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void Draw();

    virtual void OnRender();
    virtual void OnFrame();

    CUICellItem* ParentItem() { return m_pParent; }
    void SetBackList(CUIDragDropListEx* l);
    CUIDragDropListEx* BackList() { return m_back_list; }
    Fvector2 GetPosition();
};
