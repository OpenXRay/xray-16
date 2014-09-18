///////////////////////////////////////////////////////////////
// ExoOutfit.h
// ExoOutfit - защитный костюм с усилением
///////////////////////////////////////////////////////////////


#pragma once

#include "customoutfit.h"

class CExoOutfit: public CCustomOutfit
{
private:
    typedef	CCustomOutfit inherited;
public:
	CExoOutfit(void);
	virtual ~CExoOutfit(void);
};
