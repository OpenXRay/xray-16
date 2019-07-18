#pragma once

#include "xrUICore/Windows/UIWindow.h"

class CUIFrameWindow;
class CUITextFrameLineWnd;
class CUIAnimatedStatic;
class CUIStatic;
class CUICharacterInfo;
class CUIScrollView;
class CUIXml;

class CUIActorInfoWnd : public CUIWindow
{
    typedef CUIWindow inherited;

public:
    CUIActorInfoWnd();
    bool Init();

    virtual void Show(bool status);
    CUIScrollView& DetailList() { return *UIDetailList; }
    CUIScrollView& MasterList() { return *UIMasterList; }
    void FillPointsDetail(const shared_str& idx);
    virtual void Reset();

protected:
    CUIFrameWindow* UIInfoFrame;
    CUITextFrameLineWnd* UIInfoHeader;
    CUIFrameWindow* UICharIconFrame;
    CUITextFrameLineWnd* UICharIconHeader;
    CUIAnimatedStatic* UIAnimatedIcon;

    CUIWindow* UICharacterWindow;
    CUICharacterInfo* UICharacterInfo;

    CUIScrollView* UIMasterList;
    CUIScrollView* UIDetailList;
    void FillPointsInfo();
    void FillReputationDetails(CUIXml* xml, LPCSTR path);
    void FillMasterPart(CUIXml* xml, const shared_str& key_name);
};

class CUIActorStaticticHeader : public CUIWindow, public CUISelectable
{
    CUIActorInfoWnd* m_actorInfoWnd;
protected:
    u32 m_stored_alpha;
public:
    CUIStatic* m_text1;
    CUIStatic* m_text2;
public:
    CUIActorStaticticHeader(CUIActorInfoWnd* w);
    void Init(CUIXml* xml, LPCSTR path, int idx_in_xml);
    virtual bool OnMouseDown(int mouse_btn);
    virtual void SetSelected(bool b);

    shared_str m_id;
};

class CUIActorStaticticDetail : public CUIWindow
{
protected:
public:
    CUIStatic* m_text0;
    CUIStatic* m_text1;
    CUIStatic* m_text2;
    CUIStatic* m_text3;
public:
    void Init(CUIXml* xml, LPCSTR path, int xml_idx);
};
