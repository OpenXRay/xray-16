#include "stdafx.h"
#include "GameSpy_Available.h"

bool CGameSpy_Available::CheckAvailableServices(shared_str& resultstr)
{
    GSIACResult result;
    GSIStartAvailableCheckA(GAMESPY_GAMENAME);

    while ((result = GSIAvailableCheckThink()) == GSIACWaiting)
        msleep(5);

    if (result != GSIACAvailable)
    {
        switch (result)
        {
        case GSIACUnavailable: { resultstr = "! Online Services for STALKER are no longer available.";
        }
        break;
        case GSIACTemporarilyUnavailable:
        {
            resultstr = "! Online Services for STALKER are temporarily down for maintenance.";
        }
        break;
        }
        return false;
    }
    else
    {
        resultstr = "Success";
    };
    return true;
};
