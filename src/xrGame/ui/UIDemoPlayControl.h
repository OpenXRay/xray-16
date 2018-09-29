#pragma once
#ifndef UIDEMOPLAY_CONTROL
#define UIDEMOPLAY_CONTROL

#include "UIDialogWnd.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "xrCore/buffer_vector.h"

class CUI3tButton;
class CUIProgressBar;
class CUIPropertiesBox;
class CUITextWnd;
class CUIStatic;
class demoplay_control;

class CUIDemoPlayControl : public CUIDialogWnd, public CUIWndCallback
{
    typedef CUIDialogWnd inherited;

public:
    CUIDemoPlayControl();
    ~CUIDemoPlayControl();

    virtual void Init();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
    // virtual bool	OnMouse			(float x, float y, EUIMessages mouse_action);
    // virtual bool	OnKeyboard		(int dik, EUIMessages keyboard_action);
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool WorkInPause() const { return true; }
    virtual void Update();
    Fvector2 const& GetLastCursorPos() const { return m_last_curr_pos; };
    void xr_stdcall OnRestart(CUIWindow* w, void* d);
    void xr_stdcall OnDecresaseSpeed(CUIWindow* w, void* d);
    void xr_stdcall OnPlayPause(CUIWindow* w, void* d);
    void xr_stdcall OnIncreaseSpeed(CUIWindow* w, void* d);
    void xr_stdcall OnRewindUntil(CUIWindow* w, void* d);
    void xr_stdcall OnRepeatRewind(CUIWindow* w, void* d);

    void xr_stdcall OnRewindTypeSelected(CUIWindow* w, void* d);
    void xr_stdcall OnRewindPlayerSelected(CUIWindow* w, void* d);

private:
    void StopRewind();
    void UIStartRewind();
    void xr_stdcall UIStopRewindCb();

    enum ERewindTypeTags
    {
        eRewindUntilStart = 0x00,
        eRewindUntilKill,
        eRewindUntilDie,
        eRewindUntilArtTake,
        eRewindUntilArtDrop,
        eRewindUntilArtDeliver
    }; // enum eRewindTypeTags
    typedef buffer_vector<shared_str> players_collection_t;

    ERewindTypeTags m_last_rewind_type;
    shared_str m_last_rewind_target; // player name

    Frect m_pbox_rect;
    Fvector2 m_rewind_type_pos;

    void InitRewindTypeList();
    CUIPropertiesBox* m_rewind_type;

    void InitAllPlayers(/* some players list */);
    CUIPropertiesBox* m_all_players;

    players_collection_t* m_players;
    void* m_players_store;

    CUIStatic* m_background;
    CUI3tButton* m_play_pause_btn;
    CUI3tButton* m_restart_btn;
    CUI3tButton* m_decrease_speed_btn;
    CUI3tButton* m_increase_speed_btn;
    CUI3tButton* m_rewind_until_btn;
    CUI3tButton* m_repeat_rewind_btn;
    CUIProgressBar* m_progress_bar;
    CUITextWnd* m_static_demo_status;

    Fvector2 m_last_curr_pos;
    demoplay_control* m_demo_play_control;
}; // class UIDemoPlayControl

#endif //#ifndef UIDEMOPLAY_CONTROL
