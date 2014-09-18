////////////////////////////////////////////////////////////////////////////
//	Module 		: movement_manager_space.h
//	Created 	: 02.10.2001
//  Modified 	: 12.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Movement manager space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace MovementManager {
	enum EPathType {
		ePathTypeGamePath = u32(0),
		ePathTypeLevelPath,
		ePathTypePatrolPath,
		ePathTypeNoPath,
		ePathTypeDummy = u32(-1),
	};
};

