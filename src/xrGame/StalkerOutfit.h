///////////////////////////////////////////////////////////////
// StalkerOutfit.cpp
// StalkerOutfit - защитный костюм сталкера
///////////////////////////////////////////////////////////////

#pragma once

#include "CustomOutfit.h"

class CStalkerOutfit : public CCustomOutfit
{
private:
    typedef CCustomOutfit inherited;

public:
    CStalkerOutfit(void);
    virtual ~CStalkerOutfit(void);

private:
    DECLARE_SCRIPT_REGISTER_FUNCTION(CGameObject);
};
