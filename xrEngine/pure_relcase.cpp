#include "stdafx.h"
#include "pure_relcase.h"
#include "xr_object_list.h"
#include "IGame_Level.h"

pure_relcase::~pure_relcase	()
{
	if (g_pGameLevel) 
		g_pGameLevel->Objects.relcase_unregister	(&m_ID);
}
