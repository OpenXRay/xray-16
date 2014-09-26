////////////////////////////////////////////////////////////////////////////
//	Module 		: sight_manager_space.h
//	Created 	: 27.12.2003
//  Modified 	: 03.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Sight actiomanager spacen
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace SightManager {
	enum ESightType {
		eSightTypeCurrentDirection	= u32(0),
		eSightTypePathDirection,
		eSightTypeDirection,
		eSightTypePosition,
		eSightTypeObject,
		eSightTypeCover,
		eSightTypeSearch,
		eSightTypeLookOver,
		eSightTypeCoverLookOver,
		eSightTypeFireObject,
		eSightTypeFirePosition,		// must be removed
		eSightTypeAnimationDirection,
		eSightTypeDummy				= u32(-1),
	};
};