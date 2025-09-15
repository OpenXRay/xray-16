
#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "../encyclopedia_article_defs.h"

class CUINewsItemWnd;
class CUIFrameLineWnd;
class CUIFrameWindow;
class CUIAnimatedStatic;
class CUIStatic;
class CUITabControl;
class CUIScrollView;
class CUIListWnd;
class CEncyclopediaArticle;

class CUIDiaryWnd : public CUIWindow, public CUIWndCallback
{
private:
    typedef CUIWindow inherited;
    enum EDiaryFilter : u32 {
        eJournal,
        eNews,
        eNone,
        eInfo
    };
    enum EDiarySections : u32 {
        eDNews = (1 << 10) | (1 << 1),
        eDInfo = (1 << 10) | (1 << 2),
        eDJournal = (1 << 10) | (1 << 3),
    };
protected:
    u32 g_pda_info_state;
    EDiaryFilter m_currFilter;

    CUINewsItemWnd* m_UINewsWnd;

    CUIWindow* m_UILeftWnd;
    CUIWindow* m_UIRightWnd;
    CUIFrameWindow* m_UILeftFrame;
    CUIFrameLineWnd* m_UILeftHeader;
    CUIFrameWindow* m_UIRightFrame;
    CUIFrameLineWnd* m_UIRightHeader;
    CUIAnimatedStatic* m_UIAnimation;
    CUITabControl* m_FilterTab;
    CUIListWnd* m_SrcListWnd;
    CUIScrollView* m_DescrView;
    CGameFont* m_pTreeRootFont;
    u32 m_uTreeRootColor;
    CGameFont* m_pTreeItemFont;
    u32 m_uTreeItemColor;

    xr_vector<Fvector2> m_sign_places;
    CUIStatic* m_updatedSectionImage;
    CUIStatic* m_oldSectionImage;

    typedef xr_vector<CEncyclopediaArticle*> ArticlesDB;
    typedef xr_vector<CEncyclopediaArticle*>::iterator ArticlesDB_it;
    ArticlesDB m_ArticlesDB;

    void __stdcall OnFilterChanged(CUIWindow*, void*);
    void __stdcall OnSrcListItemClicked(CUIWindow*, void*);
    void UnloadJournalTab();
    void LoadJournalTab(ARTICLE_DATA::EArticleType _type);
    void LoadInfoTab();
    void UnloadNewsTab();
    void LoadNewsTab();
    void Reload(EDiaryFilter new_filter);
public:
    CUIDiaryWnd();
    virtual ~CUIDiaryWnd();

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);
    virtual void Draw();
    virtual void Reset();

    bool Init();
    void AddNews();
    void RearrangeTabButtons(CUITabControl* pTab, xr_vector<Fvector2>& vec_sign_places);
    virtual void Show(bool status);
};
