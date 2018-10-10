////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory.cpp
//	Created 	: 27.05.2004
//  Modified 	: 27.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "object_factory.h"
#include "Common/object_broker.h"

CObjectFactory* g_object_factory = nullptr;

CObjectFactory::CObjectFactory()
{
    m_actual = false;
    register_classes();
}

CObjectFactory::~CObjectFactory() { delete_data(m_clsids); }
void CObjectFactory::init() { register_script_classes(); }
