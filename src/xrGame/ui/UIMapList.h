#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "gametype_chooser.h"
#include "UIGameCustom.h"

class CUIListBox;
class CUIListBoxItem;
class CUIFrameLineWnd;
class CUIStatic;
class CUIFrameWindow;
class CUI3tButton;
class CUISpinText;
class CUIMapInfo;
class CUIComboBox;
class CUIXml;

#define MAP_ROTATION_LIST "maprot_list.ltx"

class CUIMapList : public CUIWindow
{
public:
    CUIMapList();
    virtual ~CUIMapList();
    virtual void Update();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
    void InitFromXml(CUIXml& xml_doc, const char* path);

    void SetWeatherSelector(CUIComboBox* ws);
    void SetModeSelector(CUIWindow* ms);
    void SetMapPic(CUIStatic* map_pic);
    void SetMapInfo(CUIMapInfo* map_info);
    void SetServerParams(LPCSTR params);
    void OnModeChange();
    void OnListItemClicked();
    void LoadMapList();
    void SaveMapList();
    const char* GetCommandLine(LPCSTR player_name);
    EGameIDs GetCurGameType();
    void StartDedicatedServer();
    void ClearList();
    bool IsEmpty();
    const MPLevelDesc& GetMapNameInt(EGameIDs _type, u32 idx);

private:
    CUIListBoxItem* GetMapItem_fromList1(shared_str const& map_name);
    void UpdateMapList(EGameIDs GameType);
    void SaveRightList();

    void OnBtnLeftClick();
    void OnBtnRightClick();
    void OnBtnUpClick();
    void OnBtnDownClick();
    void AddWeather(const shared_str& WeatherType, const shared_str& WeatherTime, u32 _id);
    void ParseWeather(char** ps, char* e);

    CUIListBox* m_pList1;
    CUIListBox* m_pList2;
    CUIFrameWindow* m_pFrame1;
    CUIFrameWindow* m_pFrame2;
    CUIFrameLineWnd* m_pLbl1;
    CUIFrameLineWnd* m_pLbl2;
    CUI3tButton* m_pBtnLeft;
    CUI3tButton* m_pBtnRight;
    CUI3tButton* m_pBtnUp;
    CUI3tButton* m_pBtnDown;

    CUIComboBox* m_pWeatherSelector;
    CUIWindow* m_pModeSelector;
    //	CUISpinText*		m_pModeSelector;
    CUIStatic* m_pMapPic;
    CUIMapInfo* m_pMapInfo;
    // XXX nitrocaster: use MPWeatherDesc
    struct Sw
    {
        shared_str weather_name;
        shared_str weather_time;
    };
    xr_vector<Sw> m_mapWeather;
    xr_string m_command;
    xr_string m_srv_params;
};
