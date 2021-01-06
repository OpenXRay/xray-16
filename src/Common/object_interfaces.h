////////////////////////////////////////////////////////////////////////////
//	Module 		: object_interfaces.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife interfaces
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/_types.h"

class NET_Packet;
class IReader;
class IWriter;

class IPureDestroyableObject
{
public:
    virtual ~IPureDestroyableObject() = 0;

    virtual void destroy() = 0;
};

inline IPureDestroyableObject::~IPureDestroyableObject() = default;

class ISerializable
{
public:
    virtual ~ISerializable() = 0;
    virtual void load(IReader& reader) = 0;
    virtual void save(IWriter& writer) = 0;
};

inline ISerializable::~ISerializable() = default;

class IPureServerObject : public ISerializable
{
public:
    virtual void STATE_Write(NET_Packet& tNetPacket) = 0;
    virtual void STATE_Read(NET_Packet& tNetPacket, u16 size) = 0;
    virtual void UPDATE_Write(NET_Packet& tNetPacket) = 0;
    virtual void UPDATE_Read(NET_Packet& tNetPacket) = 0;
};

class IPureSchedulableObject
{
public:
    virtual ~IPureSchedulableObject() = 0;
    virtual void update() = 0;
};

inline IPureSchedulableObject::~IPureSchedulableObject() = default;
