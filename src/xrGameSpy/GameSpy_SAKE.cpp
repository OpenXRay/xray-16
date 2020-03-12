#include "stdafx.h"
#include "GameSpy_SAKE.h"

shared_str const CGameSpy_SAKE::TryToTranslate(SAKERequestResult const& request_result)
{
    string16 digit_dest;
    LPCSTR tmp_string = NULL;
    _itoa_s(request_result, digit_dest, 10);
    STRCONCAT(tmp_string, "mp_sake_database_request_error_", digit_dest);
    return tmp_string;
}
shared_str const CGameSpy_SAKE::TryToTranslate(SAKEStartRequestResult const& request_result)
{
    string16 digit_dest;
    LPCSTR tmp_string = NULL;
    _itoa_s(request_result, digit_dest, 10);
    STRCONCAT(tmp_string, "mp_sake_database_start_request_error_", digit_dest);
    return tmp_string;
}

SAKEStartupResult xrGS_sakeStartup(SAKE* sakePtr)
{
    SAKEStartupResult tmp_res = sakeStartup(sakePtr);
    if (tmp_res == SAKEStartupResult_SUCCESS)
    {
        sakeSetGame(*sakePtr, GAMESPY_GAMENAME, GAMESPY_GAMEID, GAMESPY_GAMEKEY);
    }
    return tmp_res;
}

void CGameSpy_SAKE::Init()
{
    SAKEStartupResult result = sakeStartup(&m_sake_inst);
    if (result == SAKEStartupResult_SUCCESS)
    {
        sakeSetGame(m_sake_inst, GAMESPY_GAMENAME, GAMESPY_GAMEID, GAMESPY_GAMEKEY);
    }
    else
        Msg("! GameSpy SAKE: failed to initialize, error code: %d", result);
    VERIFY(result == SAKEStartupResult_SUCCESS);
}

CGameSpy_SAKE::CGameSpy_SAKE()
{
    m_sake_inst = NULL;
    Init();
}

CGameSpy_SAKE::~CGameSpy_SAKE()
{
    if (m_sake_inst)
    {
        sakeShutdown(m_sake_inst);
    }
}

void CGameSpy_SAKE::SetProfile(int profileId, const char* loginTicket)
{
    sakeSetProfile(m_sake_inst, profileId, loginTicket);
}

SAKEStartRequestResult CGameSpy_SAKE::GetRequestResult() { return sakeGetStartRequestResult(m_sake_inst); }
SAKERequest CGameSpy_SAKE::GetMyRecords(SAKEGetMyRecordsInput* input, SAKERequestCallback callback, void* userData)
{
    return sakeGetMyRecords(m_sake_inst, input, callback, userData);
}
SAKERequest CGameSpy_SAKE::CreateRecord(SAKECreateRecordInput* input, SAKERequestCallback callback, void* userdata)
{
    return sakeCreateRecord(m_sake_inst, input, callback, userdata);
}
SAKERequest CGameSpy_SAKE::UpdateRecord(SAKEUpdateRecordInput* input, SAKERequestCallback callback, void* userdata)
{
    return sakeUpdateRecord(m_sake_inst, input, callback, userdata);
}

SAKERequest CGameSpy_SAKE::SearchForRecords(
    SAKESearchForRecordsInput* input, SAKERequestCallback callback, void* userData)
{
    return sakeSearchForRecords(m_sake_inst, input, callback, userData);
}
