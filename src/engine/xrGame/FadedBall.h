///////////////////////////////////////////////////////////////
// FadedBall.h
// FadedBall - артефакт блеклый шар
///////////////////////////////////////////////////////////////

#pragma once
#include "artefact.h"

class CFadedBall : public CArtefact 
{
private:
	typedef CArtefact inherited;
public:
	CFadedBall(void);
	virtual ~CFadedBall(void);

	virtual void Load				(LPCSTR section);

protected:
};