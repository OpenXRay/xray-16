////////////////////////////////////////////////////////////////////////////
//	Module 		: object_interfaces.h
//	Created 	: 05.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife interfaces
////////////////////////////////////////////////////////////////////////////

#pragma once

class NET_Packet;

class IPureDestroyableObject {
public:
	virtual ~IPureDestroyableObject() = 0;
	virtual void					destroy()											= 0;
};

IC IPureDestroyableObject::~IPureDestroyableObject() {}
template <typename _storage_type>
class IPureLoadableObject {
public:
	virtual ~IPureLoadableObject() = 0;
	virtual void					load(_storage_type	&storage)						= 0;
};

template <typename _storage_type>
IC IPureLoadableObject<_storage_type>::~IPureLoadableObject() {}
template <typename _storage_type>
class IPureSavableObject {
public:
	virtual ~IPureSavableObject() = 0;
	virtual void					save(_storage_type	&storage)						= 0;
};

template <typename _storage_type>
IC IPureSavableObject<_storage_type>::~IPureSavableObject() {}
template <typename _storage_type_load, typename _storage_type_save>
class IPureSerializeObject : public IPureLoadableObject<_storage_type_load>, public IPureSavableObject<_storage_type_save> {
public:
    virtual ~IPureSerializeObject() = 0;
};

template <typename _storage_type_load, typename _storage_type_save>
IC IPureSerializeObject<typename _storage_type_load, typename _storage_type_save>::~IPureSerializeObject() {}
class IPureServerObject : public IPureSerializeObject<IReader,IWriter> {
public:
    virtual ~IPureServerObject() = 0;
	virtual void					STATE_Write	(NET_Packet &tNetPacket)				= 0;
	virtual void					STATE_Read	(NET_Packet &tNetPacket, u16 size)		= 0;
	virtual void					UPDATE_Write(NET_Packet &tNetPacket)				= 0;
	virtual void					UPDATE_Read	(NET_Packet &tNetPacket)				= 0;
};

IC IPureServerObject::~IPureServerObject() {}
class IPureSchedulableObject {
public:
	virtual ~IPureSchedulableObject() = 0;
	virtual void					update		()										= 0;
};

IC IPureSchedulableObject::~IPureSchedulableObject() {}
