///////////////////////////////////////////////////////////////
// DummyArtifact.h
// DummyArtefact - артефакт пустышка
///////////////////////////////////////////////////////////////

#pragma once
#include "Artefact.h"

class CDummyArtefact : public CArtefact
{
private:
    typedef CArtefact inherited;

public:
    CDummyArtefact(void);
    virtual ~CDummyArtefact(void);

    virtual void Load(LPCSTR section);

protected:
};
