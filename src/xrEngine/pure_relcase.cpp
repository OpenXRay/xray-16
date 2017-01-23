#include "pure_relcase.h"
#include "IGame_Level.h"
#include "stdafx.h"
#include "xr_object_list.h"

pure_relcase::~pure_relcase()
{
    if (g_pGameLevel) g_pGameLevel->Objects.relcase_unregister(&m_ID);
}
