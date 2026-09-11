///////////////////////////////////////////////////////////////
// StalkerOutfit.cpp
// StalkerOutfit - защитный костюм сталкера
///////////////////////////////////////////////////////////////

#pragma once

#include "CustomOutfit.h"

class CStalkerOutfit : public CCustomOutfit
{
protected:
    using inherited = CCustomOutfit;

private:
    DECLARE_SCRIPT_REGISTER_FUNCTION(CGameObject);
};
