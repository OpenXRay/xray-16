#include "StdAfx.h"
#include "game_type.h"
#include "Level.h"

bool OnServer() throw() { return Level().IsServer(); }
bool OnClient() throw() { return Level().IsClient(); }
bool IsGameTypeSingle() throw() { return (g_pGamePersistent->GameType() == eGameIDSingle); }
