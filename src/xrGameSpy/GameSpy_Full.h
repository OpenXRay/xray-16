#pragma once
#include "xrGameSpy/xrGameSpy.h"

class CGameSpy_Available;
class CGameSpy_Patching;
class CGameSpy_HTTP;
enum class GSUpdateStatus;
class CGameSpy_Browser;
class CGameSpy_GP;
class CGameSpy_SAKE;
class CGameSpy_ATLAS;

class XRGAMESPY_API CGameSpy_Full
{
    bool m_bServicesAlreadyChecked;

public:
    CGameSpy_Full();
    ~CGameSpy_Full();

    CGameSpy_Available* GetGameSpyAvailable() const { return m_pGSA; }
    CGameSpy_Patching* GetGameSpyPatching() const { return m_pGS_Patching; }
    CGameSpy_HTTP* GetGameSpyHTTP() const { return m_pGS_HTTP; }
    CGameSpy_Browser* GetGameSpyBrowser() const { return m_pGS_SB; }
    CGameSpy_GP* GetGameSpyGP() const { return m_pGS_GP; }
    CGameSpy_SAKE* GetGameSpySAKE() const { return m_pGS_SAKE; }
    CGameSpy_ATLAS* GetGameSpyATLAS() const { return m_pGS_ATLAS; }
    GSUpdateStatus Update();
    void CoreThink(u32 millisecondsTimeout);

private:
    CGameSpy_Available* m_pGSA;
    CGameSpy_Patching* m_pGS_Patching;
    CGameSpy_HTTP* m_pGS_HTTP;
    CGameSpy_Browser* m_pGS_SB;
    CGameSpy_GP* m_pGS_GP;
    CGameSpy_SAKE* m_pGS_SAKE;
    CGameSpy_ATLAS* m_pGS_ATLAS;
};
