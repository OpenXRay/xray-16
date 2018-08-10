#pragma once

#include "xrUICore/ScrollView/UIScrollView.h"
#include "UIStatsPlayerInfo.h"

class CUIXml;
class CUITextWnd;

typedef bool (*player_cmp_func)(LPVOID v1, LPVOID v2);

class CUIStatsPlayerList : public CUIScrollView
{
public:
    CUIStatsPlayerList();
    virtual ~CUIStatsPlayerList();

    void Init(CUIXml& xml_doc, LPCSTR path);
    void SetSpectator(bool f);
    void SetTeam(int team);
    void AddField(const char* name, float width);
    CUIStatic* GetHeader();
    CUIWindow* GetTeamHeader();
    void SetTextParams(CGameFont* pF, u32 col);
    void SetHeaderHeight(float h);
    virtual void AddWindow(CUIWindow* pWnd, bool auto_delete = true);
    virtual void Update();

protected:
    void InitHeader(CUIXml& xml_doc, LPCSTR path);
    void InitTeamHeader(CUIXml& xml_doc, LPCSTR path);
    virtual void RecalcSize();
    void ShowHeader(bool bShow);
    LPCSTR GetST_entry(LPCSTR itm);

    int m_CurTeam;
    bool m_bSpectator;
    bool m_bStatus_mode;

    xr_vector<PI_FIELD_INFO> m_field_info;

    CUIStatic* m_header;
    CUIWindow* m_header_team;
    CUITextWnd* m_header_text;
    u32 m_prev_upd_time;

    typedef struct
    {
        u32 c; // color
        CGameFont* f; // font
        float h; // height
    } S_ELEMENT;

    S_ELEMENT m_h; // header
    S_ELEMENT m_i; // item
    S_ELEMENT m_t; // team header
};
