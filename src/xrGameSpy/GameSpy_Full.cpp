#include "stdafx.h"
#include "GameSpy_Full.h"
#include "GameSpy_Available.h"
#include "GameSpy_Patching.h"
#include "GameSpy_HTTP.h"
#include "GameSpy_BrowsersWrapper.h"
#include "GameSpy_GP.h"
#include "GameSpy_SAKE.h"
#include "GameSpy_ATLAS.h"
#include "Common/object_broker.h"

CGameSpy_Full::CGameSpy_Full()
{
    m_pGSA = NULL;
    m_pGS_Patching = NULL;
    m_pGS_HTTP = NULL;
    m_pGS_SB = NULL;
    m_pGS_GP = NULL;
    m_bServicesAlreadyChecked = false;
    //---------------------------------------
    m_pGSA = xr_new<CGameSpy_Available>();
    //-----------------------------------------------------
    shared_str resultstr;
    m_bServicesAlreadyChecked = m_pGSA->CheckAvailableServices(resultstr);
    //-----------------------------------------------------
    gsCoreInitialize();
    m_pGS_Patching = xr_new<CGameSpy_Patching>();
    m_pGS_HTTP = xr_new<CGameSpy_HTTP>();
    m_pGS_SB = xr_new<CGameSpy_BrowsersWrapper>();

    m_pGS_GP = xr_new<CGameSpy_GP>();
    m_pGS_SAKE = xr_new<CGameSpy_SAKE>();
    m_pGS_ATLAS = xr_new<CGameSpy_ATLAS>();
}

CGameSpy_Full::~CGameSpy_Full()
{
    delete_data(m_pGSA);
    delete_data(m_pGS_Patching);
    delete_data(m_pGS_HTTP);
    delete_data(m_pGS_SB);
    delete_data(m_pGS_GP);
    delete_data(m_pGS_SAKE);
    delete_data(m_pGS_ATLAS);
    gsCoreShutdown();
}

GSUpdateStatus CGameSpy_Full::Update()
{
    if (!m_bServicesAlreadyChecked)
    {
        m_bServicesAlreadyChecked = true;
        return GSUpdateStatus::OutOfService;
    }
    m_pGS_HTTP->Think();
    GSUpdateStatus status = m_pGS_SB->Update();
    m_pGS_GP->Think();
    CoreThink(15);
    m_pGS_ATLAS->Think();
    return status;
}

void CGameSpy_Full::CoreThink(u32 millisecondsTimeout) { gsCoreThink(millisecondsTimeout); }
