///////////////////////////////////////////////////////////////
// ZudaArtifact.h
// ZudaArtefact - артефакт зуда
///////////////////////////////////////////////////////////////

#pragma once
#include "Artefact.h"

class CZudaArtefact : public CArtefact
{
private:
    typedef CArtefact inherited;

public:
    CZudaArtefact(void);
    virtual ~CZudaArtefact(void);

    virtual void Load(LPCSTR section);

protected:
};
