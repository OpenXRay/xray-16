///////////////////////////////////////////////////////////////
// ScientificOutfit.h
// ScientificOutfit - защитный костюм ученого
///////////////////////////////////////////////////////////////

#pragma once

#include "CustomOutfit.h"

class CScientificOutfit : public CCustomOutfit
{
private:
    typedef CCustomOutfit inherited;

public:
    CScientificOutfit(void);
    virtual ~CScientificOutfit(void);
};
