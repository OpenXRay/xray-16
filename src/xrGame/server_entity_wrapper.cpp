////////////////////////////////////////////////////////////////////////////
//  Module      : server_entity_wrapper.cpp
//  Created     : 16.10.2004
//  Modified    : 16.10.2004
//  Author      : Dmitriy Iassenev
//  Description : Server entity wrapper
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "server_entity_wrapper.h"
#include "xrServer_Objects.h"
#include "xrMessages.h"

#ifdef AI_COMPILER
#include "factory_api.h"
#endif

class IServerEntity;

CServerEntityWrapper::~CServerEntityWrapper() { F_entity_Destroy(m_object); }
void CServerEntityWrapper::save(IWriter& stream)
{
    NET_Packet net_packet;

    // Spawn
    stream.open_chunk(0);

    m_object->Spawn_Write(net_packet, TRUE);
    stream.w_u16(u16(net_packet.B.count));
    stream.w(net_packet.B.data, net_packet.B.count);

    stream.close_chunk();

    // Update
    stream.open_chunk(1);

    net_packet.w_begin(M_UPDATE);
    m_object->UPDATE_Write(net_packet);
    stream.w_u16(u16(net_packet.B.count));
    stream.w(net_packet.B.data, net_packet.B.count);

    //  u16                     ID;
    //  net_packet.r_begin      (ID);
    //  VERIFY                  (ID==M_UPDATE);
    //  m_object->UPDATE_Read   (net_packet);

    stream.close_chunk();
}

void CServerEntityWrapper::load(IReader& stream)
{
    NET_Packet net_packet;
    u16 ID;
    IReader* chunk;

    chunk = stream.open_chunk(0);

    net_packet.B.count = chunk->r_u16();
    chunk->r(net_packet.B.data, net_packet.B.count);

    chunk->close();

    net_packet.r_begin(ID);
    R_ASSERT2(M_SPAWN == ID, "Invalid packet ID (!= M_SPAWN)!");

    string64 s_name;
    net_packet.r_stringZ(s_name);

    m_object = F_entity_Create(s_name);

    R_ASSERT3(m_object, "Can't create entity.", s_name);
    m_object->Spawn_Read(net_packet);

    chunk = stream.open_chunk(1);

    net_packet.B.count = chunk->r_u16();
    chunk->r(net_packet.B.data, net_packet.B.count);

    chunk->close();

    net_packet.r_begin(ID);
    R_ASSERT2(M_UPDATE == ID, "Invalid packet ID (!= M_UPDATE)!");
    m_object->UPDATE_Read(net_packet);
}

void CServerEntityWrapper::save_update(IWriter& stream)
{
    //  NET_Packet              net_packet;
    //  net_packet.w_begin      (M_UPDATE);
    //  m_object->save_update   (net_packet);
    //  stream.w_u16            (u16(net_packet.B.count));
    //  stream.w                (net_packet.B.data,net_packet.B.count);
}

void CServerEntityWrapper::load_update(IReader& stream)
{
    //  NET_Packet              net_packet;
    //  u16                     ID;
    //
    //  net_packet.B.count      = stream.r_u16();
    //  stream.r                (net_packet.B.data,net_packet.B.count);
    //
    //  net_packet.r_begin      (ID);
    //  R_ASSERT2               (M_UPDATE == ID,"Invalid packet ID (!= M_UPDATE)!");
    //  m_object->load_update   (net_packet);
}
