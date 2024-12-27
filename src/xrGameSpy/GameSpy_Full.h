#pragma once
#include "xrGameSpy/xrGameSpy.h"

class CGameSpy_Available;
class CGameSpy_HTTP;
enum class GSUpdateStatus;
class CGameSpy_BrowsersWrapper;
class CGameSpy_GP;
class CGameSpy_ATLAS;

class XRGAMESPY_API CGameSpy_Full
{
    bool m_bServicesAlreadyChecked;

public:
    CGameSpy_Full();
    ~CGameSpy_Full();

    CGameSpy_Available* GetGameSpyAvailable() const { return m_pGSA; }
    CGameSpy_HTTP* GetGameSpyHTTP() const { return m_pGS_HTTP; }
    CGameSpy_BrowsersWrapper* GetGameSpyBrowser() const { return m_pGS_SB; }
    CGameSpy_GP* GetGameSpyGP() const { return m_pGS_GP; }
    CGameSpy_ATLAS* GetGameSpyATLAS() const { return m_pGS_ATLAS; }
    GSUpdateStatus Update();
    void CoreThink(u32 millisecondsTimeout);

private:
    CGameSpy_Available* m_pGSA;
    CGameSpy_HTTP* m_pGS_HTTP;
    CGameSpy_BrowsersWrapper* m_pGS_SB;
    CGameSpy_GP* m_pGS_GP;
    CGameSpy_ATLAS* m_pGS_ATLAS;
};
