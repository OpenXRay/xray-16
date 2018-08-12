#include "StdAfx.h"
#include "xrServer.h"
#include "xrMessages.h"

#if 1 // def DEBUG
#define USE_DESIGNER_KEY
#endif

#ifdef USE_DESIGNER_KEY
#include "xrServer_Objects_ALife_Monsters.h"
#endif

void xrServer::SLS_Default()
{
    if (game->custom_sls_default())
    {
        game->sls_default();
        return;
    }

#ifdef USE_DESIGNER_KEY
    bool _designer = !!strstr(Core.Params, "-designer");
    CSE_ALifeCreatureActor* _actor = 0;
#endif

    string_path fn_spawn;
    if (FS.exist(fn_spawn, "$level$", "level.spawn"))
    {
        IReader* SP = FS.r_open(fn_spawn);
        NET_Packet P;
        u32 S_id;
        for (IReader* S = SP->open_chunk_iterator(S_id); S; S = SP->open_chunk_iterator(S_id, S))
        {
            P.B.count = S->length();
            S->r(P.B.data, P.B.count);

            u16 ID;
            P.r_begin(ID);
            R_ASSERT(M_SPAWN == ID);
            ClientID clientID;
            clientID.set(0);

#ifdef USE_DESIGNER_KEY
            CSE_Abstract* entity =
#endif
                Process_spawn(P, clientID);
#ifdef USE_DESIGNER_KEY
            if (_designer)
            {
                CSE_ALifeCreatureActor* actor = smart_cast<CSE_ALifeCreatureActor*>(entity);
                if (actor)
                    _actor = actor;
            }
#endif
        }
        FS.r_close(SP);
    }

#ifdef USE_DESIGNER_KEY
    if (!_designer)
        return;

    if (_actor)
        return;

    _actor = smart_cast<CSE_ALifeCreatureActor*>(entity_Create("actor"));
    _actor->o_Position = Fvector().set(0.f, 0.f, 0.f);
    _actor->set_name_replace("designer");
    _actor->s_flags.flags |= M_SPAWN_OBJECT_ASPLAYER;
    NET_Packet packet;
    packet.w_begin(M_SPAWN);
    _actor->Spawn_Write(packet, TRUE);

    u16 id;
    packet.r_begin(id);
    R_ASSERT(id == M_SPAWN);
    ClientID clientID;
    clientID.set(0);
    Process_spawn(packet, clientID);
#endif
}
