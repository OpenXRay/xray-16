///////////////////////////////////////////////////////////////
// ElectricBall.h
// ElectricBall - артефакт электрический шар
///////////////////////////////////////////////////////////////

#pragma once
#include "artefact.h"

class CElectricBall : public CArtefact
{
private:
    typedef CArtefact inherited;

public:
    CElectricBall(void);
    virtual ~CElectricBall(void);

    virtual void Load(LPCSTR section);

    virtual void net_Import(NET_Packet& P);					// import from server
    virtual void net_Export(NET_Packet& P);					// export to server
protected:
    virtual void UpdateCLChild();
};
