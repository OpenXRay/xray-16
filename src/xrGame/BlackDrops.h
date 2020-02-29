///////////////////////////////////////////////////////////////
// BlackDrops.h
// BlackDrops - черные капли
///////////////////////////////////////////////////////////////

#pragma once
#include "Artefact.h"

class CBlackDrops : public CArtefact
{
private:
    typedef CArtefact inherited;

public:
    CBlackDrops(void);
    virtual ~CBlackDrops(void);

    virtual void Load(const char* section);

protected:
};
