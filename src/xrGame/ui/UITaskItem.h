#pragma once
#include "UIDialogWnd.h"
#include "xrUICore/ListWnd/UIListItem.h"
#include "xrUICore/Callbacks/UIWndCallback.h"

class CGameTask;
class CUIStatic;
class CUIButton;
class SGameTaskObjective;
class CUIEventsWnd;
class CUIEditBoxEx;
class CUIEditBox;

class CUISocTaskItem : public CUIListItem, public CUIWndCallback
{
    typedef CUIListItem inherited;

protected:
    CGameTask* m_GameTask;
    u16 m_TaskObjectiveIdx;
    void OnItemClicked(CUIWindow*, void*);
    void Init();

public:
    CUISocTaskItem(CUIEventsWnd* w);
    virtual ~CUISocTaskItem();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    virtual void SetGameTask(CGameTask* gt, u16 obj_idx);

    CGameTask* GameTask() { return m_GameTask; }
    u16 ObjectiveIdx() { return m_TaskObjectiveIdx; }
    SGameTaskObjective* Objective();

    CUIEventsWnd* m_EventsWnd;
};

class CUITaskRootItem : public CUISocTaskItem
{
    typedef CUISocTaskItem inherited;

protected:
    CUIStatic* m_taskImage;
    CUIStatic* m_captionStatic;
    CUIStatic* m_captionTime;
    CUIStatic* m_remTimeStatic;
    CUI3tButton* m_switchDescriptionBtn;
    bool m_curr_descr_mode;
    void Init();

public:
    CUITaskRootItem(CUIEventsWnd* w);
    virtual ~CUITaskRootItem();
    virtual void Update();
    virtual void SetGameTask(CGameTask* gt, u16 obj_idx);
    void OnSwitchDescriptionClicked(CUIWindow*, void*);

    virtual void MarkSelected(bool b);
    virtual bool OnDbClick();
};

class CUITaskSubItem : public CUISocTaskItem
{
    typedef CUISocTaskItem inherited;
    u32 m_active_color;
    u32 m_failed_color;
    u32 m_accomplished_color;
    u32 m_skiped_color;

protected:
    CUIStatic* m_ActiveObjectiveStatic;
    CUI3tButton* m_showDescriptionBtn;
    CUIStatic* m_descriptionStatic;
    CUIStatic* m_stateStatic;

    void Init();

public:
    CUITaskSubItem(CUIEventsWnd* w);
    virtual ~CUITaskSubItem();
    virtual void Update();
    virtual void SetGameTask(CGameTask* gt, u16 obj_idx);
    void OnActiveObjectiveClicked();
    void OnShowDescriptionClicked(CUIWindow*, void*);
    virtual void MarkSelected(bool b);
    virtual bool OnDbClick();
};
/*
class CUIUserTaskEditWnd;
class CUIUserTaskItem :public CUITaskItem
{
    typedef			CUITaskItem	inherited;
protected:
    CUI3tButton*	m_showPointerBtn;
    CUI3tButton*	m_showLocationBtn;
    CUI3tButton*	m_editTextBtn;
    CUI3tButton*	m_removeBtn;
    CUIStatic*		m_captionStatic;
    CUIStatic*		m_descriptionStatic;
    CUIStatic*		m_image;
    CUIUserTaskEditWnd* m_edtWnd;
    void			Init					();

public:
                    CUIUserTaskItem			(CUIEventsWnd* w);
    virtual			~CUIUserTaskItem			();
    virtual void	Update					();
    virtual void	SetGameTask				(CGameTask* gt, u16 obj_idx);
            void	OnShowLocationClicked	();
            void	OnShowPointerClicked	();
            void	OnDescriptionChanged	();
            void	OnEditTextClicked		();
            void	OnRemoveClicked			();

    virtual bool	OnDbClick				()	{return true;};
    virtual void	MarkSelected			(bool b);
};

class CUIUserTaskEditWnd : public CUIDialogWnd, public CUIWndCallback
{
    CUIUserTaskItem*		m_userTask;
    CUI3tButton*			m_btnOk;
    CUI3tButton*			m_btnCancel;
    CUIFrameWindow*			m_background;

    CUIEditBox*			m_editCaption;
    CUIEditBoxEx*			m_editDescription;
protected:
            void			OnOk					();
            void			OnCancel				();
            void			Init					();
public:
                            CUIUserTaskEditWnd		(CUIUserTaskItem* itm);
    virtual void			SendMessage				(CUIWindow* pWnd, s16 msg, void* pData = NULL);
            void			Start					();
};*/
