///////////////////////////////////////////////////////////////
// ScientificOutfit.h
// ScientificOutfit - защитный костюм ученого
///////////////////////////////////////////////////////////////


#pragma once

#include "customoutfit.h"

class CScientificOutfit: public CCustomOutfit
{
private:
    typedef	CCustomOutfit inherited;
public:
	CScientificOutfit(void);
	virtual ~CScientificOutfit(void);
};
