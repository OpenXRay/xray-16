///////////////////////////////////////////////////////////////
// RustyHairArtifact.h
// RustyHairArtefact - артефакт ржавые волосы
///////////////////////////////////////////////////////////////

#pragma once
#include "Artefact.h"

class CRustyHairArtefact : public CArtefact
{
private:
    typedef CArtefact inherited;

public:
    CRustyHairArtefact(void);
    virtual ~CRustyHairArtefact(void);

    virtual void Load(LPCSTR section);

protected:
};
