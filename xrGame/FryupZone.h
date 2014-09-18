#pragma once

#include "script_object.h"

class CFryupZone : public CScriptObject {
	typedef	CScriptObject	inherited;

public:
	CFryupZone	();
	virtual			~CFryupZone	();

#ifdef DEBUG
	virtual void	OnRender				( );
#endif

};