#pragma once
#include "xrUICore/Windows/UIWindow.h"

class CUIStatic;
class CUITextWnd;
class CUIXml;
class CUIScrollView;
class CMMSound;

class CUIMMMagnifer : public CUIStatic
{
public:
    CUIMMMagnifer();
    virtual ~CUIMMMagnifer();
    void SetPPMode();
    void ResetPPMode();
    bool GetPPMode() { return m_bPP; };

protected:
    bool m_bPP;
};

class CUIMMShniaga : public CUIWindow, public CDeviceResetNotifier
{
public:
    CUIMMShniaga();
    virtual ~CUIMMShniaga();

    void InitShniaga(CUIXml& xml_doc, LPCSTR path);
    virtual void Update();
    virtual void Draw();

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = 0);
    void SetVisibleMagnifier(bool f);
    virtual void OnDeviceReset();
    enum enum_page_id
    {
        epi_main = 0x00,
        epi_new_game,
        epi_new_network_game,
        epi_none
    }; // enum	enum_page_id
    void SetPage(enum_page_id page_id, LPCSTR xml_file, LPCSTR xml_path);
    void ShowPage(enum_page_id page_id);

protected:
    typedef enum { E_Begin = 0, E_Update, E_Finilize, E_Stop } EVENT;

    void SelectBtn(int btn);
    void SelectBtn(CUIWindow* btn);
    int BtnCount();
    void OnBtnClick();

    void ProcessEvent(EVENT ev);

    bool IsButton(CUIWindow* st);
    void CreateList(xr_vector<CUITextWnd*>& lst, CUIXml& xml_doc, LPCSTR path, bool required = true);
    void ShowMain();
    void ShowNewGame();
    void ShowNetworkGame();
    float pos(float x1, float x2, u32 t);

    CUIStatic* m_shniaga;
    CUIMMMagnifer* m_magnifier;
    CUIScrollView* m_view;

    u32 m_start_time;
    u32 m_run_time;
    float m_origin;
    float m_destination;
    float m_mag_pos;
    float m_offset;

    xr_vector<CUITextWnd*> m_buttons;
    xr_vector<CUITextWnd*> m_buttons_new;
    xr_vector<CUITextWnd*> m_buttons_new_network;

    int m_selected_btn;
    enum_page_id m_page;
    CUIWindow* m_selected;
    CMMSound* m_sound;
    //	Fvector2				m_wheel_size[2];
    enum
    {
        fl_SoundFinalized = 1,
        fl_MovingStoped = 2
    };

    Flags32 m_flags;
};
