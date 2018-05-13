#include "stdafx.h"
#include "GameSpy_GCD_Client.h"

void CGameSpy_GCD_Client::CreateRespond(char* cdkey, char* RespondStr, char* ChallengeStr, u8 Reauth)
{
    gcd_compute_response(
        xr_strupr(cdkey), ChallengeStr, RespondStr, Reauth == 1 ? CDResponseMethod_REAUTH : CDResponseMethod_NEWAUTH);
}
