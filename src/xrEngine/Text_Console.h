#pragma once
#include "XR_IOConsole.h"
#include "IGame_Level.h"
#include "xrEngine/ILoadingScreen.h"

class TextLoadingScreen : public ILoadingScreen
{
public:
    ~TextLoadingScreen() = default;

    void Initialize() override;

    void Show(bool status) override;
    bool IsShown() override;

    void Update(int stagesCompleted, int stagesTotal) override;
    void ForceDrop() override;
    void ForceFinish() override;

    void SetLevelLogo(cpcstr name) override;
    void SetStageTitle(cpcstr title) override;
    void SetStageTip(cpcstr header, cpcstr tipNumber, cpcstr tip) override;
};

class ENGINE_API CTextConsole : public CConsole
{
private:
    typedef CConsole inherited;

private:
    HWND* m_pMainWnd;

    HWND m_hConsoleWnd;
    void CreateConsoleWnd();

    HWND m_hLogWnd;
    void CreateLogWnd();

    bool m_bScrollLog;
    u32 m_dwStartLine;
    void DrawLog(HDC hDC, RECT* pRect);

private:
    HFONT m_hLogWndFont;
    HFONT m_hPrevFont;
    HBRUSH m_hBackGroundBrush;

    HDC m_hDC_LogWnd;
    HDC m_hDC_LogWnd_BackBuffer;
    HBITMAP m_hBB_BM, m_hOld_BM;

    bool m_bNeedUpdate;
    u32 m_dwLastUpdateTime;

    u32 m_last_time;
    CServerInfo m_server_info;

public:
    CTextConsole();
    virtual ~CTextConsole();

    virtual void Initialize();
    virtual void Destroy();

    void OnDeviceInitialize() override;

    virtual void OnRender();
    virtual void OnFrame();

    // virtual void IR_OnKeyboardPress (int dik);

    void AddString(pcstr string);
    void OnPaint();

}; // class TextConsole

// extern ENGINE_API CTextConsole* TextConsole;
