#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "UIXmlInit.h"

class CUIFrameWindow;
class CUIFrameLineWnd;
class CUIAnimatedStatic;
class CUIMapWnd;
class CUI3tButton;
class CUITabControl;
class CGameTask;
class CUITaskDescrWnd;
class CUIScrollView;
class CUISocTaskItem;

class CUIEventsWnd : public CUIWindow, public CUIWndCallback
{
    typedef CUIWindow inherited;
    enum ETaskFilters
    {
        eActiveTask = 0,
        eAccomplishedTask,
        eFailedTask,
        //.						eOwnTask,
        eMaxTask
    };
    enum EEventWndFlags
    {
        flNeedReload = (1 << 0),
        flMapMode = (1 << 1),
    };
    Flags16 m_flags;
    ETaskFilters m_currFilter;
    CUIFrameWindow* m_UILeftFrame;
    CUIWindow* m_UIRightWnd;
    CUIFrameLineWnd* m_UILeftHeader;
    CUIAnimatedStatic* m_UIAnimation;
    CUIMapWnd* m_UIMapWnd;
    CUITaskDescrWnd* m_UITaskInfoWnd;
    CUIScrollView* m_ListWnd;
    CUITabControl* m_TaskFilter;
    CGameTask* m_descriptionTask{};

    bool Filter(CGameTask* t);
    void OnFilterChanged(CUIWindow*, void*);
    void ReloadList(bool bClearOnly);

public:
    void SetDescriptionMode(bool bMap);
    bool GetDescriptionMode();
    bool IsTaskDescriptionShown(const CGameTask* task) const;
    void ShowDescription(CGameTask* t, int idx);
    bool ItemHasDescription(CUISocTaskItem*);

public:
    CUIEventsWnd();
    virtual ~CUIEventsWnd();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    void Init();
    virtual void Update();
    virtual void Draw();
    virtual void Show(bool status);
    void Reload();
    virtual void Reset();

    pcstr GetDebugType() override { return "CUIEventsWnd"; }

    CUIXml m_ui_task_item_xml;
};
