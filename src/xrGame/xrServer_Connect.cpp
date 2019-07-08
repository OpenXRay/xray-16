#include "stdafx.h"
#include "xrServer.h"
#include "game_sv_single.h"

xrServer::EConnect xrServer::Connect(shared_str& session_name)
{
#ifdef DEBUG
    Msg("* sv_Connect: %s", *session_name);
#endif

    // Parse options and create game
    if (0 == strchr(*session_name, '/'))
        return ErrConnect;

    game = new game_sv_Single();
    game->Create(session_name);

    return IPureServer::Connect(*session_name);
}
