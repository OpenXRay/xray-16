////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_space.h
//	Created 	: 30.06.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory space
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef XRGAME_EXPORTS
#define NO_XR_GAME
#endif

class IFactoryObject;
class CSE_Abstract;

namespace ObjectFactory
{
using ClientObjectBaseClass = IFactoryObject;
using ServerObjectBaseClass = CSE_Abstract;
};
